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

#include <dtmc_base/dtbusywork.h>
#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtsemaphore.h>

#include <dtmc_base_benchmarks/benchmark_helpers.h>
#include <dtmc_base_benchmarks/benchmark_netportal_simplex.h>

#define TAG "dtmc_base_benchmark_netportal_simplex"

#define dtlog_debug(TAG, ...)

// the benchmark object's privates
typedef struct benchmark_t
{
    benchmark_config_t config;
    const char* topic;
    dtsemaphore_handle pong_semaphore_handle;

    dteventlogger_t eventlogger;

    int32_t received_message_count;
    bool saw_partner_sentinel;

    dtbusywork_handle busywork_handle;

    dttasker_handle tasker_handle;

} benchmark_t;

static dterr_t*
_start_server(benchmark_t* self);
static dterr_t*
_start_client(benchmark_t* self);
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

    d =
      dtstr_concat_format(d, s, "This demo is currently running as the %s side.", self->config.is_server ? "server" : "client");

    if (self->config.is_server)
    {
        d = dtstr_concat_format(d, s, "It listens for a series of incoming messages and measures the time between them.");
        d = dtstr_concat_format(d, s, "Start this before the client side.");
    }
    else
    {
        d = dtstr_concat_format(d,
          s,
          "It sends %" PRIu32 " messages of %" PRIu32 " bytes each to the server and measures the time between them.",
          self->config.message_count,
          self->config.payload_size);
        d = dtstr_concat_format(d, s, "The server side should have been started first.");
    }

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

    DTERR_C(dtcpu_sysinit());

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

    if (self->config.message_count <= 0)
    {
        self->config.message_count = 1000;
    }

    if (self->config.eventlogger_max_items <= 0)
    {
        self->config.eventlogger_max_items = 20;
    }

    self->topic = "benchmark/netportal/topic";

    DTERR_C(dteventlogger_init(&self->eventlogger, self->config.eventlogger_max_items, sizeof(dteventlogger_item1_t)));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the benchmark logic
dterr_t*
benchmark_start(benchmark_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    {
        dttasker_config_t c = { 0 };
        c.name = "benchmark";
        c.tasker_entry_point_fn = benchmark_entrypoint;
        c.tasker_entry_point_arg = self;
        c.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;
        c.core = self->config.app_core;
        c.stack_size = 8192;

        DTERR_C(dttasker_create(&self->tasker_handle, &c));
    }

    DTERR_C(dttasker_start(self->tasker_handle));

    while (true)
    {
        // wait for the benchmark task to finish
        dtruntime_sleep_milliseconds(1000);
        dttasker_info_t info = { 0 };
        DTERR_C(dttasker_get_info(self->tasker_handle, &info));
        if (info.dterr != NULL)
        {
            dterr = dterr_new(DTERR_FAIL, DTERR_LOC, info.dterr, "benchmark task reported an error");
            goto cleanup;
        }
        if (info.status == STOPPED)
        {
            dtlog_info(TAG, "benchmark task has stopped");
            break;
        }
    }

cleanup:
    dttasker_dispose(self->tasker_handle);

    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the benchmark logic
dterr_t*
benchmark_entrypoint(void* opaque_self, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    benchmark_t* self = (benchmark_t*)opaque_self;
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

    DTERR_C(dttasker_ready(tasker_handle));

    if (self->config.is_server)
    {
        DTERR_C(_start_server(self));
    }
    else
    {
        DTERR_C(_start_client(self));
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the benchmark server logic
dterr_t*
_start_server(benchmark_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // Subscribe to topic
    DTERR_C(dtnetportal_subscribe(self->config.netportal_handle, self->topic, self, _receive_callback));

    dtlog_info(TAG, "benchmark server waiting for incoming messages...");

    dtbusywork_config_t c = { 0 };
    DTERR_C(dtbusywork_create(&self->busywork_handle, &c));

    // run busy on the app core, at a low priority
    DTERR_C(dtbusywork_add_worker(self->busywork_handle, self->config.app_core, DTTASKER_PRIORITY_BACKGROUND_MEDIUM));

    // start busy workers
    DTERR_C(dtbusywork_start(self->busywork_handle));

    //  ==== the currently registered tasks ====
    {
        char* s = NULL;
        DTERR_C(dtruntime_register_tasks(&dttasker_registry_global_instance));
        DTERR_C(dttasker_registry_format_as_table(&dttasker_registry_global_instance, &s));
        dtlog_info(TAG, "Tasks:\n%s", s);
        dtstr_dispose(s);
    }

    dtruntime_milliseconds_t last_rx_at_milliseconds = 0;
    while (true)
    {
        // wait for a message from the client
        dtlog_debug(TAG, "waiting for pong %p from client...", self->pong_semaphore_handle);
        DTERR_C(dtsemaphore_wait(self->pong_semaphore_handle, DTTIMEOUT_FOREVER, NULL));

        dtlog_debug(TAG, "pong received from receive callback, checking if it's time to quit...");

        if (self->saw_partner_sentinel)
        {
            dtlog_info(TAG, "partner sentinel message received, ending benchmark");
            break;
        }

        dtruntime_milliseconds_t this_rx_at_milliseconds = dtruntime_now_milliseconds();
        // first message from the client?
        if (last_rx_at_milliseconds == 0)
        {
            // snapshot busy workers when first message comes in
            dtlog_debug(TAG, "taking initial snapshot of busywork tasks");
            DTERR_C(dtbusywork_snapshot(self->busywork_handle));
        }
        else
        {
            // record time elapsed since last received message
            int32_t rx_to_rx_milliseconds = this_rx_at_milliseconds - last_rx_at_milliseconds;
            dteventlogger_item1_t v = { //
                .timestamp = this_rx_at_milliseconds,
                .value1 = rx_to_rx_milliseconds,
                .value2 = 0
            };

            DTERR_C(dteventlogger_append(&self->eventlogger, &v));
        }
        last_rx_at_milliseconds = this_rx_at_milliseconds;

        // check internal task errors
        dtnetportal_info_t info;
        DTERR_C(dtnetportal_get_info(self->config.netportal_handle, &info));
        if (info.internal_task_dterr != NULL)
        {
            dterr = dterr_new(DTERR_FAIL, DTERR_LOC, info.internal_task_dterr, "internal netportal task reported an error");
            goto cleanup;
        }
    }

    // snapshot the cpu usage
    dtlog_debug(TAG, "taking snapshot of busywork tasks");
    DTERR_C(dtbusywork_snapshot(self->busywork_handle));

    // stop the busywork tasks
    dtlog_debug(TAG, "stopping busywork tasks");
    DTERR_C(dtbusywork_stop(self->busywork_handle));

    dtlog_info(TAG, "benchmark completed successfully after receiving %" PRId32 " messages", self->received_message_count);

    {
        char* str = NULL;

        DTERR_C(dteventlogger_printf_item1(&self->eventlogger, "Receive-to-Receive", "milliseconds", NULL, &str));

        dtlog_info(TAG, "\n%s", str);
        dtstr_dispose(str);
    }

    {
        char* str = NULL;

        DTERR_C(dtbusywork_summarize(self->busywork_handle, &str));

        dtlog_info(TAG, "\n%s", str);
        dtstr_dispose(str);
    }

cleanup:

    dtbusywork_dispose(self->busywork_handle);

    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the benchmark client logic which sends messages as fast as possible
dterr_t*
_start_client(benchmark_t* self)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // bump payload size to full 32-bit words
    self->config.payload_size = (self->config.payload_size + 3) & ~0x3;

    DTERR_C(dtbuffer_create(&buffer, self->config.payload_size));

    dtlog_info(TAG,
      "benchmark client sending %" PRId32 " messages of %" PRId32 " bytes each...",
      self->config.message_count,
      self->config.payload_size);

    int32_t message_count = 0;
    bool will_break_after_this = false;
    dtruntime_milliseconds_t last_tx_at_milliseconds = 0;
    while (true)
    {
        message_count++;
        if (message_count == self->config.message_count)
        {
            will_break_after_this = true;
        }

        // send the message
        int32_t check_word = will_break_after_this ? -1 : message_count;
        ((int32_t*)buffer->payload)[0] = check_word;
        // also put the counter in the last word of the payload for extra sanity checking on the receive side
        ((int32_t*)buffer->payload)[(buffer->length / 4) - 1] = check_word;
        DTERR_C(dtnetportal_publish(self->config.netportal_handle, self->topic, buffer));

        dtruntime_milliseconds_t this_tx_at_milliseconds = dtruntime_now_milliseconds();
        if (last_tx_at_milliseconds > 0)
        {
            // record time elapsed since last transmitted message
            int32_t tx_to_tx_milliseconds = this_tx_at_milliseconds - last_tx_at_milliseconds;
            dteventlogger_item1_t v = { //
                .timestamp = this_tx_at_milliseconds,
                .value1 = tx_to_tx_milliseconds,
                .value2 = 0
            };

            DTERR_C(dteventlogger_append(&self->eventlogger, &v));
        }
        last_tx_at_milliseconds = this_tx_at_milliseconds;

        if (will_break_after_this)
        {
            break;
        }
    }

    dtlog_info(TAG, "benchmark completed successfully after sending %" PRId32 " messages", message_count);

    {
        char* str = NULL;
        DTERR_C(dteventlogger_printf_item1(&self->eventlogger, "Send-to-Send", "milliseconds", NULL, &str));
        dtlog_info(TAG, "\n%s", str);
        dtstr_dispose(str);
    }
cleanup:
    dtbuffer_dispose(buffer);

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

    // deserialize the message
    int32_t check_word = ((int32_t*)buffer->payload)[0];

    self->received_message_count++;

    dtlog_debug(TAG,
      "received message %" PRId32 " on topic \"%s\" (%" PRId32 " bytes) check_word=0x%" PRIx32,
      self->received_message_count,
      topic,
      buffer->length,
      check_word);

    // somebody who is not us has sent the quit sentinel?
    if (check_word == -1)
    {
        self->saw_partner_sentinel = true;
        dtlog_info(
          TAG, "partner sentinel message received, will end benchmark after posting pong %p", self->pong_semaphore_handle);
        dtsemaphore_post(self->pong_semaphore_handle);
        goto cleanup;
    }
    else
    {
        // sanity check that the counter is what we expect
        if (check_word != self->received_message_count)
        {
            dterr = dterr_new(DTERR_FAIL,
              DTERR_LOC,
              NULL,
              "expected first check_word 0x%" PRIx32 " but got 0x%" PRIx32 " instead",
              self->received_message_count,
              check_word);
            goto cleanup;
        }

        // last word in the payload is also supposed to be the counter, check that too
        int32_t last_word_index = (buffer->length / 4) - 1;
        int32_t last_word = ((int32_t*)buffer->payload)[last_word_index];
        if (last_word != check_word)
        {
            dterr = dterr_new(DTERR_FAIL,
              DTERR_LOC,
              NULL,
              "expected last check_word index [%" PRId32 "] to be 0x%" PRIx32 " but got 0x%" PRIx32 " instead",
              last_word_index,
              self->received_message_count,
              last_word);
            goto cleanup;
        }
    }

    dtsemaphore_post(self->pong_semaphore_handle);

cleanup:
    dtbuffer_dispose(buffer);
    if (dterr)
    {
        dtsemaphore_post(self->pong_semaphore_handle);
    }

    return dterr;
}
