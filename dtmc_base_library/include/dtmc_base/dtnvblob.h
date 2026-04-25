/*
 * dtnvblob -- Non-volatile binary blob storage facade with model-numbered dispatch.
 *
 * Defines a minimal interface for persistent binary storage: read and write
 * an opaque byte blob via an opaque handle. A global registry maps model
 * numbers to concrete flash/NVS backends (ESP-IDF NVS, Zephyr flash, PicoSDK
 * flash, Linux file) so higher-level modules such as dtradioconfig can access
 * persistent data without platform conditionals.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

// -----------------------------------------------------------------------------
// Opaque handle

struct dtnvblob_handle_t;
typedef struct dtnvblob_handle_t* dtnvblob_handle;

// -----------------------------------------------------------------------------
// Argument helpers (pattern parity with dtsemaphore.h)

#define DTNVBLOB_READ_ARGS , void *blob, int32_t *size
#define DTNVBLOB_READ_PARAMS , blob, size
#define DTNVBLOB_WRITE_ARGS , const void *blob, int32_t *size
#define DTNVBLOB_WRITE_PARAMS , blob, size

// -----------------------------------------------------------------------------
// Delegate (vtable) signatures

typedef dterr_t* (*dtnvblob_read_fn)(void* self DTNVBLOB_READ_ARGS);
typedef dterr_t* (*dtnvblob_write_fn)(void* self DTNVBLOB_WRITE_ARGS);
typedef void (*dtnvblob_dispose_fn)(void* self);

// -----------------------------------------------------------------------------
// Virtual table type

typedef struct dtnvblob_vt_t
{
    dtnvblob_read_fn read;
    dtnvblob_write_fn write;
    dtnvblob_dispose_fn dispose;
} dtnvblob_vt_t;

// -----------------------------------------------------------------------------
// Vtable registry

extern dterr_t*
dtnvblob_set_vtable(int32_t model_number, dtnvblob_vt_t* vtable);

extern dterr_t*
dtnvblob_get_vtable(int32_t model_number, dtnvblob_vt_t** vtable);

// -----------------------------------------------------------------------------
// Declaration helpers

#define DTNVBLOB_DECLARE_API_EX(NAME, T)                                                                                       \
    extern dterr_t* NAME##_read(NAME##T self DTNVBLOB_READ_ARGS);                                                              \
    extern dterr_t* NAME##_write(NAME##T self DTNVBLOB_WRITE_ARGS);                                                            \
    extern void NAME##_dispose(NAME##T self);

DTNVBLOB_DECLARE_API_EX(dtnvblob, _handle)
#define DTNVBLOB_DECLARE_API(NAME) DTNVBLOB_DECLARE_API_EX(NAME, _t*)

#define DTNVBLOB_INIT_VTABLE(NAME)                                                                                             \
    static dtnvblob_vt_t NAME##_vt = {                                                                                         \
        .read = (dtnvblob_read_fn)NAME##_read,                                                                                 \
        .write = (dtnvblob_write_fn)NAME##_write,                                                                              \
        .dispose = (dtnvblob_dispose_fn)NAME##_dispose,                                                                        \
    };

// -----------------------------------------------------------------------------
// Common struct prefix

#define DTNVBLOB_COMMON_MEMBERS int32_t model_number;

// -----------------------------------------------------------------------------
#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]
# dtnvblob

Cross-platform facade for access to NVBLOB. Each handle represents a blob-readable/writable persistent on the hardware.

## Mini-guide

- The handle is opaque; acquire it from an implementation-specific factory (e.g. `dtnvblob_picosdk_create`).
- `read` and `write` return the number of bytes read/written via the `size` pointer.
- No error is returned if the blob does not fit. 
- Call `dispose` exactly once per handle to free its resources.
- All functions return `NULL` on success, or a `dterr_t*` describing an error.

## Interface Functions

| Function | Purpose |
|-----------|----------|
| `dtnvblob_read` | Read persistent blob |
| `dtnvblob_write` | Write persistent blob |
| `dtnvblob_dispose` | Free resources |

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
