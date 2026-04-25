/*
 * dtinterval_scheduled -- Wall-clock periodic callback timer implementing the dtinterval facade.
 *
 * Fires a registered callback at a fixed millisecond period using
 * dtruntime_now_milliseconds and dtruntime_sleep_milliseconds.
 *
 * It's named "scheduled" because it is running on task schedule, not an ISR or hardware timer.
 * Suitable for any platform that exposes a millisecond wall-clock without dedicated timer peripherals.
 *
 * Each sleep is trimmed by the time already consumed inside the callback, preventing
 * execution overhead from accumulating into the period. If the callback
 * overruns by more than one interval, missed ticks are skipped and the
 * schedule resumes on the original phase.
 *
 * cdox v1.0.2.1
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtinterval.h>
#include <dtmc_base/dtruntime.h>

typedef struct dtinterval_scheduled_config_t
{
    dtruntime_milliseconds_t interval_milliseconds; // interval between calls to the callback function
} dtinterval_scheduled_config_t;

// forward declaration to the object's private structure
typedef struct dtinterval_scheduled_t dtinterval_scheduled_t;

extern dterr_t*
dtinterval_scheduled_create(dtinterval_scheduled_t** self_ptr);

extern dterr_t*
dtinterval_scheduled_init(dtinterval_scheduled_t* self);

extern dterr_t*
dtinterval_scheduled_configure(dtinterval_scheduled_t* self, dtinterval_scheduled_config_t* config);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTINTERVAL_DECLARE_API(dtinterval_scheduled);
