/*
 * dtlightsensor -- Ambient light sensor facade with model-numbered vtable dispatch.
 *
 * Defines a minimal interface for light sensors: activate the sensor, take
 * an int64 lux sample, and dispose. A global registry maps model numbers to
 * concrete vtables, enabling dummy (test) and Zephyr sensor implementations
 * to be resolved at runtime without compile-time coupling.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdint.h>

#include <dtcore/dterr.h>

// opaque handle for dispatch calls
struct dtlightsensor_handle_t;
typedef struct dtlightsensor_handle_t* dtlightsensor_handle;

// arguments
#define DTLIGHTSENSOR_SAMPLE_ARGS , int64_t* sample
#define DTLIGHTSENSOR_SAMPLE_PARAMS , sample
#define DTLIGHTSENSOR_ACTIVATE_ARGS
#define DTLIGHTSENSOR_ACTIVATE_PARAMS

// delegates
typedef dterr_t* (*dtlightsensor_sample_fn)(void* self DTLIGHTSENSOR_SAMPLE_ARGS);
typedef dterr_t* (*dtlightsensor_activate_fn)(void* self DTLIGHTSENSOR_ACTIVATE_ARGS);
typedef void (*dtlightsensor_dispose_fn)(void* self);

// virtual table type
typedef struct dtlightsensor_vt_t
{
    dtlightsensor_sample_fn sample;
    dtlightsensor_activate_fn activate;
    dtlightsensor_dispose_fn dispose;
} dtlightsensor_vt_t;

// vtable registration
extern dterr_t*
dtlightsensor_set_vtable(int32_t model_number, dtlightsensor_vt_t* vtable);
extern dterr_t*
dtlightsensor_get_vtable(int32_t model_number, dtlightsensor_vt_t** vtable);

// dispatcher
extern dterr_t*
dtlightsensor_sample(dtlightsensor_handle handle DTLIGHTSENSOR_SAMPLE_ARGS);
extern dterr_t*
dtlightsensor_activate(dtlightsensor_handle handle DTLIGHTSENSOR_ACTIVATE_ARGS);
extern void
dtlightsensor_dispose(dtlightsensor_handle handle);

// declaration macro for implementations (put in their .h)
#define DTLIGHTSENSOR_DECLARE_API(NAME)                                                                                        \
    extern dterr_t* NAME##_create(NAME##_t** sensor);                                                                          \
    extern dterr_t* NAME##_sample(NAME##_t* self DTLIGHTSENSOR_SAMPLE_ARGS);                                                   \
    extern dterr_t* NAME##_activate(NAME##_t* self DTLIGHTSENSOR_ACTIVATE_ARGS);                                               \
    extern void NAME##_dispose(NAME##_t* self);

// vtable init macro (put in .c)
#define DTLIGHTSENSOR_INIT_VTABLE(NAME)                                                                                        \
    static dtlightsensor_vt_t _sensor_vt = {                                                                                   \
        .sample = (dtlightsensor_sample_fn)NAME##_sample,                                                                      \
        .activate = (dtlightsensor_activate_fn)NAME##_activate,                                                                \
        .dispose = (dtlightsensor_dispose_fn)NAME##_dispose,                                                                   \
    };

// expected base members in all sensor structs
#define DTLIGHTSENSOR_COMMON_MEMBERS int32_t model_number;
