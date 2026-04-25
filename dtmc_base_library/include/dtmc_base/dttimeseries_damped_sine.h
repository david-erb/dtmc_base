/*
 * dttimeseries_damped_sine -- Exponentially decaying sinusoidal time series implementing
 * the dttimeseries facade.
 *
 * Returns amplitude * e^(-decay * t) * sin(2π * frequency * t) for a given microsecond
 * timestamp t, where frequency is in Hz, decay is in 1/s (larger values decay faster),
 * and t is in seconds. Models resonant mechanical or electrical systems that ring down
 * after excitation. Follows the standard create lifecycle and registers its vtable for
 * model-number dispatch via the dttimeseries registry.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dttimeseries.h>

typedef struct dttimeseries_damped_sine_t dttimeseries_damped_sine_t;

extern dterr_t*
dttimeseries_damped_sine_create(dttimeseries_damped_sine_t** self_ptr);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTTIMESERIES_DECLARE_API(dttimeseries_damped_sine);
