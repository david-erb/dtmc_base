#include <dtcore/dtbuffer.h>
#include <dtcore/dtbytes.h>
#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtguid.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpackable.h>
#include <dtcore/dtpackx.h>
#include <dtcore/dtpicosdk_helper.h>
#include <dtcore/dtstr.h>

#include <dtcore/dteventlogger.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtsemaphore.h>

#include <dtmc_base_benchmarks/benchmark_helpers.h>
#include <dtmc_base_benchmarks/benchmark_netportal_duplex.h>

#define TAG "dtmc_base_benchmark_netportal_duplex"
#define dtlog_debug(TAG, ...)

// message used as the pingpong ball
// data size (plus other fields in the message, plus framer overhead including topic)
// cannot exceed MTU size of the netportal, which is 240 bytes
// count of 48 gives message length of 206 bytes, for example
#define BENCHMARK_MESSAGE_DATA_COUNT 1
typedef struct benchmark_message_t
{
    int32_t counter;
    char sender_guid[DTGUID_STRING_TINY_SIZE];
    char message_guid[DTGUID_STRING_TINY_SIZE];
    int32_t data[BENCHMARK_MESSAGE_DATA_COUNT];
} benchmark_message_t;

// the benchmark object's privates
typedef struct benchmark_t
{
    benchmark_config_t config;
    const char* topic;
    dtsemaphore_handle pong_semaphore_handle;
    benchmark_message_t last_received_message;
    char sender_guid[DTGUID_STRING_TINY_SIZE];
    char last_sent_message_guid[DTGUID_STRING_TINY_SIZE];

    int32_t packed_at_milliseconds;
    int32_t received_at_milliseconds;

    dteventlogger_t eventlogger;
    int32_t eventlogger_got_items;

    char guid_sentinel[DTGUID_STRING_TINY_SIZE];

    bool saw_partner_sentinel;
} benchmark_t;

// forward declarations
static void
_generate_guid(char* output, bool will_break_after_this);
static int32_t
_message_length(benchmark_message_t* message);
static void
_message_pack(benchmark_message_t* message, dtbuffer_t* buffer);
static void
_message_unpack(benchmark_message_t* message, dtbuffer_t* buffer);
static dterr_t*
_receive_callback(void* opaque_self, const char* topic, dtbuffer_t* buffer);

// --------------------------------------------------------------------------------------
// return a string description of the benchmark (the returned string is heap allocated)
extern dterr_t*
benchmark_describe(benchmark_t* self, char** out_description)
{
    dterr_t* dterr = NULL;
    char tmp[256];
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->config.netportal_handle);
    DTERR_ASSERT_NOT_NULL(out_description);

    char* d = *out_description;
    char* s = "\n#    ";

    int32_t message_length = _message_length(&(benchmark_message_t){ 0 });
    d = dtstr_concat_format(d,
      s,
      "This benchmark runs until it has achieved %" PRId32 " message exchanges of %" PRId32 " bytes each.",
      self->config.eventlogger_max_items - 1,
      message_length);

    dtobject_to_string((dtobject_handle)self->config.netportal_handle, tmp, sizeof(tmp));
    d = dtstr_concat_format(d, s, "The netportal object in use is %s", tmp);

    *out_description = d;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Create a new instance, allocating memory for it
dterr_t*
benchmark_create(benchmark_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dtheaper_alloc_and_zero(sizeof(benchmark_t), "benchmark_t", (void**)self));

    // sentinel for the last message we send/receive
    _generate_guid((*self)->guid_sentinel, true);

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the benchmark instance with handles to implementations and settings
dterr_t*
benchmark_configure( //
  benchmark_t* self,
  benchmark_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);
    DTERR_ASSERT_NOT_NULL(config->netportal_handle);

    self->config = *config;

    if (self->config.eventlogger_max_items <= 0)
    {
        self->config.eventlogger_max_items = 21;
    }

    self->topic = "benchmark/netportal/topic";

    // get a sender id that is unique for this benchmark instance
    _generate_guid(self->sender_guid, false);

    DTERR_C(dteventlogger_init(&self->eventlogger, self->config.eventlogger_max_items, sizeof(dteventlogger_item1_t)));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the benchmark logic (blocks forever)
dterr_t*
benchmark_start(benchmark_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // print the description of the demo
    {
        char* description = NULL;
        DTERR_C(dtmc_base_benchmark_helpers_decorate_description(
          (void*)self, (benchmark_describe_fn)benchmark_describe, &description));
        dtlog_info(TAG, "%s", description);
    }

    DTERR_C(dtsemaphore_create(&self->pong_semaphore_handle, 0, 0));

    // Connect
    DTERR_C(dtnetportal_activate(self->config.netportal_handle));

    // Subscribe to topic
    DTERR_C(dtnetportal_subscribe(self->config.netportal_handle, self->topic, self, _receive_callback));

    benchmark_message_t message = { 0 };
    strcpy(message.sender_guid, self->sender_guid);

    int32_t message_length = _message_length(&message);
    dtbuffer_t buffer;
    DTERR_C(dtbuffer_wrap(&buffer, &message, message_length));

    int32_t counter = 0;
    bool first_read_occurred = false;
    bool will_break_after_this = false;
    while (true)
    {
        counter++;
        if (self->eventlogger_got_items + 1 >= self->config.eventlogger_max_items)
        {
            dtlog_info(TAG, "will send sentinel message after this");
            will_break_after_this = true;
        }

        // compose and pack the ping message we will send
        message.counter = counter;
        self->packed_at_milliseconds = (int32_t)dtruntime_now_milliseconds();
        if (!self->config.inhibit_write_until_first_read || first_read_occurred)
        {
            _generate_guid(message.message_guid, will_break_after_this);
            _message_pack(&message, &buffer);

            // send the ping
            strcpy(self->last_sent_message_guid, message.message_guid);
            DTERR_C(dtnetportal_publish(self->config.netportal_handle, self->topic, &buffer));
        }

        // wait for a response to arrive
        // wait long enough so that the publish+subscribe roundtrip can theoretically complete
        int32_t nap_milliseconds = 200;
        bool was_timeout = false;
        DTERR_C(dtsemaphore_wait(self->pong_semaphore_handle, nap_milliseconds, &was_timeout));

        if (self->saw_partner_sentinel)
        {
            dtlog_info(TAG, "partner sentinel message received, ending benchmark");
            break;
        }

        if (!was_timeout)
        {
            int32_t turnaround_milliseconds = self->received_at_milliseconds - self->packed_at_milliseconds;
            dteventlogger_item1_t v = { //
                .timestamp = (int32_t)dtruntime_now_milliseconds(),
                .value1 = turnaround_milliseconds,
                .value2 = 0
            };

            DTERR_C(dteventlogger_append(&self->eventlogger, &v));
            self->eventlogger_got_items++;
            first_read_occurred = true;
        }

        // check internal task errors
        dtnetportal_info_t info;
        DTERR_C(dtnetportal_get_info(self->config.netportal_handle, &info));
        if (info.internal_task_dterr != NULL)
        {
            dterr = dterr_new(DTERR_FAIL, DTERR_LOC, info.internal_task_dterr, "internal netportal task reported an error");
            goto cleanup;
        }

        if (will_break_after_this && first_read_occurred)
        {
            dtlog_info(TAG, "sent sentinel message, ending benchmark");
            break;
        }
    }

    dtlog_info(TAG, "benchmark completed successfully after %" PRId32 " measured turnarounds", self->eventlogger_got_items);

    {
        char* str = NULL;
        DTERR_C(dteventlogger_printf_item1(&self->eventlogger, "Turnaround", "milliseconds", NULL, &str));
        dtlog_info(TAG, "\n%s", str);
        dtstr_dispose(str);
    }
cleanup:

    return dterr;
}

// --------------------------------------------------------------------------------------
// current implementation does no cleanup on dispose, expects this resource to be valid for the entire firmware lifetime
void
benchmark_dispose(benchmark_t* self)
{
    if (self == NULL)
        return;
}

// --------------------------------------------------------------------------------------------
// send responses to what we get
static dterr_t*
_receive_callback(void* opaque_self, const char* topic, dtbuffer_t* buffer)
{
    dterr_t* dterr = NULL;
    benchmark_t* self = (benchmark_t*)opaque_self;
    DTERR_ASSERT_NOT_NULL(self);

    dtlog_debug(TAG, "received message on topic \"%s\" (%" PRId32 " bytes)", topic, buffer->length);

    // deserialize the message
    benchmark_message_t message;
    _message_unpack(&message, buffer);

    // ignore if sent by ourselves (mqtt self-sends)
    if (strcmp(message.sender_guid, self->sender_guid) == 0)
    {
        goto cleanup;
    }

    // ignore if not our message (for example multiple benchmark instances running on same broker)
    if (strcmp(message.message_guid, self->last_sent_message_guid) == 0)
    {
        // somebody who is not us has sent the quit sentinel?
        if (strcmp(message.message_guid, self->guid_sentinel) == 0)
        {
            self->saw_partner_sentinel = true;
            dtsemaphore_post(self->pong_semaphore_handle);
        }

        goto cleanup;
    }

    memcpy(&self->last_received_message, &message, sizeof(benchmark_message_t));
    self->received_at_milliseconds = dtruntime_now_milliseconds();

    dtsemaphore_post(self->pong_semaphore_handle);

cleanup:
    dtbuffer_dispose(buffer);
    if (dterr)
    {
        dtsemaphore_post(self->pong_semaphore_handle);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------------
// helpers
// --------------------------------------------------------------------------------------------

static int32_t
_message_length(benchmark_message_t* message)
{
    int32_t length = 0;
    length += dtpackx_pack_int32_length();
    length += DTGUID_STRING_TINY_SIZE;
    length += DTGUID_STRING_TINY_SIZE;
    length += dtpackx_pack_int32_length() * BENCHMARK_MESSAGE_DATA_COUNT;
    return length;
}

// --------------------------------------------------------------------------------------------
static void
_message_pack(benchmark_message_t* message, dtbuffer_t* buffer)
{
    int32_t p = 0;
    p += dtpackx_pack_int32(message->counter, buffer->payload, p, buffer->length);
    p += dtpackx_pack_string(message->sender_guid, buffer->payload, p, buffer->length);
    p += dtpackx_pack_string(message->message_guid, buffer->payload, p, buffer->length);
    for (int i = 0; i < BENCHMARK_MESSAGE_DATA_COUNT; i++)
    {
        p += dtpackx_pack_int32(message->data[i], buffer->payload, p, buffer->length);
    }
}

// --------------------------------------------------------------------------------------------
static void
_message_unpack(benchmark_message_t* message, dtbuffer_t* buffer)
{
    int32_t p = 0;
    p += dtpackx_unpack_int32(buffer->payload, p, buffer->length, &message->counter);
    {
        char* s;
        p += dtpackx_unpack_string(buffer->payload, p, buffer->length, &s);
        int32_t l = (int32_t)strlen(s) + 1;
        if (l > DTGUID_STRING_TINY_SIZE)
            l = DTGUID_STRING_TINY_SIZE;
        memcpy(message->sender_guid, s, l);
        message->sender_guid[DTGUID_STRING_TINY_SIZE - 1] = '\0';
        free((void*)s);
    }
    {
        char* s;
        p += dtpackx_unpack_string(buffer->payload, p, buffer->length, &s);
        int32_t l = (int32_t)strlen(s) + 1;
        if (l > DTGUID_STRING_TINY_SIZE)
            l = DTGUID_STRING_TINY_SIZE;
        memcpy(message->message_guid, s, l);
        message->message_guid[DTGUID_STRING_TINY_SIZE - 1] = '\0';
        free((void*)s);
    }
    for (int i = 0; i < BENCHMARK_MESSAGE_DATA_COUNT; i++)
    {
        p += dtpackx_unpack_int32(buffer->payload, p, buffer->length, &message->data[i]);
    }
}

// --------------------------------------------------------------------------------------------
static void
_generate_guid(char* guid, bool will_break_after_this)
{
    if (will_break_after_this)
    {
        strncpy(guid, "DEAD", DTGUID_STRING_TINY_SIZE);
    }
    else
    {
        int32_t random = dtcpu_random_int32();
        dtguid_t o;
        dtguid_generate_from_int32(&o, random);
        dtguid_to_string_tiny(&o, guid, DTGUID_STRING_TINY_SIZE);
    }
}
