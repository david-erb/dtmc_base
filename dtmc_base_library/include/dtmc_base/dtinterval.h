/*
 * dtinterval -- Periodic callback timer facade with model-numbered dispatch.
 *
 * Defines a vtable interface for a repeating interval timer: bind a callback,
 * start firing, and pause. The opaque handle and global registry allow
 * concrete implementations (wall-clock scheduled, ESP-IDF timer, Zephyr
 * timer) to be selected at initialization.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdint.h>

#include <dtcore/dterr.h>

// opaque handle for dispatch calls
struct dtinterval_handle_t;
typedef struct dtinterval_handle_t* dtinterval_handle;

// callbacks
#define DTINTERVAL_CALLBACK_ARGS void *context, int *should_pause
typedef dterr_t* (*dtinterval_callback_fn)(DTINTERVAL_CALLBACK_ARGS);

// arguments
#define DTINTERVAL_SET_CALLBACK_ARGS , dtinterval_callback_fn callback, void *context
#define DTINTERVAL_SET_CALLBACK_PARAMS , callback, context
#define DTINTERVAL_START_ARGS
#define DTINTERVAL_START_PARAMS
#define DTINTERVAL_PAUSE_ARGS
#define DTINTERVAL_PAUSE_PARAMS

// delegates
typedef dterr_t* (*dtinterval_set_callback_fn)(void* self DTINTERVAL_SET_CALLBACK_ARGS);
typedef dterr_t* (*dtinterval_start_fn)(void* self DTINTERVAL_START_ARGS);
typedef dterr_t* (*dtinterval_pause_fn)(void* self DTINTERVAL_PAUSE_ARGS);
typedef void (*dtinterval_dispose_fn)(void* self);

// virtual table type
typedef struct dtinterval_vt_t
{
    dtinterval_set_callback_fn set_callback;
    dtinterval_start_fn start;
    dtinterval_pause_fn pause;
    dtinterval_dispose_fn dispose;
} dtinterval_vt_t;

extern dterr_t*
dtinterval_set_vtable(int32_t model_number, dtinterval_vt_t* vtable);
extern dterr_t*
dtinterval_get_vtable(int32_t model_number, dtinterval_vt_t** vtable);

// declaration dispatcher or implementation
#define DTINTERVAL_DECLARE_API_EX(NAME, T)                                                                                     \
    extern dterr_t* NAME##_set_callback(NAME##T self DTINTERVAL_SET_CALLBACK_ARGS);                                            \
    extern dterr_t* NAME##_start(NAME##T self DTINTERVAL_START_ARGS);                                                          \
    extern dterr_t* NAME##_pause(NAME##T self DTINTERVAL_PAUSE_ARGS);                                                          \
    extern void NAME##_dispose(NAME##T self);

// declare dispatcher
DTINTERVAL_DECLARE_API_EX(dtinterval, _handle)

// declare implementation (put this in its .h file)
#define DTINTERVAL_DECLARE_API(NAME) DTINTERVAL_DECLARE_API_EX(NAME, _t*)

// initialize implementation vtable  (put this in its .c file)
#define DTINTERVAL_INIT_VTABLE(NAME)                                                                                           \
    static dtinterval_vt_t NAME##_vt = {                                                                                       \
        .set_callback = (dtinterval_set_callback_fn)NAME##_set_callback,                                                       \
        .start = (dtinterval_start_fn)NAME##_start,                                                                            \
        .pause = (dtinterval_pause_fn)NAME##_pause,                                                                            \
        .dispose = (dtinterval_dispose_fn)NAME##_dispose,                                                                      \
    };

// common members expected at the start of all implementation structures
#define DTINTERVAL_COMMON_MEMBERS int32_t model_number;
