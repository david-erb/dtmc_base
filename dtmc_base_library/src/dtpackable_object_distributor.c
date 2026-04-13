#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpackable.h>

#include <dtmc_base/dtnetportal.h>

#include <dtmc_base/dtpackable_object_distributor.h>

#define TAG "dtpackable_object_distributor"

#define dtlog_debug(...)

// forward the declaration to the method which netportal calls when it gets a new message
static dterr_t* netportal_receive_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS);

// -----------------------------------------------------------------------------

dterr_t*
dtpackable_object_distributor_init(dtpackable_object_distributor_t* self)
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
        dtpackable_object_distributor_dispose(self);

    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtpackable_object_distributor_configure( //
  dtpackable_object_distributor_t* self,
  dtpackable_object_distributor_config_t* config)
{
    dterr_t* dterr = NULL;

    if (self == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "self is NULL");
        goto cleanup;
    }

    if (config == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "config is NULL");
        goto cleanup;
    }

    if (config->topic == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "topic is NULL");
        goto cleanup;
    }

    if (config->netportal_handle == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "netportal_handle is NULL");
        goto cleanup;
    }

    // Save the configuration for later.
    self->config = *config;

    goto cleanup;

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "%s failed", __func__);
    }
    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtpackable_object_distributor_activate(dtpackable_object_distributor_t* self)
{
    dterr_t* dterr = NULL;

    // self-subscribe to simple handler which puts into our bufferqueue
    DTERR_C(dtnetportal_subscribe(self->config.netportal_handle, self->config.topic, self, netportal_receive_callback));

    dtlog_debug(TAG, "activated for topic \"%s\"", self->config.topic);

cleanup:
    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtpackable_object_distributor_collect( //
  dtpackable_object_distributor_t* self,
  dtobject_handle* out_object_handle,
  int32_t timeout_ms)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* network_buffer = NULL;

    if (self == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "self is NULL");
        goto cleanup;
    }

    if (self->config.bufferqueue_handle == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "bufferqueue_handle was not provided in config");
        goto cleanup;
    }

    // get the single buffer that was sent
    DTERR_C(dtbufferqueue_get(self->config.bufferqueue_handle, &network_buffer, timeout_ms, NULL));
    if (network_buffer == NULL)
    {
        dterr = dterr_new(DTERR_TIMEOUT, DTERR_LOC, NULL, "timeout waiting for buffer from topic \"%s\"", self->config.topic);
        goto cleanup;
    }
    dtlog_debug(TAG, "received buffer of %" PRId32 " bytes from topic \"%s\"", network_buffer->length, self->config.topic);

    // pull model_number from the network_buffer at the current reading position
    int32_t model_number = *((int32_t*)((uint8_t*)network_buffer->payload));

    // create such an object
    dtobject_handle object_handle;

    DTERR_C(dtobject_create(model_number, &object_handle));

    // unpack it
    dtpackable_handle packable_handle = (dtpackable_handle)object_handle;
    int32_t offset = 0;
    DTERR_C(dtpackable_unpackx(packable_handle, network_buffer->payload, &offset, network_buffer->length));

    *out_object_handle = object_handle;

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(
          dterr->error_code, DTERR_LOC, dterr, "failed to collect packable object from topic \"%s\"", self->config.topic);
    }

    dtbuffer_dispose(network_buffer);

    return dterr;
}

// -----------------------------------------------------------------------------

dterr_t*
dtpackable_object_distributor_distribute( //
  dtpackable_object_distributor_t* self,
  dtobject_handle object_handle)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(object_handle);

    dtpackable_handle packable_handle = (dtpackable_handle)object_handle;
    int32_t pack_len;
    DTERR_C(dtpackable_packx_length(packable_handle, &pack_len));

    DTERR_C(dtbuffer_create(&buffer, pack_len));
    dtlog_debug(TAG, "created buffer of length %d to distribute packable object on topic \"%s\"", pack_len, self->config.topic);

    int32_t offset = 0;
    DTERR_C(dtpackable_packx(packable_handle, buffer->payload, &offset, pack_len));

    dtlog_debug(TAG, "distributing packable object of length %d on topic \"%s\"", pack_len, self->config.topic);

    // send the single buffer as is
    DTERR_C(dtnetportal_publish(self->config.netportal_handle, self->config.topic, buffer));

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(
          dterr->error_code, DTERR_LOC, dterr, "unable to distribute packable object on topic \"%s\"", self->config.topic);
    }
    dtbuffer_dispose(buffer);
    return dterr;
}

// -----------------------------------------------------------------------------
void
dtpackable_object_distributor_dispose(dtpackable_object_distributor_t* self)
{
    if (self == NULL)
        return;

    memset(self, 0, sizeof(*self));
}

// -----------------------------------------------------------------------------
// callback by netportal when a new message arrives on our topic
static dterr_t*
netportal_receive_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS)
{
    dtpackable_object_distributor_t* self = (dtpackable_object_distributor_t*)receiver_self;
    dterr_t* dterr = NULL;

    DTERR_C(dtbufferqueue_put(self->config.bufferqueue_handle, buffer, DTTIMEOUT_NOWAIT, NULL));

cleanup:

    return dterr;
}