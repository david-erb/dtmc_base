#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dtbytes.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtframer.h>
#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtmanifold.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dttasker.h>

#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtnetportal_iox.h>

DTNETPORTAL_INIT_VTABLE(dtnetportal_iox);
DTOBJECT_INIT_VTABLE(dtnetportal_iox);

#define TAG "dtnetportal_iox"

#define dtlog_debug(TAG, ...)

// --------------------------------------------------------------------------------------
typedef struct dtnetportal_iox_t
{
    DTNETPORTAL_COMMON_MEMBERS;

    dtnetportal_iox_config_t config;

    dtsemaphore_handle rx_semaphore_handle;
    dttasker_handle rxtasker_handle;

    dtmanifold_t _manifold, *manifold; // manifold for managing topics

    dtbuffer_t* encoded_buffer;
    dtbuffer_t* raw_input_buffer;

    bool _is_malloced;
    bool _should_fan_out;

    bool _should_rx_thread_stop;

    dterr_t* internal_task_dterr;

} dtnetportal_iox_t;

extern dtnetportal_iox_t* static_self;

// --------------------------------------------------------------------------------------
// user-space callback from framer with completed message
static dterr_t*
_framer_decoded_callback(void* opaque_self, const char* topic, dtbuffer_t* buffer)
{
    dterr_t* dterr = NULL;
    dtnetportal_iox_t* self = NULL;
    DTERR_ASSERT_NOT_NULL(opaque_self);
    self = (dtnetportal_iox_t*)opaque_self;

    if (self->_should_fan_out)
    {
        dtlog_debug(TAG, "received and fanning out %" PRId32 " bytes to topic \"%s\"", buffer->length, topic);
        // publish the data to the manifold (it will make a copy of the buffer for each recipient)
        DTERR_C(dtmanifold_publish(self->manifold, topic, buffer));
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// user-space task to monitor incoming raw bytes from the iox
static dterr_t*
_rx_tasker_entry_point(void* opaque_self, dttasker_handle tasker_handle)
{
    dterr_t* dterr;
    dtnetportal_iox_t* self = NULL;
    DTERR_ASSERT_NOT_NULL(opaque_self);
    self = (dtnetportal_iox_t*)opaque_self;

    // signal we are prepared to work
    DTERR_C(dttasker_ready(tasker_handle));

    while (true)
    {
        // wait for iox to indicate new bytes have arrived
        DTERR_C(dtsemaphore_wait(self->rx_semaphore_handle, DTTIMEOUT_FOREVER, NULL));

        if (self->_should_rx_thread_stop)
        {
            dtlog_debug(TAG, "rx thread loop stopping upon request");
            break;
        }

        // drain the iox rx fifo
        DTERR_C(dtnetportal_iox_rx_drain(self));
    }

cleanup:
    if (dterr != NULL)
    {
        if (self)
        {
            // keep the error chain for our own info
            self->internal_task_dterr = dterr;
        }

        // give an independent error to the tasker (not chained since disposal can happen separately)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "netportal error (details in netportal info): %s", dterr->message);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------
// drain the iox rx fifo
// this breakout is to facilitate testing in a dry environment when tasks aren't really available
dterr_t*
dtnetportal_iox_rx_drain(dtnetportal_iox_t* self)
{
    dterr_t* dterr;
    DTERR_ASSERT_NOT_NULL(self);

    int32_t count;
    int32_t have_waited = 0;

    // experiment: never re-wait
    have_waited = 1;

    while (true)
    {
        DTERR_C(dtiox_read(                // non-blocking read for whatever bytes are in the iox's ring buffer
          self->config.iox_handle,         //
          self->raw_input_buffer->payload, //
          self->raw_input_buffer->length,  //
          &count));

        dtlog_debug(TAG, "dtnetportal_iox_rx_drain read %" PRId32 " bytes", count);

        if (count == 0)
        {
            if (have_waited >= 0)
            {
                // already waited enough, no more data
                break;
            }
            have_waited++;
            dtruntime_sleep_milliseconds(1);
            continue;
        }

        // decode the raw bytes and callback if a message is complete
        DTERR_C(dtframer_decode(self->config.framer_handle, self->raw_input_buffer->payload, count));

        have_waited = 0; // reset wait counter since we got data
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
extern dterr_t*
dtnetportal_iox_create(dtnetportal_iox_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    *self_ptr = (dtnetportal_iox_t*)malloc(sizeof(dtnetportal_iox_t));
    if (*self_ptr == NULL)
    {
        dterr = dterr_new(
          DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for dtnetportal_iox_t", sizeof(dtnetportal_iox_t));
        goto cleanup;
    }

    DTERR_C(dtnetportal_iox_init(*self_ptr));

    (*self_ptr)->_is_malloced = true;

cleanup:

    if (dterr != NULL)
    {
        if (*self_ptr != NULL)
        {
            free(*self_ptr);
        }

        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s(): failed", __func__);
    }
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_iox_init(dtnetportal_iox_t* self)
{
    dterr_t* dterr = NULL;

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_NETPORTAL_MODEL_IOX;

    // set the vtable for this model number
    DTERR_C(dtnetportal_set_vtable(self->model_number, &dtnetportal_iox_vt));
    DTERR_C(dtobject_set_vtable(self->model_number, &dtnetportal_iox_object_vt));

    // binary semaphore for rx task wakeup
    DTERR_C(dtsemaphore_create(&self->rx_semaphore_handle, 0, 0));

    // make a private manifold for fan-out
    self->manifold = &self->_manifold;
    DTERR_C(dtmanifold_init(self->manifold));

    // make space to encode the framed message
    int max_encoded_buffer_length = 1024;
    DTERR_C(dtbuffer_create(&self->encoded_buffer, max_encoded_buffer_length));

    // make space to encode the raw input bytes
    DTERR_C(dtbuffer_create(&self->raw_input_buffer, 128));

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s(): failed", __func__);
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_iox_configure(dtnetportal_iox_t* self, dtnetportal_iox_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);
    DTERR_ASSERT_NOT_NULL(config->iox_handle);
    DTERR_ASSERT_NOT_NULL(config->framer_handle);

    DTERR_C(dttasker_validate_priority_enum(config->rx_tasker_priority));

    self->config = *config;

    if (self->config.rx_tasker_priority == DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION)
        self->config.rx_tasker_priority = DTNETPORTAL_IOX_RX_TASKER_PRIORITY_DEFAULT;

    if (self->config.max_encoded_buffer_length == 0)
        self->config.max_encoded_buffer_length = 128;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_iox_activate(dtnetportal_iox_t* self DTNETPORTAL_ACTIVATE_ARGS)
{

    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dtframer_set_message_callback( // tell the framer to call us back when it finishes decoding a message
      self->config.framer_handle,
      _framer_decoded_callback,
      self));

    {
        // configure and start the rx tasker
        dttasker_config_t c = { 0 };
        c.name = "dtnetportal_iox";
        c.stack_size = 4096;
        c.priority = self->config.rx_tasker_priority;
        DTERR_C(dttasker_create(&self->rxtasker_handle, &c));
    }

    DTERR_C(dttasker_set_entry_point( // tell the rx tasker what its entry point is
      self->rxtasker_handle,
      _rx_tasker_entry_point,
      self));

    DTERR_C(dttasker_start( // start the rx task running
      self->rxtasker_handle));

    DTERR_C(dtiox_set_rx_semaphore( // semaphore to wake up the task
      self->config.iox_handle,
      self->rx_semaphore_handle));

    DTERR_C(dtiox_attach( // engage the iox hardware
      self->config.iox_handle));

    DTERR_C(dtiox_enable( // enable hardware interrupts
      self->config.iox_handle,
      true));

    // enable fan-out of incoming decoded framed messages
    self->_should_fan_out = true;

cleanup:

    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_iox_subscribe(dtnetportal_iox_t* self DTNETPORTAL_SUBSCRIBE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dtmanifold_subscribe(self->manifold, topic, recipient_self, receive_callback));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_iox_publish(dtnetportal_iox_t* self DTNETPORTAL_PUBLISH_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    int32_t encoded_length = 0;
    int32_t out_written = 0;

    // apply the framing to the entire message
    DTERR_C(dtframer_encode(self->config.framer_handle, topic, buffer, self->encoded_buffer, &encoded_length));

#ifndef dtlog_debug
    {
        char payload_hex[2 * encoded_length +     // hex representation
                         encoded_length / 4 + 2]; // with spaces
        dtbytes_compose_hex(
          (const char*)self->encoded_buffer->payload, encoded_length, (char*)payload_hex, sizeof(payload_hex));
        dtlog_debug(TAG,
          "publish %" PRId32 " bytes framed into %" PRId32 " bytes on topic \"%s\" payload_hex='%s'",
          buffer->length,
          encoded_length,
          topic,
          payload_hex);
    }
#endif

    int32_t remaining = encoded_length;
    const uint8_t* p = (const uint8_t*)self->encoded_buffer->payload;
    while (true)
    {
        // put the message on the wire
        DTERR_C(dtiox_write(self->config.iox_handle, p, remaining, &out_written));
        p += out_written;

        remaining -= out_written;
        if (remaining > 0)
        {
            dtlog_debug(TAG,
              "partial write of %d bytes, will attempt to write remaining %d bytes after sleeping",
              out_written,
              remaining);
            dtruntime_sleep_milliseconds(1);
        }
        else
        {
            break;
        }
        if (remaining <= 0)
        {
            break;
        }
    }

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "failed for topic \"%s\"", topic);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtnetportal_iox_get_info(dtnetportal_iox_t* self, dtnetportal_info_t* info)
{
    dterr_t* dterr = NULL;
    if (info == NULL)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "called with NULL info");
        goto cleanup;
    }

    memset(info, 0, sizeof(*info));

    snprintf(info->listening_origin, sizeof(info->listening_origin), "iox");
    info->listening_origin[sizeof(info->listening_origin) - 1] = '\0';

    info->internal_task_dterr = self->internal_task_dterr;

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to obtain netportal info");

    return dterr;
}

// --------------------------------------------------------------------------------------
void
dtnetportal_iox_dispose(dtnetportal_iox_t* self)
{
    if (self == NULL)
        return;

    dtlog_debug(TAG, "disposing");

    // stop doing fan-out in case any pipelined data arrive after this
    self->_should_fan_out = false;

    // stop our rx thread from handling any more data
    self->_should_rx_thread_stop = true;

    // kick the rx thread to wake it up if it's waiting
    dtsemaphore_post(self->rx_semaphore_handle);

    if (self->rxtasker_handle != NULL)
    {
        // sleep to let the task handle pending semaphore/callbacks
        int32_t wait_tries = 5;
        int32_t nap_ms = 100;
        for (int32_t i = 1; i <= wait_tries; i++)
        {
            if (i == 1)
            {
                dtlog_debug(TAG, "waiting up to %" PRId32 " ms for rx tasker to stop", wait_tries * nap_ms);
            }
            dttasker_info_t info;
            dttasker_get_info(self->rxtasker_handle, &info);

            dtlog_debug(TAG, "try %2" PRId32 ": rx tasker state is %s", i, dttasker_state_to_string(info.status));

            if (info.status != RUNNING)
            {
                break;
            }
            dtruntime_sleep_milliseconds(nap_ms);
        }
    }

    dtbuffer_dispose(self->raw_input_buffer);

    dtbuffer_dispose(self->encoded_buffer);

    dtmanifold_dispose(self->manifold);

    dtsemaphore_dispose(self->rx_semaphore_handle);

    bool is_malloced = self->_is_malloced;
    memset(self, 0, sizeof(*self));
    if (is_malloced)
    {
        free(self);
    }
}

// --------------------------------------------------------------------------------------------
// dtobject implementation
// --------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
// Copy constructor
void
dtnetportal_iox_copy(dtnetportal_iox_t* this, dtnetportal_iox_t* that)
{
    // this object does not support copying
    (void)this;
    (void)that;
}

// --------------------------------------------------------------------------------------------
// Equality check
bool
dtnetportal_iox_equals(dtnetportal_iox_t* a, dtnetportal_iox_t* b)
{
    if (a == NULL || b == NULL)
    {
        return false;
    }

    // TODO: Reconside equality semantics for dtnetportal_iox_equals backend.
    return (a->model_number == b->model_number);
}

// --------------------------------------------------------------------------------------------
const char*
dtnetportal_iox_get_class(dtnetportal_iox_t* self)
{
    return "dtnetportal_iox_t";
}

// --------------------------------------------------------------------------------------------

bool
dtnetportal_iox_is_iface(dtnetportal_iox_t* self, const char* iface_name)
{
    return strcmp(iface_name, DTNETPORTAL_IFACE_NAME) == 0 || //
           strcmp(iface_name, "dtobject_iface") == 0;
}

// --------------------------------------------------------------------------------------------
// Convert to string
void
dtnetportal_iox_to_string(dtnetportal_iox_t* self, char* buffer, size_t buffer_size)
{
    dterr_t* dterr = NULL;

    if (self == NULL || buffer == NULL || buffer_size == 0)
        return;

    char* s = NULL;
    char* t = NULL;

    s = dtstr_concat_format(s, t, "dtnetportal_iox_t: [");
    DTERR_C(dtiox_concat_format(self->config.iox_handle, s, t, &s));
    s = dtstr_concat_format(s, t, "]", t);

    // dtobject_to_string((dtobject_handle)self->config.iox_handle, buffer + strlen(buffer), buffer_size - strlen(buffer));

    strncpy(buffer, s, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
cleanup:
    dtstr_dispose(s);
    if (dterr != NULL)
    {
        dterr_t* dterr2 = dterr;
        while (dterr2->inner_err != NULL)
        {
            dterr2 = dterr2->inner_err;
        }
        snprintf(buffer, buffer_size, "dtnetportal_iox_t: <error generating string: %s>", dterr2->message);
        dterr_dispose(dterr);
    }
}
