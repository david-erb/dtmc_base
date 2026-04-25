/*
 * dtsemaphore -- Cross-platform counting semaphore facade.
 *
 * Wraps platform semaphore primitives (POSIX, FreeRTOS, Zephyr) behind a
 * common create/wait/post/dispose API with millisecond-resolution timeouts.
 * The opaque handle and standard dterr_t error return make it a building
 * block for higher-level synchronization throughout the library, including
 * dtmanifold, dthttpd, dtnetportal, and dtbufferqueue.
 *
 * cdox v1.0.2
 */
#pragma once

// Semaphore facade for cross-module use.

// See markdown documentation at the end of this file.

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dttimeout.h>

// -----------------------------------------------------------------------------
// Opaque handles

// Opaque handle used by dispatcher functions.
struct dtsemaphore_handle_t;
typedef struct dtsemaphore_handle_t* dtsemaphore_handle;

// ----------------------------------------------------------------------------
// Create semaphore.  Allocates memory and references the system resources for the semaphore.
extern dterr_t*
dtsemaphore_create(dtsemaphore_handle* self, int32_t initial_count, int32_t max_count);

// ----------------------------------------------------------------------------
// Wait (take) for semaphore.
extern dterr_t*
dtsemaphore_wait(dtsemaphore_handle self, dttimeout_millis_t timeout_milliseconds, bool* was_timeout);

// ----------------------------------------------------------------------------
// Post (give) semaphore.
extern dterr_t*
dtsemaphore_post(dtsemaphore_handle self);

// -----------------------------------------------------------------------------
// Dispose semaphore resources.  Implies release.  No error if already disposed.
extern void
dtsemaphore_dispose(dtsemaphore_handle self);

// -----------------------------------------------------------------------------
#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]
# dtsemaphore

Cross-platform semaphore facade. Provides interface for common semaphore operations.

For a reference implementation, see [dtsemaphore_dummy](../groups/dtsemaphore_dummy.md).

For more on how facades work, see the [Virtual-table pattern used across dt* modules](../guides/vtables.md).

## Mini-guide

- Typical order: acquire/create handle -> optional configuration -> `post`/`wait` as needed -> `dispose` when finished.
- Handle is opaque: treat `dtsemaphore_handle` as a black box obtained from an implementation-specific creator.
- Errors: functions return NULL on success. Otherwise a dterr_t pointer that the caller must dispose.
- Timeouts: `dtsemaphore_wait` uses relative milliseconds; pass `DTTIMEOUT_FOREVER` to block indefinitely, or `0` for a non-blocking poll.
- Disposal: call `dtsemaphore_dispose` exactly once for each owned handle; calling it on `NULL` is safe.
- Concurrency: `post` increments/releases; `wait` decrements/acquires. Semantics such as fairness or max count are implementation-defined.

## Example

```c
#include <dtmc_base/dtsemaphore.h>
#include <dtcore/dttimeout.h>
#include <dtcore/dterr.h>

// Assume some implementation-specific factory returns a dtsemaphore_handle.
extern dterr_t* mysem_create(dtsemaphore_handle* out);

dterr_t* example(void) {
    dterr_t* err = NULL;
    dtsemaphore_handle sem = NULL;

    // Create a semaphore via your chosen implementation.
    DTERR_C(mysem_create(&sem));

    // Signal availability (release/increment).
    DTERR_C(dtsemaphore_post(sem));

    // Wait for availability with a timeout (here: 1000 ms).
    DTERR_C(dtsemaphore_wait(sem, 1000));

cleanup:
    // Clean up when done.
    dtsemaphore_dispose(sem);

    // Complain if error.
    if (dterr != NULL) {
        fprintf(stderr, "Error: %s\n", dterr->message);
    }

    // Bubble up error (or dispose it if you consider it handled).
    return dterr;
}
```

## Interface Functions

These 3 functions define the dtsemaphore interface.

### dtsemaphore_post

```c
dterr_t* dtsemaphore_post(dtsemaphore_handle handle);
```
Release/increment the semaphore. 
Returns `NULL` on success, otherwise a dterr_t pointer.

### dtsemaphore_wait

```c
dterr_t* dtsemaphore_wait(dtsemaphore_handle handle, dttimeout_millis_t timeout_milliseconds);
```
Acquire/decrement the semaphore, waiting up to `timeout_milliseconds`. Use `DTTIMEOUT_FOREVER` to block indefinitely, or `0` for a non-blocking check. 
Returns `NULL` on success, otherwise a dterr_t pointer (e.g., for timeout or other errors).

### dtsemaphore_dispose

```c
void dtsemaphore_dispose(dtsemaphore_handle handle);
```
Dispose the semaphore handle and release associated resources. Safe to call with `NULL`. Should be called exactly once for each owned handle.

## Vtable Functions

These functions let the implementor register their concrete implementation vtable for their model number.

### dtsemaphore_set_vtable

```c
dterr_t* dtsemaphore_set_vtable(int32_t model_number, dtsemaphore_vt_t* vtable);
```
Publish a vtable for a given model number.  Each implementation should call this during initialization.
An impelementation can use the DTSEMAPHORE_DECLARE_API and DTSEMAPHORE_INIT_VTABLE macros to help define their vtable.
Returns `NULL` on success, otherwise a dterr_t pointer.

### dtsemaphore_get_vtable

```c
dterr_t* dtsemaphore_get_vtable(int32_t model_number, dtsemaphore_vt_t** vtable);
```
Retrieve the vtable for a given model number.




// --8<-- [end:markdown-documentation]
// clang-format on
#endif
