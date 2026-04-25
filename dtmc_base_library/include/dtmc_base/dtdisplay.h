/*
 * dtdisplay -- Display device facade with model-numbered vtable dispatch.
 *
 * Defines a uniform interface for blitting raster images to a display,
 * attaching and detaching from hardware resources, and creating raster
 * buffers that match the display's pixel format. A global registry maps
 * integer model numbers to concrete vtables, supporting SDL2, LVGL, and
 * hardware LCD backends (e.g., ILI9341) without changes at call sites.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdint.h>

#include <dtcore/dtbuffer.h>

#include <dtcore/dterr.h>
#include <dtcore/dtobject.h>

#include <dtcore/dtraster.h>

// opaque handle for dispatch calls
struct dtdisplay_handle_t;
typedef struct dtdisplay_handle_t* dtdisplay_handle;

// arguments
#define DTDISPLAY_BLIT_ARGS , dtraster_handle raster_handle, int32_t x, int32_t y
#define DTDISPLAY_ATTACH_ARGS
#define DTDISPLAY_DETACH_ARGS
#define DTDISPLAY_CREATE_COMPATIBLE_RASTER_ARGS , dtraster_handle *out_raster_handle, int32_t w, int32_t h

#define DTDISPLAY_BLIT_PARAMS , raster_handle, x, y
#define DTDISPLAY_ATTACH_PARAMS
#define DTDISPLAY_DETACH_PARAMS
#define DTDISPLAY_CREATE_COMPATIBLE_RASTER_PARAMS , out_raster_handle, w, h

// delegates
typedef dterr_t* (*dtdisplay_blit_fn)(void* self DTDISPLAY_BLIT_ARGS);
typedef void (*dtdisplay_dispose_fn)(void* self);
typedef dterr_t* (*dtdisplay_attach_fn)(void* self DTDISPLAY_ATTACH_ARGS);
typedef dterr_t* (*dtdisplay_detach_fn)(void* self DTDISPLAY_DETACH_ARGS);
typedef dterr_t* (*dtdisplay_create_compatible_raster_fn)(void* self DTDISPLAY_CREATE_COMPATIBLE_RASTER_ARGS);

// virtual table type
typedef struct dtdisplay_vt_t
{
    dtdisplay_blit_fn blit;
    dtdisplay_dispose_fn dispose;
    dtdisplay_attach_fn attach;
    dtdisplay_detach_fn detach;
    dtdisplay_create_compatible_raster_fn create_compatible_raster;
} dtdisplay_vt_t;

// vtable registration
extern dterr_t*
dtdisplay_set_vtable(int32_t model_number, dtdisplay_vt_t* vtable);

extern dterr_t*
dtdisplay_get_vtable(int32_t model_number, dtdisplay_vt_t** vtable);

// declaration dispatcher or implementation
#define DTDISPLAY_DECLARE_API_EX(NAME, T)                                                                                      \
    extern dterr_t* NAME##_blit(NAME##T self DTDISPLAY_BLIT_ARGS);                                                             \
    extern void NAME##_dispose(NAME##T self);                                                                                  \
    extern dterr_t* NAME##_attach(NAME##T self DTDISPLAY_ATTACH_ARGS);                                                         \
    extern dterr_t* NAME##_detach(NAME##T self DTDISPLAY_DETACH_ARGS);                                                         \
    extern dterr_t* NAME##_create_compatible_raster(NAME##T self DTDISPLAY_CREATE_COMPATIBLE_RASTER_ARGS);

// declare dispatcher
DTDISPLAY_DECLARE_API_EX(dtdisplay, _handle)

// declare implementation (put this in its .h file)
#define DTDISPLAY_DECLARE_API(NAME) DTDISPLAY_DECLARE_API_EX(NAME, _t*)

// initialize implementation vtable (put this in its .c file)
#define DTDISPLAY_INIT_VTABLE(NAME)                                                                                            \
    static dtdisplay_vt_t NAME##_display_vt = {                                                                                \
        .blit = (dtdisplay_blit_fn)NAME##_blit,                                                                                \
        .dispose = (dtdisplay_dispose_fn)NAME##_dispose,                                                                       \
        .attach = (dtdisplay_attach_fn)NAME##_attach,                                                                          \
        .detach = (dtdisplay_detach_fn)NAME##_detach,                                                                          \
        .create_compatible_raster = (dtdisplay_create_compatible_raster_fn)NAME##_create_compatible_raster,                    \
    };

// common members expected at the start of all implementation structures
#define DTDISPLAY_COMMON_MEMBERS int32_t model_number;
