/*
 * dtdotstar_dummy -- Null-hardware test double for the dtdotstar LED interface.
 *
 * Implements the full dtdotstar vtable (connect, dither, transmit, enqueue,
 * set_post_cb, dispose) without any SPI or DMA dependency. Call counters
 * on the struct let unit tests verify that the correct sequence of
 * operations was invoked without requiring physical APA102/DotStar hardware.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtdotstar.h>

typedef struct dtdotstar_dummy_config_t
{
    int led_count;
} dtdotstar_dummy_config_t;

typedef struct dtdotstar_dummy_t
{
    DTDOTSTAR_COMMON_MEMBERS;

    // data members
    dtdotstar_dummy_config_t config;

    dtdotstar_post_cb_fn _post_cb;
    void* _post_cb_user_context;

    int32_t _connect_call_count;
    int32_t _dither_call_count;
    int32_t _transmit_call_count;
    int32_t _enqueue_call_count;
    int32_t _post_call_count;
    int32_t _dispose_call_count;

    bool _is_malloced;

} dtdotstar_dummy_t;

extern dterr_t*
dtdotstar_dummy_create(dtdotstar_dummy_t** self_ptr);

extern dterr_t*
dtdotstar_dummy_init(dtdotstar_dummy_t* self);

extern dterr_t*
dtdotstar_dummy_configure(dtdotstar_dummy_t* self, dtdotstar_dummy_config_t* config);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTDOTSTAR_DECLARE_API(dtdotstar_dummy);
