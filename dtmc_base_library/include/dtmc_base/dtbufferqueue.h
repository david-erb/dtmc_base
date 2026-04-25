/*
 * dtbufferqueue -- Thread-safe FIFO queue for dtbuffer_t pointers.
 *
 * Provides bounded-capacity put and get operations with millisecond timeouts
 * and optional overwrite-on-full behavior. The opaque handle hides the
 * underlying synchronization mechanism, allowing the same queue API to work
 * across RTOS and POSIX environments. Used throughout the library to decouple
 * producers from consumers across task boundaries.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dttimeout.h>

// opaque handle for dispatch calls
struct dtbufferqueue_handle_t;
typedef struct dtbufferqueue_handle_t* dtbufferqueue_handle;

// arguments
#define DTBUFFERQUEUE_PUT_ARGS , dtbuffer_t *buffer, dttimeout_millis_t timeout_millis, bool *was_timeout
#define DTBUFFERQUEUE_PUT_PARAMS , buffer, timeout_millis, was_timeout
#define DTBUFFERQUEUE_GET_ARGS , dtbuffer_t **buffer, dttimeout_millis_t timeout_millis, bool *was_timeout
#define DTBUFFERQUEUE_GET_PARAMS , buffer, timeout_millis, was_timeout

extern dterr_t*
dtbufferqueue_create(dtbufferqueue_handle* out_handle, int32_t max_capacity, bool should_overwrite);
extern dterr_t*
dtbufferqueue_put(dtbufferqueue_handle self DTBUFFERQUEUE_PUT_ARGS);
extern dterr_t*
dtbufferqueue_get(dtbufferqueue_handle self DTBUFFERQUEUE_GET_ARGS);
extern void
dtbufferqueue_dispose(dtbufferqueue_handle self);

#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]

# dtbufferqueue

Coming soon...

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
