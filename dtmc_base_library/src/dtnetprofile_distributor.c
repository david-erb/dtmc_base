#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dttimeout.h>
#include <dtmc_base/dtbufferqueue.h>

#include <dtmc_base/dtnetportal.h>

#include <dtmc_base/dtnetprofile_distributor.h>

#define TAG "dtnetprofile_distributor"
#define dtlog_debug(TAG, ...)

#define TOPIC DTMC_BASE_CONSTANTS_NETPROFILE_TOPIC

// forward the declaration to the method which netportal calls when it gets a new message
static dterr_t* netportal_receive_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS);

// -----------------------------------------------------------------------------
// distributor init function
dterr_t*
dtnetprofile_distributor_init(dtnetprofile_distributor_t* self)
{
    dterr_t* dterr = NULL;

    if (self == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "self is NULL");
        goto cleanup;
    }

    memset(self, 0, sizeof(*self));

cleanup:

    if (dterr)
        dtnetprofile_distributor_dispose(self);

    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtnetprofile_distributor_configure( //
  dtnetprofile_distributor_t* self,
  dtnetprofile_distributor_config_t* config)
{
    dterr_t* dterr = NULL;

    if (self == NULL)
    {
        return dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "self is NULL");
    }

    if (config == NULL)
    {
        return dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "config is NULL");
    }

    if (config->netportal_handle == NULL)
    {
        return dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "netportal_handle is NULL");
    }

    if (config->bufferqueue_handle == NULL)
    {
        return dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "bufferqueue_handle is NULL");
    }

    // Save the configuration for later.
    self->config = *config;

    goto cleanup;

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to configure dtnetprofile_distributor");
    }

    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtnetprofile_distributor_activate(dtnetprofile_distributor_t* self)
{
    dterr_t* dterr = NULL;

    // self-subscribe to simple handler which puts into our bufferqueue
    DTERR_C(dtnetportal_subscribe(self->config.netportal_handle, TOPIC, self, netportal_receive_callback));

    dtlog_debug(TAG, "activated for topic \"%s\"", TOPIC);

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "%s failed", __func__);
    }

    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtnetprofile_distributor_collect( //
  dtnetprofile_distributor_t* self,
  dtnetprofile_t* netprofile)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;

    // Receive (blocking) into a newly allocated buffer.
    DTERR_C(dtbufferqueue_get(self->config.bufferqueue_handle, &buffer, DTTIMEOUT_FOREVER, NULL));

    memset(netprofile, 0, sizeof(*netprofile));
    dtnetprofile_unpack(netprofile, buffer->payload, 0, buffer->length);

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "%s failed", __func__);
    }

    // Free the buffer that was allocated by the netportal to hold the raw message.
    dtbuffer_dispose(buffer);

    return dterr;
}

// -----------------------------------------------------------------------------

dterr_t*
dtnetprofile_distributor_distribute( //
  dtnetprofile_distributor_t* self,
  dtnetprofile_t* netprofile)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(netprofile);

    int buffer_size = 0;
    buffer_size += dtnetprofile_pack_length(netprofile);

    DTERR_C(dtbuffer_create(&buffer, buffer_size));

    int p = 0;
    p += dtnetprofile_pack(netprofile, buffer->payload, p, buffer_size);

    dtlog_debug(TAG, "distributing netprofile of %d bytes on topic \"%s\"", p, TOPIC);
    DTERR_C(dtnetportal_publish(self->config.netportal_handle, TOPIC, buffer));

cleanup:
    // Free the buffer that was allocated by the netportal to hold the raw message.
    dtbuffer_dispose(buffer);

    if (dterr != NULL)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s failed", __func__);
    }

    return dterr;
}

// -----------------------------------------------------------------------------
void
dtnetprofile_distributor_dispose(dtnetprofile_distributor_t* self)
{
    if (self == NULL)
        return;
    memset(self, 0, sizeof(*self));
}

// -----------------------------------------------------------------------------
// callback by netportal when a new message arrives on our topic
// we just put in on our queue which the collect method is listening for
static dterr_t*
netportal_receive_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS)
{
    dtnetprofile_distributor_t* self = (dtnetprofile_distributor_t*)receiver_self;
    dterr_t* dterr = NULL;

    // TODO: Consider how to handle in ability to enqueue an arriving buffer in any netportal_receive_callback.
    DTERR_C(dtbufferqueue_put(self->config.bufferqueue_handle, buffer, DTTIMEOUT_NOWAIT, NULL));

cleanup:

    return dterr;
}