/*
 * dttimeseries_sawtooth -- Asymmetric sawtooth signal source implementing the dttimeseries facade.
 *
 * Generates a periodic sawtooth wave bounded by [min, max] with independently
 * configurable rise and fall durations. Within each period the signal ramps
 * linearly from min to max over rise_time seconds, then resets linearly from
 * max back to min over fall_time seconds; period = rise_time + fall_time.
 *
 * Stateless: each read evaluates the position within the current period from
 * the raw microsecond timestamp, so arbitrary timestamps may be queried in any
 * order without accumulated drift.
 *
 * cdox v1.0.2.1
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dttimeseries.h>

typedef struct dttimeseries_sawtooth_t dttimeseries_sawtooth_t;

extern dterr_t*
dttimeseries_sawtooth_create(dttimeseries_sawtooth_t** self_ptr);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTTIMESERIES_DECLARE_API(dttimeseries_sawtooth);
