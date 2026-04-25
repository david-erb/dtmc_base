#include <dtcore/dtbuffer.h>
#include <dtcore/dtbytes.h>
#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtguid.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtsemaphore.h>

#include <dtmc_base_demos/demo_netportal.h>

#define TAG "dtmc_base_demo_netportal"
#define dtlog_debug(TAG, ...)

// the demo object's privates
typedef struct dtmc_base_demo_netportal_t
{
    dtmc_base_demo_netportal_config_t config;

    const char* topic;

    volatile int32_t server_rx_count;
    volatile int32_t client_rx_count;

} dtmc_base_demo_netportal_t;

// forward declaration of receive callback
static dterr_t*
_receive_callback(void* opaque_self, const char* topic, dtbuffer_t* buffer);

// --------------------------------------------------------------------------------------
// return a string description of the demo (the returned string is heap allocated)
extern dterr_t*
dtmc_base_demo_netportal_describe(dtmc_base_demo_netportal_t* self, char** out_description)
{
    dterr_t* dterr = NULL;
    char tmp[256];
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->config.netportal_handle);
    DTERR_ASSERT_NOT_NULL(out_description);

    *out_description = NULL;

    char* d = NULL;
    char* s = "\n#    ";
    d = dtstr_concat_format(d, s, "Description of the demo:");

    d =
      dtstr_concat_format(d, s, "This demo is currently running as the %s side.", self->config.is_server ? "server" : "client");

    if (self->config.is_server)
    {
        d = dtstr_concat_format(d, s, "It listens for a single incoming message then replies to it.");
        d = dtstr_concat_format(d, s, "Start this before the client side.");
    }
    else
    {
        d = dtstr_concat_format(d, s, "It sends a single message to the server then waits for a reply.");
        d = dtstr_concat_format(d, s, "The server side should have been started first.");
    }

    dtobject_to_string((dtobject_handle)self->config.netportal_handle, tmp, sizeof(tmp));
    d = dtstr_concat_format(d, s, "The netportal object in use is %s", tmp);

    *out_description = d;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// dreate a new instance, allocating memory for it
dterr_t*
dtmc_base_demo_netportal_create(dtmc_base_demo_netportal_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dtheaper_alloc_and_zero(sizeof(dtmc_base_demo_netportal_t), "dtmc_base_demo_netportal_t", (void**)self));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
dtmc_base_demo_netportal_configure( //
  dtmc_base_demo_netportal_t* self,
  dtmc_base_demo_netportal_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);
    DTERR_ASSERT_NOT_NULL(config->netportal_handle);

    self->config = *config;
    self->topic = "demo/netportal/topic";

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the demo logic (blocks forever)
dterr_t*
dtmc_base_demo_netportal_start(dtmc_base_demo_netportal_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // print the description of the demo
    {
        char* description = NULL;
        DTERR_C(dtmc_base_demo_netportal_describe(self, &description));
        dtlog_info(TAG, "%s", description);
    }

    // activate the netportal
    DTERR_C(dtnetportal_activate(self->config.netportal_handle));

    // subscribe to topic and take action when the callback is invoked
    DTERR_C(dtnetportal_subscribe(self->config.netportal_handle, self->topic, self, _receive_callback));

    if (self->config.is_server)
    {
        dtlog_info(TAG, "demo server waiting for incoming message...");
    }
    else
    {
        int32_t random = dtcpu_random_int32() & 0xFFFF;
        char message[64];
        snprintf(message, sizeof(message), "random %04" PRIx32, random);

        dtlog_info(TAG, "demo client sending message: %s", message);

        dtbuffer_t buffer;
        DTERR_C(dtbuffer_wrap(&buffer, &message, strlen(message) + 1));
        DTERR_C(dtnetportal_publish(self->config.netportal_handle, self->topic, &buffer));
        dtlog_info(TAG, "demo client waiting for reply message...");
    }

    int32_t cycle_number = 0;
    while (true)
    {
        dtnetportal_info_t info;
        if (self->client_rx_count > 0 || self->server_rx_count)
        {
            dtlog_info(TAG, "exiting demo");
            break;
        }

        dtruntime_sleep_milliseconds(1000);

        DTERR_C(dtnetportal_get_info(self->config.netportal_handle, &info));

        {
            char s[256];
            dtobject_to_string((dtobject_handle)self->config.netportal_handle, s, sizeof(s));
            if (info.internal_task_dterr == NULL)
            {
                dtlog_info(TAG, "%5" PRId32 ". %s running ok", ++cycle_number, s);
            }
            else
            {
                dtlog_info(TAG, "%5" PRId32 ". %s rx error %p", ++cycle_number, s, info.internal_task_dterr);
            }
        }
    }

cleanup:

    return dterr;
}

// --------------------------------------------------------------------------------------
// this implementation does nothing, since he demo runs until externally stopped

void
dtmc_base_demo_netportal_dispose(dtmc_base_demo_netportal_t* self)
{
}

// --------------------------------------------------------------------------------------------
// handle incoming messages
static dterr_t*
_receive_callback(void* opaque_self, const char* topic, dtbuffer_t* buffer)
{
    dterr_t* dterr = NULL;
    dtmc_base_demo_netportal_t* self = (dtmc_base_demo_netportal_t*)opaque_self;
    DTERR_ASSERT_NOT_NULL(self);

    if (self->config.is_server)
    {
        // demo server already received a message, ignoring further messages
        // needed on some mqtt which self-sends messages to itself
        if (self->server_rx_count > 0)
            goto cleanup;

        self->server_rx_count++;

        int32_t random = dtcpu_random_int32() & 0xFFFF;

        // as server, reply to the client
        dtlog_info(TAG,
          "demo server received message from client: %s, replying with random %04" PRIx32,
          (const char*)buffer->payload,
          random);

        char reply_message[64];
        snprintf(reply_message,
          sizeof(reply_message),
          "got your %s, replying with random %04" PRIx32,
          (const char*)buffer->payload,
          random);

        dtbuffer_t reply_buffer;
        DTERR_C(dtbuffer_wrap(&reply_buffer, reply_message, strlen(reply_message) + 1));
        DTERR_C(dtnetportal_publish(self->config.netportal_handle, self->topic, &reply_buffer));
    }
    else
    {
        // as client, log the reply from server
        dtlog_info(TAG, "demo client received reply from server: %s", (const char*)buffer->payload);

        self->client_rx_count++;
    }

cleanup:
    // release memory from the received buffer
    dtbuffer_dispose(buffer);

    return dterr;
}
