/*
 * dtlightsensor_dummy -- Null-hardware test double for the dtlightsensor interface.
 *
 * Implements the dtlightsensor vtable (activate, sample, dispose) without
 * physical sensor hardware. Invocation counters on the struct allow unit
 * tests to confirm that the expected call sequence occurred, using the same
 * create/init/configure lifecycle as production sensor implementations.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtlightsensor.h>

typedef struct dtlightsensor_dummy_config_t
{
    int placeholder;
} dtlightsensor_dummy_config_t;

typedef struct dtlightsensor_dummy_t
{
    DTLIGHTSENSOR_COMMON_MEMBERS;

    dtlightsensor_dummy_config_t config;

    // call counts for testing
    int _create_call_count;
    int _init_call_count;
    int _configure_call_count;
    int _activate_call_count;
    int _sample_call_count;

    bool _is_malloced;

} dtlightsensor_dummy_t;

extern dterr_t*
dtlightsensor_dummy_create(dtlightsensor_dummy_t** self_ptr);

extern dterr_t*
dtlightsensor_dummy_init(dtlightsensor_dummy_t* self);

extern dterr_t*
dtlightsensor_dummy_configure(dtlightsensor_dummy_t* self, dtlightsensor_dummy_config_t* config);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTLIGHTSENSOR_DECLARE_API(dtlightsensor_dummy);
