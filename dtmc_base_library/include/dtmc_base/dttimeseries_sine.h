/*
 * dttimeseries_sine -- Sinusoidal signal source implementing the dttimeseries facade.
 *
 * Evaluates offset + amplitude * sin(2*pi * frequency * t) for a microsecond
 * timestamp t converted to seconds. Frequency is in Hz; all three parameters
 * are double-precision and set at configuration time.
 *
 * Provides a stateless, analytically exact periodic signal for test fixtures and
 * signal-processing pipelines. Each read is a direct formula evaluation with no
 * accumulated state, so arbitrary timestamps can be queried in any order without
 * drift.
 *
 * Follows the standard create/configure/read lifecycle and registers its
 * vtable for polymorphic dispatch alongside other dttimeseries backends.
 *
 * cdox v1.0.2.1
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dttimeseries.h>

typedef struct dttimeseries_sine_t dttimeseries_sine_t;

extern dterr_t*
dttimeseries_sine_create(dttimeseries_sine_t** self_ptr);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTTIMESERIES_DECLARE_API(dttimeseries_sine);
