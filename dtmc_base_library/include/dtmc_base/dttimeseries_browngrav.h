/*
 * dttimeseries_browngrav -- Brownian random-walk signal source implementing the dttimeseries facade.
 *
 * Produces a continuous double-precision value stream by advancing a gravitationally-attracted
 * Brownian walk one step per read call; the microsecond timestamp argument is not consumed.
 *
 * Provides a bounded, continuously-varying synthetic signal useful for sensor simulation and
 * test fixture generation. A configurable restoring force, noise intensity, and seed control
 * the character of the walk. An offset and scale factor -- defaulting to 0 and 1.0 -- map the
 * raw integer output to any target numeric range without external post-processing.
 *
 * Follows the standard create/configure/read lifecycle and registers its vtable for polymorphic
 * dispatch alongside other dttimeseries backends.
 *
 * cdox v1.0.2.1
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dttimeseries.h>

typedef struct dttimeseries_browngrav_t dttimeseries_browngrav_t;

extern dterr_t*
dttimeseries_browngrav_create(dttimeseries_browngrav_t** self_ptr);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTTIMESERIES_DECLARE_API(dttimeseries_browngrav);
