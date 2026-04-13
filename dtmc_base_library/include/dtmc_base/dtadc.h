/*
 * dtadc -- Analog-to-digital converter facade with model-numbered dispatch.
 *
 * Defines a vtable interface for ADC devices: activate (with a per-scan
 * callback), deactivate, status query, and string formatting. A global
 * registry maps integer model numbers to concrete vtables, allowing
 * platform-specific ADC implementations (ESP-IDF one-shot, Zephyr SAADC,
 * etc.) to be swapped without changing call sites. Companion macros
 * DTADC_DECLARE_API and DTADC_INIT_VTABLE reduce boilerplate when wiring
 * in a new implementation.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtpackx.h>

#include <dtmc_base/dtadc_scan.h>

// opaque handle for dispatch calls
struct dtadc_handle_t;
typedef struct dtadc_handle_t* dtadc_handle;

// state of the process, returned in get_status
typedef enum dtadc_state_t
{
    DTADC_STATE_IDLE,
    DTADC_STATE_STARTING,
    DTADC_STATE_ACTIVE,
    DTADC_STATE_STOPPED,
    DTADC_STATE_ERROR,
} dtadc_state_t;

// status returned by get_status
typedef struct dtadc_status_t
{
    dterr_t* dterr;
    dtadc_state_t state;
} dtadc_status_t;

// callback when a scan is ready
// scans array is only valid for the duration of the callback
#define DTADC_SCAN_FN_ARGS , dtadc_scan_t* scan
#define DTADC_SCAN_FN_PARAMS , scan
typedef dterr_t* (*dtadc_scan_callback_fn)(void* self DTADC_SCAN_FN_ARGS);

// arguments
#define DTADC_ACTIVATE_ARGS , void *scan_callback_context, dtadc_scan_callback_fn scan_callback_fn
#define DTADC_ACTIVATE_PARAMS , scan_callback_context, scan_callback_fn
#define DTADC_DEACTIVATE_ARGS
#define DTADC_DEACTIVATE_PARAMS
#define DTADC_GET_STATUS_ARGS , dtadc_status_t* status
#define DTADC_GET_STATUS_PARAMS , status
#define DTADC_TO_STRING_ARGS , char *out_string, int32_t out_string_size
#define DTADC_TO_STRING_PARAMS , out_string, out_string_size

// delegates
typedef dterr_t* (*dtadc_activate_fn)(void* self DTADC_ACTIVATE_ARGS);
typedef dterr_t* (*dtadc_deactivate_fn)(void* self DTADC_DEACTIVATE_ARGS);
typedef dterr_t* (*dtadc_get_status_fn)(void* self DTADC_GET_STATUS_ARGS);
typedef dterr_t* (*dtadc_to_string_fn)(void* self DTADC_TO_STRING_ARGS);
typedef void (*dtadc_dispose_fn)(void* self);

// virtual table type
typedef struct dtadc_vt_t
{
    dtadc_activate_fn activate;
    dtadc_deactivate_fn deactivate;
    dtadc_get_status_fn get_status;
    dtadc_to_string_fn to_string;
    dtadc_dispose_fn dispose;
} dtadc_vt_t;

// vtable registration
extern dterr_t*
dtadc_set_vtable(int32_t model_number, dtadc_vt_t* vtable);
extern dterr_t*
dtadc_get_vtable(int32_t model_number, dtadc_vt_t** vtable);

// dispatcher
extern dterr_t*
dtadc_activate(dtadc_handle handle DTADC_ACTIVATE_ARGS);
extern dterr_t*
dtadc_deactivate(dtadc_handle handle DTADC_DEACTIVATE_ARGS);
extern dterr_t*
dtadc_get_status(dtadc_handle handle DTADC_GET_STATUS_ARGS);
extern dterr_t*
dtadc_to_string(dtadc_handle handle DTADC_TO_STRING_ARGS);
extern void
dtadc_dispose(dtadc_handle handle);

// declaration macro for implementations (put in their .h)
#define DTADC_DECLARE_API(NAME)                                                                                                \
    extern dterr_t* NAME##_activate(NAME##_t* self DTADC_ACTIVATE_ARGS);                                                       \
    extern dterr_t* NAME##_deactivate(NAME##_t* self DTADC_DEACTIVATE_ARGS);                                                   \
    extern dterr_t* NAME##_get_status(NAME##_t* self DTADC_GET_STATUS_ARGS);                                                   \
    extern dterr_t* NAME##_to_string(NAME##_t* self DTADC_TO_STRING_ARGS);                                                     \
    extern void NAME##_dispose(NAME##_t* self);

// vtable init macro (put in .c)
#define DTADC_INIT_VTABLE(NAME)                                                                                                \
    static dtadc_vt_t NAME##_vt = {                                                                                            \
        .activate = (dtadc_activate_fn)NAME##_activate,                                                                        \
        .deactivate = (dtadc_deactivate_fn)NAME##_deactivate,                                                                  \
        .get_status = (dtadc_get_status_fn)NAME##_get_status,                                                                  \
        .to_string = (dtadc_to_string_fn)NAME##_to_string,                                                                     \
        .dispose = (dtadc_dispose_fn)NAME##_dispose,                                                                           \
    };

// expected base members in all adc structs
#define DTADC_COMMON_MEMBERS int32_t model_number;
