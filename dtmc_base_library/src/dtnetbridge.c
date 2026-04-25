#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtmanifold.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtnetprofile.h>

#include <dtmc_base/dtnetbridge.h>

#define TAG "dtnetbridge"

// #define DTNETPORTAL_RECEIVE_CALLBACK_ARGS void *receiver_self, dtbuffer_t *buffer
// #define DTNETPORTAL_RECEIVE_CALLBACK_PARAMS receiver_self, buffer
// typedef dterr_t* (*dtnetportal_receive_callback_t)(DTNETPORTAL_RECEIVE_CALLBACK_ARGS);

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetportal_handle_if_netprofile(dtnetbridge_t* self, const char* topic, dtbuffer_t* buffer)
{
    dterr_t* dterr = NULL;

    if (strcmp(topic, DTMC_BASE_CONSTANTS_NETPROFILE_TOPIC) == 0)
    {
        dtnetprofile_t netprofile = { 0 };

        // TODO: Should use distributor method to unpack in dtnetportal_handle_if_netprofile.
        dtnetprofile_unpack(&netprofile, buffer->payload, 0, buffer->length);

        char sa[512];
        dtnetprofile_to_string(&netprofile, sa, (int)sizeof(sa));
        dtlog_info(TAG, "netprofile received:\n%s", sa);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetbridge_receive_north_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS)
{
    dterr_t* dterr = NULL;
    dtnetbridge_t* self = (dtnetbridge_t*)receiver_self;

    dtlog_debug(TAG, "north netportal received %d bytes on topic \"%s\" ", (buffer != NULL) ? buffer->length : 0, topic);

    DTERR_C(dtnetportal_handle_if_netprofile(self, topic, buffer));

    DTERR_C(dtnetportal_publish(self->south_netportal_handle, topic, buffer));

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to process north netportal receive");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetbridge_receive_south_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS)
{
    dterr_t* dterr = NULL;
    dtnetbridge_t* self = (dtnetbridge_t*)receiver_self;

    dtlog_debug(TAG, "south netportal received %d bytes on topic \"%s\" ", (buffer != NULL) ? buffer->length : 0, topic);

    DTERR_C(dtnetportal_handle_if_netprofile(self, topic, buffer));

    DTERR_C(dtnetportal_publish(self->north_netportal_handle, topic, buffer));

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to process south netportal receive");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetbridge_add_north_topic(dtnetbridge_t* self, const char* topic)
{
    dterr_t* dterr = NULL;
    if (self == NULL || topic == NULL)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "invalid arguments");
        goto cleanup;
    }

    DTERR_C(dtnetportal_subscribe(self->north_netportal_handle, topic, self, dtnetbridge_receive_north_callback));

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to add topic \"%s\" to netbridge", topic);
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetbridge_add_south_topic(dtnetbridge_t* self, const char* topic)
{
    dterr_t* dterr = NULL;
    if (self == NULL || topic == NULL)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "invalid arguments");
        goto cleanup;
    }

    DTERR_C(dtnetportal_subscribe(self->south_netportal_handle, topic, self, dtnetbridge_receive_south_callback));

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to add topic \"%s\" to netbridge", topic);
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetbridge_activate(dtnetbridge_t* self)
{
    dterr_t* dterr = NULL;
    if (self == NULL)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "invalid arguments");
        goto cleanup;
    }

    DTERR_C(dtnetportal_activate(self->north_netportal_handle));
    DTERR_C(dtnetportal_activate(self->south_netportal_handle));

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to activate netbridge");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
void
dtnetbridge_dispose(dtnetbridge_t* self)
{
    if (self == NULL)
        return;

    dtnetportal_dispose(self->south_netportal_handle);
    dtnetportal_dispose(self->north_netportal_handle);
    dtbufferqueue_dispose(self->south_bufferqueue_handle);
    dtbufferqueue_dispose(self->north_bufferqueue_handle);
    dtsemaphore_dispose(self->cancellation_semaphore_handle);
    dterr_dispose(self->tasker_dterrs);

    memset(self, 0, sizeof(*self));
}