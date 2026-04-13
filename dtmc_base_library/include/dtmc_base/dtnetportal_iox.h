/*
 * dtnetportal_iox -- Netportal implementation over a framed serial IOX channel.
 *
 * Adapts a dtiox byte-stream device and a dtframer codec into a full
 * dtnetportal (activate, subscribe, publish, get_info, dispose). An RX task
 * drains the IOX FIFO and delivers decoded topic-payload pairs to subscribers.
 * The rx_drain breakout function allows testing the decode path without a
 * live RTOS task scheduler.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtobject.h>

#include <dtmc_base/dtframer.h>
#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtnetportal.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dttasker.h>

#define DTNETPORTAL_IOX_RX_TASKER_PRIORITY_DEFAULT DTTASKER_PRIORITY_URGENT_MEDIUM

typedef struct dtnetportal_iox_config_t
{
    dtiox_handle iox_handle;
    dtframer_handle framer_handle;
    int max_encoded_buffer_length;
    dttasker_priority_t rx_tasker_priority;
} dtnetportal_iox_config_t;

typedef struct dtnetportal_iox_t dtnetportal_iox_t;

extern dterr_t*
dtnetportal_iox_create(dtnetportal_iox_t** self_ptr);

extern dterr_t*
dtnetportal_iox_init(dtnetportal_iox_t* self);

extern dterr_t*
dtnetportal_iox_configure(dtnetportal_iox_t* self, dtnetportal_iox_config_t* config);

// --------------------------------------------------------------------------------------
// drain the iox rx fifo
// this breakout is to facilitate testing in a dry environment when tasks aren't really available
extern dterr_t*
dtnetportal_iox_rx_drain(dtnetportal_iox_t* self);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTNETPORTAL_DECLARE_API(dtnetportal_iox);
DTOBJECT_DECLARE_API(dtnetportal_iox);
