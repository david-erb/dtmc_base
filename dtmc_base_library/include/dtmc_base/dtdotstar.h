/*
 * dtdotstar -- APA102/DotStar RGB LED strip facade with model-numbered dispatch.
 *
 * Defines a vtable interface for driving addressable LED strips: connect
 * (initialize the bus), dither (map linear lumen values to gamma-corrected
 * RGB), transmit (push the current frame to hardware), enqueue (stage a
 * frame for deferred transmission), and a post callback for async completion
 * notification. A global registry allows platform implementations (ESP-IDF
 * SPI, Zephyr, etc.) to be substituted at initialization time.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdint.h>

#include <dtcore/dterr.h>

// Lumen structure
typedef struct
{
    int32_t R;
    int32_t G;
    int32_t B;
    int32_t Flux;
} dtdotstar_lumen_t;

// opaque handle for dispatch calls
struct dtdotstar_handle_t;
typedef struct dtdotstar_handle_t* dtdotstar_handle;

typedef void (*dtdotstar_post_cb_fn)(void* user_context);

// arguments
#define DTDOTSTAR_SET_POST_CB_ARGS , dtdotstar_post_cb_fn post_cb, void *user_context
#define DTDOTSTAR_SET_POST_CB_PARAMS , post_cb, user_context
#define DTDOTSTAR_CONNECT_ARGS
#define DTDOTSTAR_DITHER_ARGS , dtdotstar_lumen_t* lumens
#define DTDOTSTAR_DITHER_PARAMS , lumens
#define DTDOTSTAR_TRANSMIT_ARGS
#define DTDOTSTAR_TRANSMIT_PARAMS
#define DTDOTSTAR_ENQUEUE_ARGS
#define DTDOTSTAR_ENQUEUE_PARAMS

// delegates
typedef dterr_t* (*dtdotstar_set_post_cb_fn)(void* self DTDOTSTAR_SET_POST_CB_ARGS);
typedef dterr_t* (*dtdotstar_connect_fn)(void* self DTDOTSTAR_CONNECT_ARGS);
typedef dterr_t* (*dtdotstar_dither_fn)(void* self DTDOTSTAR_DITHER_ARGS);
typedef dterr_t* (*dtdotstar_transmit_fn)(void* self DTDOTSTAR_TRANSMIT_ARGS);
typedef dterr_t* (*dtdotstar_enqueue_fn)(void* self DTDOTSTAR_ENQUEUE_ARGS);
typedef void (*dtdotstar_dispose_fn)(void* self);

// virtual table type
typedef struct dtdotstar_vt_t
{
    dtdotstar_set_post_cb_fn set_post_cb;
    dtdotstar_connect_fn connect;
    dtdotstar_dither_fn dither;
    dtdotstar_transmit_fn transmit;
    dtdotstar_enqueue_fn enqueue;
    dtdotstar_dispose_fn dispose;
} dtdotstar_vt_t;

extern dterr_t*
dtdotstar_set_vtable(int32_t model_number, dtdotstar_vt_t* vtable);
extern dterr_t*
dtdotstar_get_vtable(int32_t model_number, dtdotstar_vt_t** vtable);

// declaration dispatcher or implementation
#define DTDOTSTAR_DECLARE_API_EX(NAME, T)                                                                                      \
    extern dterr_t* NAME##_set_post_cb(NAME##T self DTDOTSTAR_SET_POST_CB_ARGS);                                               \
    extern dterr_t* NAME##_connect(NAME##T self DTDOTSTAR_CONNECT_ARGS);                                                       \
    extern dterr_t* NAME##_dither(NAME##T self DTDOTSTAR_DITHER_ARGS);                                                         \
    extern dterr_t* NAME##_transmit(NAME##T self DTDOTSTAR_TRANSMIT_ARGS);                                                     \
    extern dterr_t* NAME##_enqueue(NAME##T self DTDOTSTAR_ENQUEUE_ARGS);                                                       \
    extern void NAME##_dispose(NAME##T self);

// declare dispatcher
DTDOTSTAR_DECLARE_API_EX(dtdotstar, _handle)

// declare implementation (put this in its .h file)
#define DTDOTSTAR_DECLARE_API(NAME) DTDOTSTAR_DECLARE_API_EX(NAME, _t*)

// initialize implementation vtable  (put this in its .c file)
#define DTDOTSTAR_INIT_VTABLE(NAME)                                                                                            \
    static dtdotstar_vt_t NAME##_vt = {                                                                                        \
        .set_post_cb = (dtdotstar_set_post_cb_fn)NAME##_set_post_cb,                                                           \
        .connect = (dtdotstar_connect_fn)NAME##_connect,                                                                       \
        .dither = (dtdotstar_dither_fn)NAME##_dither,                                                                          \
        .transmit = (dtdotstar_transmit_fn)NAME##_transmit,                                                                    \
        .enqueue = (dtdotstar_enqueue_fn)NAME##_enqueue,                                                                       \
        .dispose = (dtdotstar_dispose_fn)NAME##_dispose,                                                                       \
    };

// common members expected at the start of all implementation structures
#define DTDOTSTAR_COMMON_MEMBERS int32_t model_number;
