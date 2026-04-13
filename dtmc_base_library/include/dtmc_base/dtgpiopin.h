/*
 * dtgpiopin -- Cross-platform GPIO pin facade with interrupt support.
 *
 * Defines a vtable-dispatched interface for a single configured GPIO pin:
 * attach an ISR callback with edge selection, enable or disable interrupt
 * delivery, read or write the logic level, and format pin state as a string.
 * A global model-number registry allows platform implementations (ESP-IDF,
 * Zephyr, PicoSDK, RPi Linux, PYNQ, Arduino) to register and be resolved
 * at runtime without recompiling call sites.
 *
 * cdox v1.0.2
 */
#pragma once
// GPIO Pin facade for cross-platform use (Pico, nRF, ESP, etc.)
// Each handle represents one configured pin with independent lifetime and behavior.

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

// -----------------------------------------------------------------------------
// Opaque handle

struct dtgpiopin_handle_t;
typedef struct dtgpiopin_handle_t* dtgpiopin_handle;

// -----------------------------------------------------------------------------
// Configuration types

typedef enum
{
    DTGPIOPIN_MODE_INPUT,
    DTGPIOPIN_MODE_OUTPUT,
    DTGPIOPIN_MODE_INOUT
} dtgpiopin_mode_t;

typedef enum
{
    DTGPIOPIN_PULL_NONE,
    DTGPIOPIN_PULL_UP,
    DTGPIOPIN_PULL_DOWN
} dtgpiopin_pull_t;

typedef enum
{
    DTGPIOPIN_DRIVE_DEFAULT,
    DTGPIOPIN_DRIVE_OPEN_DRAIN,
    DTGPIOPIN_DRIVE_WEAK,
    DTGPIOPIN_DRIVE_STRONG
} dtgpiopin_drive_t;

typedef enum
{
    DTGPIOPIN_IRQ_NONE = 0,
    DTGPIOPIN_IRQ_RISING = 1 << 0,
    DTGPIOPIN_IRQ_FALLING = 1 << 1,
} dtgpiopin_edge_t;

typedef void (*dtgpiopin_isr_fn)(dtgpiopin_edge_t edge, void* caller_context);

// -----------------------------------------------------------------------------
// Argument helpers (pattern parity with dtsemaphore.h)

#define DTGPIOPIN_ATTACH_ARGS , dtgpiopin_isr_fn cb, void *caller_context
#define DTGPIOPIN_ATTACH_PARAMS , cb, caller_context
#define DTGPIOPIN_ENABLE_ARGS , bool enable
#define DTGPIOPIN_ENABLE_PARAMS , enable
#define DTGPIOPIN_READ_ARGS , bool* out_level
#define DTGPIOPIN_READ_PARAMS , out_level
#define DTGPIOPIN_WRITE_ARGS , bool level
#define DTGPIOPIN_WRITE_PARAMS , level
#define DTGPIOPIN_CONCAT_FORMAT_ARGS , char *in_str, char *separator, char **out_str
#define DTGPIOPIN_CONCAT_FORMAT_PARAMS , in_str, separator, out_str

// -----------------------------------------------------------------------------
// Delegate (vtable) signatures

typedef dterr_t* (*dtgpiopin_attach_fn)(void* self DTGPIOPIN_ATTACH_ARGS);
typedef dterr_t* (*dtgpiopin_enable_fn)(void* self DTGPIOPIN_ENABLE_ARGS);
typedef dterr_t* (*dtgpiopin_read_fn)(void* self DTGPIOPIN_READ_ARGS);
typedef dterr_t* (*dtgpiopin_write_fn)(void* self DTGPIOPIN_WRITE_ARGS);
typedef dterr_t* (*dtgpiopin_concat_format_fn)(void* self DTGPIOPIN_CONCAT_FORMAT_ARGS);
typedef void (*dtgpiopin_dispose_fn)(void* self);

// -----------------------------------------------------------------------------
// Virtual table type

typedef struct dtgpiopin_vt_t
{
    dtgpiopin_attach_fn attach;
    dtgpiopin_enable_fn enable;
    dtgpiopin_read_fn read;
    dtgpiopin_write_fn write;
    dtgpiopin_concat_format_fn concat_format;
    dtgpiopin_dispose_fn dispose;
} dtgpiopin_vt_t;

// -----------------------------------------------------------------------------
// Vtable registry

extern dterr_t*
dtgpiopin_set_vtable(int32_t model_number, dtgpiopin_vt_t* vtable);

extern dterr_t*
dtgpiopin_get_vtable(int32_t model_number, dtgpiopin_vt_t** vtable);

// -----------------------------------------------------------------------------
// Declaration helpers

#define DTGPIOPIN_DECLARE_API_EX(NAME, T)                                                                                      \
    extern dterr_t* NAME##_attach(NAME##T self DTGPIOPIN_ATTACH_ARGS);                                                         \
    extern dterr_t* NAME##_enable(NAME##T self DTGPIOPIN_ENABLE_ARGS);                                                         \
    extern dterr_t* NAME##_read(NAME##T self DTGPIOPIN_READ_ARGS);                                                             \
    extern dterr_t* NAME##_write(NAME##T self DTGPIOPIN_WRITE_ARGS);                                                           \
    extern dterr_t* NAME##_concat_format(NAME##T self DTGPIOPIN_CONCAT_FORMAT_ARGS);                                           \
    extern void NAME##_dispose(NAME##T self);

DTGPIOPIN_DECLARE_API_EX(dtgpiopin, _handle)
#define DTGPIOPIN_DECLARE_API(NAME) DTGPIOPIN_DECLARE_API_EX(NAME, _t*)

#define DTGPIOPIN_INIT_VTABLE(NAME)                                                                                            \
    static dtgpiopin_vt_t NAME##_vt = {                                                                                        \
        .attach = (dtgpiopin_attach_fn)NAME##_attach,                                                                          \
        .enable = (dtgpiopin_enable_fn)NAME##_enable,                                                                          \
        .read = (dtgpiopin_read_fn)NAME##_read,                                                                                \
        .write = (dtgpiopin_write_fn)NAME##_write,                                                                             \
        .concat_format = (dtgpiopin_concat_format_fn)NAME##_concat_format,                                                     \
        .dispose = (dtgpiopin_dispose_fn)NAME##_dispose,                                                                       \
    };

// -----------------------------------------------------------------------------
// Common struct prefix

#define DTGPIOPIN_COMMON_MEMBERS int32_t model_number;

// -----------------------------------------------------------------------------
#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]
# dtgpiopin

Cross-platform GPIO pin facade. Each handle represents one configured GPIO pin with its own interrupt callback and lifetime.

## Mini-guide

- The handle is opaque; acquire it from an implementation-specific factory (e.g. `dtgpiopin_picosdk_create`).
- ISR callbacks (`dtgpiopin_isr_fn`) execute in interrupt context; keep them brief.
- `attach` binds an interrupt callback.
- `enable` controls interrupt delivery.
- `read` and `write` are synchronous
- Call `dispose` exactly once per handle to free its resources.
- All functions return `NULL` on success, or a `dterr_t*` describing an error.

## Interface Functions

| Function | Purpose |
|-----------|----------|
| `dtgpiopin_attach` | Attach ISR callback and configuration |
| `dtgpiopin_enable` | Enable or disable interrupts |
| `dtgpiopin_read` | Read current logic level |
| `dtgpiopin_write` | Write logic level |
| `dtgpiopin_dispose` | Free resources and detach handler |

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
