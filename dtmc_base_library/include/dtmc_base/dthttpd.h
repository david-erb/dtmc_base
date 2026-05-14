/*
 * dthttpd -- HTTP server facade with vtable dispatch and POST callback model.
 *
 * Defines a uniform interface for running an embedded HTTP server: loop
 * (accept connections until stopped), stop (signal shutdown), join (wait
 * for clean exit with timeout), and dispose. A typed POST callback delivers
 * request path, payload, and response-buffer ownership to the application
 * without binding it to a specific server library or network stack.
 *
 * cdox v1.0.2
 */
#pragma once
// HTTP server facade

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dttimeout.h>

#include <dtmc_base/dtlock.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dttasker.h>

#define DTHTTPD_IFACE_NAME "dthttpd_iface"

#define DTHTTPD_DEFAULT_MAX_CONCURRENT_CONNECTIONS (10)
#define DTHTTPD_MAX_REQUEST_BYTES (1024 * 1024)
#define DTHTTPD_MAX_HEADER_BYTES (16 * 1024)
#define DTHTTPD_MAX_PATH_BYTES (1024)
#define DTHTTPD_MAX_METHOD_BYTES (16)
#define DTHTTPD_MAX_CONTENT_TYPE_BYTES (128)

// ------------------------------------------------------------------------------
// The dthttpd facade.
// A dthttpd provides a generic interface for running an HTTP server.
// Concrete implementations may use sockets, RTOS networking stacks, or other
// platform-specific mechanisms, but expose a common API for configuration,
// loop, stop, join, and disposal.
//
// dthttpd_handle: Opaque handle representing an HTTP server instance.
// dthttpd_loop: Loops listening and accepting connections.
// dthttpd_stop: Requests that the server and its child tasks stop.
// dthttpd_join: Waits for the server to finish stopping.
// dthttpd_dispose: Disposes of the server instance and releases resources.

// --------------------------------------------------------------------------------------
// forward declarations

struct dthttpd_handle_t;
typedef struct dthttpd_handle_t* dthttpd_handle;

// --------------------------------------------------------------------------------------
// POST callback
//
// Ownership model:
// - payload is owned by the server; the pointer is valid only for the duration
//   of the callback. payload->length is 0 when the request body is empty.
// - *out_response must be allocated via dtbuffer_create() if non-NULL.
// - ownership of *out_response transfers to the server, which disposes it.

typedef dterr_t* (*dthttpd_post_callback_t)(void* callback_context,
  const char* request_path,
  const dtbuffer_t* payload,
  dtbuffer_t** out_response,
  const char** out_content_type,
  int32_t* out_status_code);

// -----------------------------------------------------------------------------
// API argument helpers

#define DTHTTPD_LOOP_ARGS
#define DTHTTPD_LOOP_PARAMS

#define DTHTTPD_STOP_ARGS
#define DTHTTPD_STOP_PARAMS

#define DTHTTPD_JOIN_ARGS , dttimeout_millis_t timeout_millis, bool *was_timeout
#define DTHTTPD_JOIN_PARAMS , timeout_millis, was_timeout

#define DTHTTPD_SET_CALLBACK_ARGS , dthttpd_post_callback_t callback, void* context
#define DTHTTPD_SET_CALLBACK_PARAMS , callback, context

#define DTHTTPD_CONCAT_FORMAT_ARGS , char *in_str, char *separator, char **out_str
#define DTHTTPD_CONCAT_FORMAT_PARAMS , in_str, separator, out_str

// -----------------------------------------------------------------------------
// Vtable

typedef dterr_t* (*dthttpd_loop_fn)(void* self DTHTTPD_LOOP_ARGS);
typedef dterr_t* (*dthttpd_stop_fn)(void* self DTHTTPD_STOP_ARGS);
typedef dterr_t* (*dthttpd_join_fn)(void* self DTHTTPD_JOIN_ARGS);
typedef dterr_t* (*dthttpd_set_callback_fn)(void* self DTHTTPD_SET_CALLBACK_ARGS);
typedef dterr_t* (*dthttpd_concat_format_fn)(void* self DTHTTPD_CONCAT_FORMAT_ARGS);
typedef void (*dthttpd_dispose_fn)(void* self);

typedef struct dthttpd_vt_t
{
    dthttpd_loop_fn loop;
    dthttpd_stop_fn stop;
    dthttpd_join_fn join;
    dthttpd_set_callback_fn set_callback;
    dthttpd_concat_format_fn concat_format;
    dthttpd_dispose_fn dispose;
} dthttpd_vt_t;

// Registry
extern dterr_t*
dthttpd_set_vtable(int32_t model_number, dthttpd_vt_t* vtable);
extern dterr_t*
dthttpd_get_vtable(int32_t model_number, dthttpd_vt_t** vtable);

// -----------------------------------------------------------------------------
// declaration dispatcher or implementation

#define DTHTTPD_DECLARE_API_EX(NAME, T)                                                                                        \
    extern dterr_t* NAME##_loop(NAME##T self DTHTTPD_LOOP_ARGS);                                                               \
    extern dterr_t* NAME##_stop(NAME##T self DTHTTPD_STOP_ARGS);                                                               \
    extern dterr_t* NAME##_join(NAME##T self DTHTTPD_JOIN_ARGS);                                                               \
    extern dterr_t* NAME##_set_callback(NAME##T self DTHTTPD_SET_CALLBACK_ARGS);                                               \
    extern dterr_t* NAME##_concat_format(NAME##T self DTHTTPD_CONCAT_FORMAT_ARGS);                                             \
    extern void NAME##_dispose(NAME##T self);

// declare dispatcher
DTHTTPD_DECLARE_API_EX(dthttpd, _handle)

// declare implementation (put this in its .h file)
#define DTHTTPD_DECLARE_API(NAME) DTHTTPD_DECLARE_API_EX(NAME, _t*)

// Helper for concrete objects to embed as first field
#define DTHTTPD_COMMON_MEMBERS int32_t model_number;

// Helper to build a vtable variable from backend symbol prefix
#define DTHTTPD_INIT_VTABLE(NAME)                                                                                              \
    static dthttpd_vt_t NAME##_vt = {                                                                                          \
        .loop = (dthttpd_loop_fn)NAME##_loop,                                                                                  \
        .stop = (dthttpd_stop_fn)NAME##_stop,                                                                                  \
        .join = (dthttpd_join_fn)NAME##_join,                                                                                  \
        .set_callback = (dthttpd_set_callback_fn)NAME##_set_callback,                                                          \
        .concat_format = (dthttpd_concat_format_fn)NAME##_concat_format,                                                       \
        .dispose = (dthttpd_dispose_fn)NAME##_dispose,                                                                         \
    }
