/*
 * dttimeseries -- Time-series data source facade with model-numbered vtable dispatch.
 *
 * Defines a minimal interface for any source that returns a double-precision
 * value keyed by a microsecond timestamp: configure (via a key-value list),
 * read (at a given time), to_string, and dispose. The vtable registry allows
 * multiple backend models (steady-state, interpolating, recorded) to be
 * selected at runtime and composed into larger signal-processing pipelines.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtkvp.h>

#include <dtmc_base/dtcpu.h>

// opaque handle for dispatch calls
struct dttimeseries_handle_t;
typedef struct dttimeseries_handle_t* dttimeseries_handle;

// arguments
#define DTTIMESERIES_CONFIGURE_ARGS , dtkvp_list_t* kvp_list
#define DTTIMESERIES_CONFIGURE_PARAMS , kvp_list
#define DTTIMESERIES_READ_ARGS , dtcpu_microseconds_t microseconds, double *value
#define DTTIMESERIES_READ_PARAMS , microseconds, value
#define DTTIMESERIES_TO_STRING_ARGS , char *out_string, int32_t out_string_capacity
#define DTTIMESERIES_TO_STRING_PARAMS , out_string, out_string_capacity

// delegates
typedef dterr_t* (*dttimeseries_configure_fn)(void* self DTTIMESERIES_CONFIGURE_ARGS);
typedef dterr_t* (*dttimeseries_read_fn)(void* self DTTIMESERIES_READ_ARGS);
typedef dterr_t* (*dttimeseries_to_string_fn)(void* self DTTIMESERIES_TO_STRING_ARGS);
typedef void (*dttimeseries_dispose_fn)(void* self);

// virtual table type
typedef struct dttimeseries_vt_t
{
    dttimeseries_configure_fn configure;
    dttimeseries_read_fn read;
    dttimeseries_to_string_fn to_string;
    dttimeseries_dispose_fn dispose;
} dttimeseries_vt_t;

// vtable registration
extern dterr_t*
dttimeseries_set_vtable(int32_t model_number, dttimeseries_vt_t* vtable);

extern dterr_t*
dttimeseries_get_vtable(int32_t model_number, dttimeseries_vt_t** vtable);

// declaration dispatcher or implementation
#define DTTIMESERIES_DECLARE_API_EX(NAME, T)                                                                                   \
    extern dterr_t* NAME##_configure(NAME##T self DTTIMESERIES_CONFIGURE_ARGS);                                                \
    extern dterr_t* NAME##_read(NAME##T self DTTIMESERIES_READ_ARGS);                                                          \
    extern dterr_t* NAME##_to_string(NAME##T self DTTIMESERIES_TO_STRING_ARGS);                                                \
    extern void NAME##_dispose(NAME##T self);

// declare dispatcher
DTTIMESERIES_DECLARE_API_EX(dttimeseries, _handle)

// declare implementation (put this in its .h file)
#define DTTIMESERIES_DECLARE_API(NAME) DTTIMESERIES_DECLARE_API_EX(NAME, _t*)

// initialize implementation vtable (put this in its .c file)
#define DTTIMESERIES_INIT_VTABLE(NAME)                                                                                         \
    static dttimeseries_vt_t NAME##_vt = {                                                                                     \
        .configure = (dttimeseries_configure_fn)NAME##_configure,                                                              \
        .read = (dttimeseries_read_fn)NAME##_read,                                                                             \
        .to_string = (dttimeseries_to_string_fn)NAME##_to_string,                                                              \
        .dispose = (dttimeseries_dispose_fn)NAME##_dispose,                                                                    \
    };

// common members expected at the start of all implementation structures
#define DTTIMESERIES_COMMON_MEMBERS int32_t model_number;