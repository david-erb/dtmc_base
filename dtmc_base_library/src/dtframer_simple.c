// dtframer_simple.c
// Simple framing of messages into byte streams and vice versa.

#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dtbytes.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtmc_base/dtframer.h>
#include <dtmc_base/dtframer_simple.h>

// Declare global vtable instance for this class
DTFRAMER_INIT_VTABLE(dtframer_simple)

// Fixed framing format (no CRC, no compression)
//
// Wire format per message:
//
//   magic_first   : 1 byte (0xA5)
//   magic_second  : 1 byte (0x5A)
//   version       : 1 byte
//   topic_length  : 2 bytes, big-endian
//   payload_length: 2 bytes, big-endian
//   topic_bytes   : topic_length bytes (no terminator on the wire)
//   payload_bytes : payload_length bytes
//
// This works equally well when the underlying transport
// is a long UART stream or a chunked CAN-based scheme,
// as long as all bytes are eventually delivered in order.

// Magic bytes for frame start
#define DTFRAMER_SIMPLE_MAGIC_FIRST_BYTE ((uint8_t)0xA5)
#define DTFRAMER_SIMPLE_MAGIC_SECOND_BYTE ((uint8_t)0x5A)

// Size of the fixed header after the two magic bytes
#define DTFRAMER_SIMPLE_FIXED_HEADER_SIZE 5u

// Defaults tuned so you can choose “UART style” vs “CAN style” in config
#define DTFRAMER_SIMPLE_DEFAULT_MAX_TOPIC_STREAM 64u
#define DTFRAMER_SIMPLE_DEFAULT_MAX_PAYLOAD_STREAM 2048u

#define DTFRAMER_SIMPLE_DEFAULT_MAX_TOPIC_BUS 32u
#define DTFRAMER_SIMPLE_DEFAULT_MAX_PAYLOAD_BUS 256u

#define TAG "dtframer_simple"
#define dtlog_debug(...)

// -----------------------------------------------------------------------------
// Internal decode state machine

typedef enum
{
    DTFRAMER_SIMPLE_DECODE_PHASE_WAIT_MAGIC_FIRST = 0,
    DTFRAMER_SIMPLE_DECODE_PHASE_WAIT_MAGIC_SECOND,
    DTFRAMER_SIMPLE_DECODE_PHASE_READ_HEADER,
    DTFRAMER_SIMPLE_DECODE_PHASE_READ_TOPIC,
    DTFRAMER_SIMPLE_DECODE_PHASE_READ_PAYLOAD,
} dtframer_simple_decode_phase_t;

/*------------------------------------------------------------------------*/
// Concrete instance layout (private to this TU)
typedef struct dtframer_simple_t
{
    DTFRAMER_COMMON_MEMBERS; // int32_t model_number;
    bool _is_malloced;

    dtframer_simple_config_t config; // copy of configuration settings

    // Decode state
    dtframer_simple_decode_phase_t decode_phase;
    uint8_t header_buffer[DTFRAMER_SIMPLE_FIXED_HEADER_SIZE];
    uint32_t header_bytes_filled;

    uint16_t current_topic_length;
    uint16_t current_payload_length;

    char* topic_storage;
    uint32_t topic_bytes_filled;

    uint8_t* payload_storage;
    uint32_t payload_bytes_filled;

    // Called when a complete message has been decoded.
    // Optional; if NULL, decoded messages are discarded.
    dtframer_message_callback_t message_callback_function;

    // Passed back to callbacks.
    void* message_callback_context;

} dtframer_simple_t;

/*------------------------------------------------------------------------*/
static void
dtframer_simple_apply_defaults(dtframer_simple_t* self)
{
    // Fill in sensible defaults if caller left fields at zero.
    if (self->config.protocol_version == 0)
    {
        self->config.protocol_version = 1;
    }

    if (self->config.maximum_topic_length == 0 || self->config.maximum_payload_length == 0)
    {
        if (self->config.framing_mode == DTFRAMER_SIMPLE_FRAMING_MODE_BUS)
        {
            if (self->config.maximum_topic_length == 0)
                self->config.maximum_topic_length = DTFRAMER_SIMPLE_DEFAULT_MAX_TOPIC_BUS;
            if (self->config.maximum_payload_length == 0)
                self->config.maximum_payload_length = DTFRAMER_SIMPLE_DEFAULT_MAX_PAYLOAD_BUS;
        }
        else
        {
            if (self->config.maximum_topic_length == 0)
                self->config.maximum_topic_length = DTFRAMER_SIMPLE_DEFAULT_MAX_TOPIC_STREAM;
            if (self->config.maximum_payload_length == 0)
                self->config.maximum_payload_length = DTFRAMER_SIMPLE_DEFAULT_MAX_PAYLOAD_STREAM;
        }
    }
}

/*------------------------------------------------------------------------*/
static void
dtframer_simple_reset_decode_state(dtframer_simple_t* self)
{
    self->decode_phase = DTFRAMER_SIMPLE_DECODE_PHASE_WAIT_MAGIC_FIRST;
    self->header_bytes_filled = 0;
    self->current_topic_length = 0;
    self->current_payload_length = 0;
    self->topic_bytes_filled = 0;
    self->payload_bytes_filled = 0;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtframer_simple_create(dtframer_simple_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    *self_ptr = (dtframer_simple_t*)malloc(sizeof(dtframer_simple_t));
    if (*self_ptr == NULL)
    {
        dterr = dterr_new(
          DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for dtframer_simple_t", sizeof(dtframer_simple_t));
        goto cleanup;
    }

    DTERR_C(dtframer_simple_init(*self_ptr));
    (*self_ptr)->_is_malloced = true;

cleanup:
    if (dterr != NULL)
    {
        if (self_ptr != NULL && *self_ptr != NULL)
        {
            free(*self_ptr);
            *self_ptr = NULL;
        }
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "dtframer_simple_create failed");
    }
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtframer_simple_init(dtframer_simple_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_FRAMER_MODEL_SIMPLE; // matches your registry constants

    // Publish vtable for this model (idempotent in single-threaded tests).
    DTERR_C(dtframer_set_vtable(self->model_number, &dtframer_simple_vt));

    dtframer_simple_reset_decode_state(self);

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "dtframer_simple_init failed");
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtframer_simple_configure(dtframer_simple_t* self, dtframer_simple_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    // Free any previous decode storage, if configure is called more than once.
    if (self->topic_storage != NULL)
    {
        free(self->topic_storage);
        self->topic_storage = NULL;
    }
    if (self->payload_storage != NULL)
    {
        free(self->payload_storage);
        self->payload_storage = NULL;
    }

    self->config = *config;
    dtframer_simple_apply_defaults(self);

    // Allocate decode buffers sized to configured maxima.
    self->topic_storage = (char*)malloc(self->config.maximum_topic_length + 1u); // +1 for terminator
    if (self->topic_storage == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM,
          DTERR_LOC,
          NULL,
          "failed to allocate %u bytes for topic_storage",
          (unsigned int)(self->config.maximum_topic_length + 1u));
        goto cleanup;
    }

    self->payload_storage = (uint8_t*)malloc(self->config.maximum_payload_length);
    if (self->payload_storage == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM,
          DTERR_LOC,
          NULL,
          "failed to allocate %u bytes for payload_storage",
          (unsigned int)self->config.maximum_payload_length);
        goto cleanup;
    }

    dtframer_simple_reset_decode_state(self);

cleanup:
    if (dterr != NULL)
    {
        if (self->topic_storage != NULL)
        {
            free(self->topic_storage);
            self->topic_storage = NULL;
        }
        if (self->payload_storage != NULL)
        {
            free(self->payload_storage);
            self->payload_storage = NULL;
        }
    }
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtframer_simple_set_message_callback(dtframer_simple_t* self DTFRAMER_SET_MESSAGE_CALLBACK_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    self->message_callback_function = callback_function;
    self->message_callback_context = callback_context;

cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/
// Helper: big-endian store/load for 16-bit lengths

static void
dtframer_simple_store_u16_be(uint8_t* destination, uint16_t value)
{
    destination[0] = (uint8_t)((value >> 8) & 0xFFu);
    destination[1] = (uint8_t)(value & 0xFFu);
}

static uint16_t
dtframer_simple_load_u16_be(const uint8_t* source)
{
    uint16_t high = (uint16_t)source[0];
    uint16_t low = (uint16_t)source[1];
    return (uint16_t)((high << 8) | low);
}

/*------------------------------------------------------------------------*/

dterr_t*
dtframer_simple_encode(dtframer_simple_t* self DTFRAMER_ENCODE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(topic);
    DTERR_ASSERT_NOT_NULL(payload);
    DTERR_ASSERT_NOT_NULL(encoded_buffer);

#if !defined(dtlog_debug)
    {
        int32_t topic_length = (int32_t)strlen(topic);
        char* payload_hex[2 * payload->length + payload->length / 4 + 2];
        dtbytes_compose_hex((const char*)payload->payload, (int)payload->length, (char*)payload_hex, sizeof(payload_hex));
        dtlog_debug(TAG,
          "dtframer_simple_encode: encoding message: topic='%s' topic_length=%" PRId32 " payload_length=%" PRId32
          " payload_hex='%s'",
          topic,
          topic_length,
          payload->length,
          (char*)payload_hex);
    }
#endif

    int32_t topic_length = (int32_t)strlen(topic);
    if ((uint32_t)topic_length > self->config.maximum_topic_length)
    {
        dterr = dterr_new(DTERR_BOUNDS,
          DTERR_LOC,
          NULL,
          "topic length %" PRId32 " exceeds configured maximum %" PRId32,
          topic_length,
          self->config.maximum_topic_length);
        goto cleanup;
    }

    uint16_t topic_length_u16 = (uint16_t)topic_length;
    uint16_t payload_length_u16 = (uint16_t)payload->length;

    int32_t header_size = 2u /* magic bytes */
                          + DTFRAMER_SIMPLE_FIXED_HEADER_SIZE;
    int32_t total_length = header_size + topic_length + payload->length;

    if (encoded_buffer->length < (int32_t)total_length)
    {
        dterr = dterr_new(DTERR_BOUNDS,
          DTERR_LOC,
          NULL,
          "encoded_buffer length %" PRId32 " is insufficient for encoded message of total length %" PRId32,
          encoded_buffer->length,
          total_length);
        goto cleanup;
    }

    uint8_t* write_pointer = encoded_buffer->payload;

    // Magic bytes
    *write_pointer++ = DTFRAMER_SIMPLE_MAGIC_FIRST_BYTE;
    *write_pointer++ = DTFRAMER_SIMPLE_MAGIC_SECOND_BYTE;

    // Version
    *write_pointer++ = self->config.protocol_version;

    // Topic length and payload length (big endian)
    dtframer_simple_store_u16_be(write_pointer, topic_length_u16);
    write_pointer += 2;
    dtframer_simple_store_u16_be(write_pointer, payload_length_u16);
    write_pointer += 2;

    // We have now written DTFRAMER_SIMPLE_FIXED_HEADER_SIZE bytes
    // after the magic bytes; assertion for sanity.
    // (version + 2 + 2 == 5)
    // No runtime check needed here; the types guarantee it.

    // Topic bytes (no terminator on the wire)
    if (topic_length > 0)
    {
        memcpy(write_pointer, topic, topic_length);
        write_pointer += topic_length;
    }

    // Payload bytes
    if (payload->length > 0)
    {
        memcpy(write_pointer, payload->payload, (size_t)payload->length);
        write_pointer += payload->length;
    }

    *encoded_length = (int32_t)total_length;

cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/

dterr_t*
dtframer_simple_decode(dtframer_simple_t* self DTFRAMER_DECODE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(buffer);

    if (length < 0)
    {
        dterr = dterr_new(DTERR_BOUNDS, DTERR_LOC, NULL, "negative length (%d) is not allowed", length);
        goto cleanup;
    }

#if !defined(dtlog_debug)
    {
        int32_t hex_length = length > 16 ? 16 : length;
        char payload_hex[2 * hex_length + hex_length / 4 + 2];
        dtbytes_compose_hex((const char*)buffer, (int)hex_length, (char*)payload_hex, sizeof(payload_hex));
        dtlog_debug(TAG,
          "dtframer_simple_decode: phase %d decoding %" PRId32 " bytes: payload_hex='%s'",
          self->decode_phase,
          length,
          (char*)payload_hex);
    }
#endif

    uint32_t byte_index = 0u;
    uint32_t input_length = (uint32_t)length;

    while (byte_index < input_length)
    {
        uint8_t current_byte = buffer[byte_index];

        switch (self->decode_phase)
        {
            case DTFRAMER_SIMPLE_DECODE_PHASE_WAIT_MAGIC_FIRST:
                if (current_byte == DTFRAMER_SIMPLE_MAGIC_FIRST_BYTE)
                {
                    self->decode_phase = DTFRAMER_SIMPLE_DECODE_PHASE_WAIT_MAGIC_SECOND;
                }
                // Any non-magic byte is ignored while searching for frame start.
                byte_index++;
                break;

            case DTFRAMER_SIMPLE_DECODE_PHASE_WAIT_MAGIC_SECOND:
                if (current_byte == DTFRAMER_SIMPLE_MAGIC_SECOND_BYTE)
                {
                    self->decode_phase = DTFRAMER_SIMPLE_DECODE_PHASE_READ_HEADER;
                    self->header_bytes_filled = 0;
                }
                else if (current_byte == DTFRAMER_SIMPLE_MAGIC_FIRST_BYTE)
                {
                    // Stay in this phase but treat this byte as the first magic byte.
                    // Do not advance the phase; the next byte may be the second magic.
                    // We still advance byte_index, because we consumed this byte.
                    // The state machine will see it as "magic first" when it loops back.
                    // Simplest: go back to WAIT_MAGIC_SECOND and keep looking.
                    // Here we treat it as a fresh potential start.
                    // Implemented by remaining in WAIT_MAGIC_SECOND.
                }
                else
                {
                    // False start, go back to searching for first magic byte.
                    self->decode_phase = DTFRAMER_SIMPLE_DECODE_PHASE_WAIT_MAGIC_FIRST;
                }
                byte_index++;
                break;

            case DTFRAMER_SIMPLE_DECODE_PHASE_READ_HEADER:
            {
                uint32_t header_bytes_remaining = DTFRAMER_SIMPLE_FIXED_HEADER_SIZE - self->header_bytes_filled;
                uint32_t bytes_available = input_length - byte_index;
                uint32_t chunk_size = (bytes_available < header_bytes_remaining) ? bytes_available : header_bytes_remaining;

                if (chunk_size > 0)
                {
                    memcpy(self->header_buffer + self->header_bytes_filled, buffer + byte_index, chunk_size);
                    self->header_bytes_filled += chunk_size;
                    byte_index += chunk_size;
                }

                if (self->header_bytes_filled == DTFRAMER_SIMPLE_FIXED_HEADER_SIZE)
                {
                    uint8_t version = self->header_buffer[0];
                    uint16_t topic_length_u16 = dtframer_simple_load_u16_be(self->header_buffer + 1);
                    uint16_t payload_length_u16 = dtframer_simple_load_u16_be(self->header_buffer + 3);

                    // Version check (optional if protocol_version was left as zero)
                    if (self->config.protocol_version != 0 && version != self->config.protocol_version)
                    {
                        dterr = dterr_new(DTERR_CORRUPT,
                          DTERR_LOC,
                          NULL,
                          "unexpected version %u (expected %u)",
                          (unsigned int)version,
                          (unsigned int)self->config.protocol_version);
                        dtframer_simple_reset_decode_state(self);
                        goto cleanup;
                    }

                    if (topic_length_u16 > self->config.maximum_topic_length)
                    {
                        dterr = dterr_new(DTERR_BOUNDS,
                          DTERR_LOC,
                          NULL,
                          "topic length %u exceeds configured maximum %u",
                          (unsigned int)topic_length_u16,
                          (unsigned int)self->config.maximum_topic_length);
                        dtframer_simple_reset_decode_state(self);
                        goto cleanup;
                    }

                    if (payload_length_u16 > self->config.maximum_payload_length)
                    {
                        dterr = dterr_new(DTERR_BOUNDS,
                          DTERR_LOC,
                          NULL,
                          "payload length %u exceeds configured maximum %u",
                          (unsigned int)payload_length_u16,
                          (unsigned int)self->config.maximum_payload_length);
                        dtframer_simple_reset_decode_state(self);
                        goto cleanup;
                    }

                    self->current_topic_length = topic_length_u16;
                    self->current_payload_length = payload_length_u16;
                    self->topic_bytes_filled = 0;
                    self->payload_bytes_filled = 0;

                    self->decode_phase = (self->current_topic_length > 0) ? DTFRAMER_SIMPLE_DECODE_PHASE_READ_TOPIC
                                                                          : DTFRAMER_SIMPLE_DECODE_PHASE_READ_PAYLOAD;
                }
                break;
            }

            case DTFRAMER_SIMPLE_DECODE_PHASE_READ_TOPIC:
            {
                uint32_t topic_bytes_remaining = (uint32_t)self->current_topic_length - self->topic_bytes_filled;
                uint32_t bytes_available = input_length - byte_index;
                uint32_t chunk_size = (bytes_available < topic_bytes_remaining) ? bytes_available : topic_bytes_remaining;

                if (chunk_size > 0)
                {
                    memcpy((uint8_t*)self->topic_storage + self->topic_bytes_filled, buffer + byte_index, chunk_size);
                    self->topic_bytes_filled += chunk_size;
                    byte_index += chunk_size;
                }

                if (self->topic_bytes_filled == (uint32_t)self->current_topic_length)
                {
                    // Add string terminator for callback convenience.
                    self->topic_storage[self->current_topic_length] = '\0';
                    self->decode_phase = DTFRAMER_SIMPLE_DECODE_PHASE_READ_PAYLOAD;
                }
                break;
            }

            case DTFRAMER_SIMPLE_DECODE_PHASE_READ_PAYLOAD:
            {
                uint32_t payload_bytes_remaining = (uint32_t)self->current_payload_length - self->payload_bytes_filled;
                uint32_t bytes_available = input_length - byte_index;
                uint32_t chunk_size = (bytes_available < payload_bytes_remaining) ? bytes_available : payload_bytes_remaining;

                if (chunk_size > 0)
                {
                    memcpy(self->payload_storage + self->payload_bytes_filled, buffer + byte_index, chunk_size);
                    self->payload_bytes_filled += chunk_size;
                    byte_index += chunk_size;
                }

                if (self->payload_bytes_filled == (uint32_t)self->current_payload_length)
                {
                    // We have a complete message: invoke callback, if configured.
                    if (self->message_callback_function != NULL)
                    {
#if !defined(dtlog_debug)
                        {
                            char payload_hex[2 * self->current_payload_length +     // hex representation
                                             self->current_payload_length / 4 + 2]; // with spaces
                            dtbytes_compose_hex((const char*)self->payload_storage,
                              (int)self->current_payload_length,
                              payload_hex,
                              sizeof(payload_hex));
                            dtlog_debug(TAG,
                              "dtframer_simple_decode: complete message received: topic='%s' payload_length=%u "
                              "payload_hex='%s'",
                              self->topic_storage,
                              (unsigned int)self->current_payload_length,
                              payload_hex);
                        }
#endif
                        dtbuffer_t payload_buffer;
                        DTERR_C(dtbuffer_wrap(&payload_buffer, self->payload_storage, (int32_t)self->current_payload_length));

                        // call the message callback function provided in the config
                        dterr_t* callback_error =
                          self->message_callback_function(self->message_callback_context, self->topic_storage, &payload_buffer);

                        if (callback_error != NULL)
                        {
                            // Propagate the callback's error up to caller.
                            dterr = dterr_new(callback_error->error_code, DTERR_LOC, callback_error, "message callback failed");
                            dtframer_simple_reset_decode_state(self);
                            goto cleanup;
                        }
                    }

                    // Prepare for next message in the stream (which may already
                    // be present in the buffer).
                    dtframer_simple_reset_decode_state(self);
                }
                break;
            }

            default:
                // Should never happen; reset state machine.
                dtframer_simple_reset_decode_state(self);
                byte_index++; // Avoid infinite loop.
                break;
        }
    }

cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/
void
dtframer_simple_dispose(dtframer_simple_t* self)
{
    if (self == NULL)
        return;

    if (self->topic_storage != NULL)
    {
        free(self->topic_storage);
        self->topic_storage = NULL;
    }

    if (self->payload_storage != NULL)
    {
        free(self->payload_storage);
        self->payload_storage = NULL;
    }

    if (self->_is_malloced)
    {
        free(self);
    }
    else
    {
        memset(self, 0, sizeof(*self));
    }
}
