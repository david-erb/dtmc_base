/*
 * dtlock -- Platform-portable mutual exclusion lock facade.
 *
 * Wraps a system mutex behind an opaque handle with create, acquire, release,
 * and dispose operations. The thin interface hides POSIX pthread_mutex,
 * FreeRTOS mutex, and other platform-specific primitives behind a common
 * API, keeping higher-level modules free of RTOS or OS headers.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>

#include <dtcore/dterr.h>

// -----------------------------------------------------------------------------
// Opaque handle may have platform-specific members.
struct dtlock_handle_t;
typedef struct dtlock_handle_t* dtlock_handle;

// ----------------------------------------------------------------------------
// Create lock.  Allocates memory and references the system resources for the lock.
extern dterr_t*
dtlock_create(dtlock_handle* self);

// ----------------------------------------------------------------------------
// Acquire lock owned by thread.
extern dterr_t*
dtlock_acquire(dtlock_handle self);

// ----------------------------------------------------------------------------
// Release lock owned by thread.
extern dterr_t*
dtlock_release(dtlock_handle self);

// -----------------------------------------------------------------------------
// Dispose lock resources.  Implies release.  No error if already disposed.
extern void
dtlock_dispose(dtlock_handle self);
