#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtframer.h>
#include <dtmc_base/dtframer_simple.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtframer_simple"

/* Callback context that can record a small number of decoded messages. */
typedef struct callback_message_t
{
    dtbuffer_t* topic;
    dtbuffer_t* payload;
} callback_message_t;

typedef struct callback_context_t
{
    callback_message_t messages[4];
    int32_t message_count;
} callback_context_t;

static void
callback_context_dispose(callback_context_t* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    for (int32_t i = 0; i < ctx->message_count; ++i)
    {
        dtbuffer_dispose(ctx->messages[i].topic);
        dtbuffer_dispose(ctx->messages[i].payload);
        ctx->messages[i].topic = NULL;
        ctx->messages[i].payload = NULL;
    }
    ctx->message_count = 0;
}

static dterr_t*
_message_callback(void* callback_context, const char* topic, dtbuffer_t* decoded_buffer)
{
    dterr_t* dterr = NULL;
    callback_context_t* ctx = (callback_context_t*)callback_context;

    DTERR_ASSERT_NOT_NULL(ctx);
    DTERR_ASSERT_NOT_NULL(topic);
    DTERR_ASSERT_NOT_NULL(decoded_buffer);

    if (ctx->message_count >= (int32_t)(sizeof(ctx->messages) / sizeof(ctx->messages[0])))
    {
        /* Defensive guard so tests notice if they forgot to bound expected messages. */
        dterr = dterr_new(DTERR_BOUNDS, DTERR_LOC, NULL, "callback_context message buffer is full");
        goto cleanup;
    }

    callback_message_t* slot = &ctx->messages[ctx->message_count];

    DTERR_C(dtbuffer_create(&slot->topic, (int32_t)strlen(topic) + 1));
    strcpy((char*)slot->topic->payload, topic);

    DTERR_C(dtbuffer_create(&slot->payload, decoded_buffer->length + 1));
    memcpy(slot->payload->payload, decoded_buffer->payload, decoded_buffer->length);
    ((char*)slot->payload->payload)[decoded_buffer->length] = '\0'; // null-terminate for convenience

    ctx->message_count++;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
// Example: basic one-shot encode/decode using the dtframer facade.

static dterr_t*
test_dtmc_base_dtframer_simple_example_basic_roundtrip(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    dtbuffer_t* source_payload = NULL;
    dtbuffer_t* encoded_buffer = NULL;
    callback_context_t callback_context = { 0 };

    const char* topic = "sensors/temperature";
    const char* payload_text = "25.5C";

    int32_t maximum_payload_length = 256;
    int32_t expected_overhead_bytes = 16; /* Plenty of room for header + topic. */

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_payload_length = maximum_payload_length;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _message_callback, &callback_context));

    DTERR_C(dtbuffer_create(&source_payload, (int32_t)strlen(payload_text)));
    memcpy(source_payload->payload, payload_text, source_payload->length);

    DTERR_C(dtbuffer_create(&encoded_buffer, maximum_payload_length + expected_overhead_bytes));

    int32_t encoded_length = 0;
    DTERR_C(dtframer_encode(handle, topic, source_payload, encoded_buffer, &encoded_length));
    DTERR_C(dtframer_decode(handle, encoded_buffer->payload, encoded_length));

    DTUNITTEST_ASSERT_INT(callback_context.message_count, ==, 1);

    {
        const char* got_topic = (const char*)callback_context.messages[0].topic->payload;
        DTUNITTEST_ASSERT_EQUAL_STRING(got_topic, topic);
    }
    {
        const char* got_payload = (const char*)callback_context.messages[0].payload->payload;
        DTUNITTEST_ASSERT_EQUAL_STRING(got_payload, payload_text);
    }

cleanup:
    callback_context_dispose(&callback_context);
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(source_payload);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Example: decode a single frame delivered in small chunks (UART/CAN style).

static dterr_t*
test_dtmc_base_dtframer_simple_example_chunked_decode(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    dtbuffer_t* payload = NULL;
    dtbuffer_t* encoded_buffer = NULL;
    callback_context_t callback_context = { 0 };

    const char* topic = "stream/chunked";
    const char* payload_text = "Hello over a noisy link";

    int32_t maximum_payload_length = 256;
    int32_t expected_overhead_bytes = 32;

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_payload_length = maximum_payload_length;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _message_callback, &callback_context));

    DTERR_C(dtbuffer_create(&payload, (int32_t)strlen(payload_text)));
    memcpy(payload->payload, payload_text, payload->length);

    DTERR_C(dtbuffer_create(&encoded_buffer, maximum_payload_length + expected_overhead_bytes));

    int32_t encoded_length = 0;
    DTERR_C(dtframer_encode(handle, topic, payload, encoded_buffer, &encoded_length));

    /* Feed the encoded bytes into the decoder in small pieces. */
    int32_t position = 0;
    const uint8_t* stream = encoded_buffer->payload;

    while (position < encoded_length)
    {
        int32_t chunk = 3; /* Arbitrary small chunk size. */
        if (position + chunk > encoded_length)
        {
            chunk = encoded_length - position;
        }

        DTERR_C(dtframer_decode(handle, (uint8_t*)(stream + position), chunk));
        position += chunk;
    }

    DTUNITTEST_ASSERT_INT(callback_context.message_count, ==, 1);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[0].topic->payload, topic);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[0].payload->payload, payload_text);

cleanup:
    callback_context_dispose(&callback_context);
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(payload);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Example: two messages back-to-back in the same bytestream.

static dterr_t*
test_dtmc_base_dtframer_simple_example_two_messages_back_to_back(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    dtbuffer_t* payload1 = NULL;
    dtbuffer_t* payload2 = NULL;
    dtbuffer_t* encoded_buffer = NULL;
    callback_context_t callback_context = { 0 };

    const char* topic1 = "topic/one";
    const char* topic2 = "topic/two";
    const char* payload_text1 = "first-message";
    const char* payload_text2 = "second-message";

    int32_t maximum_payload_length = 256;
    int32_t expected_overhead_bytes = 64;

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_payload_length = maximum_payload_length;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _message_callback, &callback_context));

    DTERR_C(dtbuffer_create(&payload1, (int32_t)strlen(payload_text1)));
    memcpy(payload1->payload, payload_text1, payload1->length);

    DTERR_C(dtbuffer_create(&payload2, (int32_t)strlen(payload_text2)));
    memcpy(payload2->payload, payload_text2, payload2->length);

    DTERR_C(dtbuffer_create(&encoded_buffer, 2 * maximum_payload_length + expected_overhead_bytes));

    /* Encode first message at the start of the buffer. */
    int32_t encoded_length1 = 0;
    DTERR_C(dtframer_encode(handle, topic1, payload1, encoded_buffer, &encoded_length1));

    /* Encode second message immediately after the first one. */
    int32_t encoded_length2 = 0;
    {
        dtbuffer_t second_output_view = *encoded_buffer;
        second_output_view.payload = (char*)encoded_buffer->payload + encoded_length1;
        second_output_view.length = encoded_buffer->length - encoded_length1;

        DTERR_C(dtframer_encode(handle, topic2, payload2, &second_output_view, &encoded_length2));
    }

    int32_t total_length = encoded_length1 + encoded_length2;
    DTERR_C(dtframer_decode(handle, encoded_buffer->payload, total_length));

    DTUNITTEST_ASSERT_INT(callback_context.message_count, ==, 2);

    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[0].topic->payload, topic1);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[0].payload->payload, payload_text1);

    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[1].topic->payload, topic2);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[1].payload->payload, payload_text2);

cleanup:
    callback_context_dispose(&callback_context);
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(payload2);
    dtbuffer_dispose(payload1);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Bounds: topic longer than configured maximum should be rejected on encode.

static dterr_t*
test_dtmc_base_dtframer_simple_01_topic_too_long_rejected(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    dtbuffer_t* payload = NULL;
    dtbuffer_t* encoded_buffer = NULL;
    callback_context_t callback_context = { 0 };

    const char* payload_text = "short payload";

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_topic_length = 4; /* Intentionally very small. */
        config.maximum_payload_length = 64;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _message_callback, &callback_context));

    DTERR_C(dtbuffer_create(&payload, (int32_t)strlen(payload_text)));
    memcpy(payload->payload, payload_text, payload->length);

    DTERR_C(dtbuffer_create(&encoded_buffer, 128));

    const char* long_topic = "this/topic/name/is/too/long";

    dterr_t* encode_error = dtframer_encode(handle, long_topic, payload, encoded_buffer, &(int32_t){ 0 });
    DTUNITTEST_ASSERT_DTERR(encode_error, DTERR_BOUNDS);

cleanup:
    callback_context_dispose(&callback_context);
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(payload);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Bounds: encoded_buffer too small for frame should be rejected.

static dterr_t*
test_dtmc_base_dtframer_simple_02_output_buffer_too_small(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    dtbuffer_t* payload = NULL;
    dtbuffer_t* encoded_buffer = NULL;
    callback_context_t callback_context = { 0 };

    const char* topic = "tiny/topic";
    const char* payload_text = "payload that will not fit";

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_topic_length = 64;
        config.maximum_payload_length = 64;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _message_callback, &callback_context));

    DTERR_C(dtbuffer_create(&payload, (int32_t)strlen(payload_text)));
    memcpy(payload->payload, payload_text, payload->length);

    /* Intentionally too short for header + topic + payload. */
    DTERR_C(dtbuffer_create(&encoded_buffer, 8));

    int32_t encoded_length = 0;
    dterr_t* encode_error = dtframer_encode(handle, topic, payload, encoded_buffer, &encoded_length);
    DTUNITTEST_ASSERT_DTERR(encode_error, DTERR_BOUNDS);

cleanup:
    callback_context_dispose(&callback_context);
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(payload);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Decode: negative length should be rejected immediately.

static dterr_t*
test_dtmc_base_dtframer_simple_03_negative_length_rejected(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    callback_context_t callback_context = { 0 };

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_payload_length = 16;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _message_callback, &callback_context));

    uint8_t dummy_byte = 0;
    dterr_t* decode_error = dtframer_decode(handle, &dummy_byte, -1);
    DTUNITTEST_ASSERT_DTERR(decode_error, DTERR_BOUNDS);

cleanup:
    callback_context_dispose(&callback_context);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Decode: version mismatch between encoder and decoder should report corruption.

static dterr_t*
test_dtmc_base_dtframer_simple_04_version_mismatch_detected(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle encode_handle = NULL;
    dtframer_handle decode_handle = NULL;
    dtbuffer_t* payload = NULL;
    dtbuffer_t* encoded_buffer = NULL;
    callback_context_t callback_context = { 0 };

    const char* topic = "version/test";
    const char* payload_text = "payload";

    int32_t maximum_payload_length = 64;
    int32_t expected_overhead_bytes = 32;

    {
        /* Encoder with protocol_version = 2. */
        dtframer_simple_t* instance_encode = NULL;
        DTERR_C(dtframer_simple_create(&instance_encode));
        encode_handle = (dtframer_handle)instance_encode;

        dtframer_simple_config_t config_encode = { 0 };
        config_encode.maximum_payload_length = maximum_payload_length;
        config_encode.protocol_version = 2;

        DTERR_C(dtframer_simple_configure(instance_encode, &config_encode));
    }

    {
        /* Decoder expecting protocol_version = 1. */
        dtframer_simple_t* instance_decode = NULL;
        DTERR_C(dtframer_simple_create(&instance_decode));
        decode_handle = (dtframer_handle)instance_decode;

        dtframer_simple_config_t config_decode = { 0 };
        config_decode.maximum_payload_length = maximum_payload_length;
        config_decode.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance_decode, &config_decode));
    }
    DTERR_C(dtframer_set_message_callback(decode_handle, _message_callback, &callback_context));

    DTERR_C(dtbuffer_create(&payload, (int32_t)strlen(payload_text)));
    memcpy(payload->payload, payload_text, payload->length);

    DTERR_C(dtbuffer_create(&encoded_buffer, maximum_payload_length + expected_overhead_bytes));

    int32_t encoded_length = 0;
    DTERR_C(dtframer_encode(encode_handle, topic, payload, encoded_buffer, &encoded_length));

    dterr_t* decode_error = dtframer_decode(decode_handle, encoded_buffer->payload, encoded_length);
    DTUNITTEST_ASSERT_DTERR(decode_error, DTERR_CORRUPT);

cleanup:
    callback_context_dispose(&callback_context);
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(payload);
    dtframer_dispose(decode_handle);
    dtframer_dispose(encode_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Decode: application callback failure should propagate back to caller.

static dterr_t*
_failing_message_callback(void* callback_context, const char* topic, dtbuffer_t* decoded_buffer)
{
    (void)callback_context;
    (void)topic;
    (void)decoded_buffer;

    /* Simulate an application-level error during message handling. */
    return dterr_new(DTERR_ASSERTION, DTERR_LOC, NULL, "failing message callback");
}

static dterr_t*
test_dtmc_base_dtframer_simple_05_callback_error_propagates(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    dtbuffer_t* payload = NULL;
    dtbuffer_t* encoded_buffer = NULL;

    const char* topic = "callback/error";
    const char* payload_text = "trigger";

    int32_t maximum_payload_length = 64;
    int32_t expected_overhead_bytes = 32;

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_payload_length = maximum_payload_length;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _failing_message_callback, NULL));

    DTERR_C(dtbuffer_create(&payload, (int32_t)strlen(payload_text)));
    memcpy(payload->payload, payload_text, payload->length);

    DTERR_C(dtbuffer_create(&encoded_buffer, maximum_payload_length + expected_overhead_bytes));

    int32_t encoded_length = 0;
    DTERR_C(dtframer_encode(handle, topic, payload, encoded_buffer, &encoded_length));

    dterr_t* decode_error = dtframer_decode(handle, encoded_buffer->payload, encoded_length);
    DTUNITTEST_ASSERT_DTERR(decode_error, DTERR_ASSERTION);

cleanup:
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(payload);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Decode: random noise bytes before a valid frame should be ignored.

static dterr_t*
test_dtmc_base_dtframer_simple_06_noise_before_magic_ignored(void)
{
    dterr_t* dterr = NULL;
    dtframer_handle handle = NULL;
    dtbuffer_t* payload = NULL;
    dtbuffer_t* encoded_buffer = NULL;
    callback_context_t callback_context = { 0 };

    const char* topic = "noise/test";
    const char* payload_text = "ok";

    int32_t maximum_payload_length = 64;
    int32_t expected_overhead_bytes = 32;

    {
        dtframer_simple_t* instance = NULL;
        DTERR_C(dtframer_simple_create(&instance));
        handle = (dtframer_handle)instance;

        dtframer_simple_config_t config = { 0 };
        config.maximum_payload_length = maximum_payload_length;
        config.protocol_version = 1;

        DTERR_C(dtframer_simple_configure(instance, &config));
    }
    DTERR_C(dtframer_set_message_callback(handle, _message_callback, &callback_context));

    DTERR_C(dtbuffer_create(&payload, (int32_t)strlen(payload_text)));
    memcpy(payload->payload, payload_text, payload->length);

    DTERR_C(dtbuffer_create(&encoded_buffer, maximum_payload_length + expected_overhead_bytes));

    int32_t encoded_length = 0;
    DTERR_C(dtframer_encode(handle, topic, payload, encoded_buffer, &encoded_length));

    /* Prepend a few noise bytes that should be skipped. */
    uint8_t noisy_stream[8 + 256] = { 0 };
    int32_t noise_length = 5;

    for (int32_t i = 0; i < noise_length; ++i)
    {
        noisy_stream[i] = (uint8_t)(0x11 + i); /* Guaranteed not to be the magic first byte (0xA5). */
    }

    memcpy(noisy_stream + noise_length, encoded_buffer->payload, encoded_length);
    int32_t total_length = noise_length + encoded_length;

    DTERR_C(dtframer_decode(handle, noisy_stream, total_length));

    DTUNITTEST_ASSERT_INT(callback_context.message_count, ==, 1);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[0].topic->payload, topic);
    DTUNITTEST_ASSERT_EQUAL_STRING((char*)callback_context.messages[0].payload->payload, payload_text);

cleanup:
    callback_context_dispose(&callback_context);
    dtbuffer_dispose(encoded_buffer);
    dtbuffer_dispose(payload);
    dtframer_dispose(handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Suite entry point

void
test_dtmc_base_dtframer_simple(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_example_basic_roundtrip);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_example_chunked_decode);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_example_two_messages_back_to_back);

    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_01_topic_too_long_rejected);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_02_output_buffer_too_small);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_03_negative_length_rejected);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_04_version_mismatch_detected);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_05_callback_error_propagates);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtframer_simple_06_noise_before_magic_ignored);
}
