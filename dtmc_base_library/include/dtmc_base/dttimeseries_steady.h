/*
 * dttimeseries_steady -- Constant-value time series implementing the dttimeseries facade.
 *
 * Returns a fixed double value for any timestamp query, providing a simple
 * baseline or placeholder in signal-processing pipelines that consume
 * dttimeseries handles. Follows the standard create lifecycle and registers
 * its vtable for model-number dispatch via the dttimeseries registry.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dttimeseries.h>

typedef struct dttimeseries_steady_t dttimeseries_steady_t;

extern dterr_t*
dttimeseries_steady_create(dttimeseries_steady_t** self_ptr);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTTIMESERIES_DECLARE_API(dttimeseries_steady);
