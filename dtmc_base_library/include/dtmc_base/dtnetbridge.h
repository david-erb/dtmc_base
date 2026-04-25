/*
 * dtnetbridge -- Two-portal message bridge with queue-buffered forwarding.
 *
 * Connects a north and south dtnetportal through paired dtbufferqueues,
 * forwarding messages received on registered topics from one side to the
 * other. A cancellation semaphore coordinates clean shutdown. Encapsulates
 * the forwarding task lifecycle so that bridging two network backends
 * (e.g., MQTT to CAN) requires only topic registration and a single
 * activate call.
 *
 * cdox v1.0.2
 */
#pragma once

#include <dtcore/dterr.h>

#include <dtcore/dterr.h>
#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtmanifold.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtsemaphore.h>

typedef struct dtnetbridge_t
{

    dtbufferqueue_handle north_bufferqueue_handle;
    dtbufferqueue_handle south_bufferqueue_handle;

    dtnetportal_handle north_netportal_handle;
    dtnetportal_handle south_netportal_handle;

    dtsemaphore_handle cancellation_semaphore_handle;

    dterr_t* tasker_dterrs;
} dtnetbridge_t;

extern dterr_t*
dtnetbridge_add_north_topic(dtnetbridge_t* self, const char* topic);

extern dterr_t*
dtnetbridge_add_south_topic(dtnetbridge_t* self, const char* topic);

extern dterr_t*
dtnetbridge_activate(dtnetbridge_t* self);

extern void
dtnetbridge_dispose(dtnetbridge_t* self);
