/*
 * dthttpclient -- Cross-platform counting httpclient facade.
 *
 * Wraps platform httpclient primitives (GET and POST) behind a
 * common create/get/post/dispose API with millisecond-resolution timeouts.
 *
 * Each platform (POSIX, FreeRTOS, Zephyr) may have different capabilities and limitations
 * around HTTP client functionality.
 *
 * Ownership of buffers returned in the response are transferred to the caller.
 *
 * cdox v1.0.2.1
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtkvp.h>
#include <dtcore/dttimeout.h>

#define DTHTTPCLIENT_MAX_CONTENT_TYPE_BYTES (128)

// -----------------------------------------------------------------------------
typedef struct dthttpclient_response_t
{
    int32_t status_code;
    dtbuffer_t* body;
    char content_type[DTHTTPCLIENT_MAX_CONTENT_TYPE_BYTES];
} dthttpclient_response_t;

// Opaque handle used by dispatcher functions.
struct dthttpclient_handle_t;
typedef struct dthttpclient_handle_t* dthttpclient_handle;

// ----------------------------------------------------------------------------
// Create httpclient.  Allocates memory and references the system resources for the httpclient.
extern dterr_t*
dthttpclient_create(dthttpclient_handle* self);

// ----------------------------------------------------------------------------
// run a GET request to the specified URL, waiting for completion up to the specified timeout.
extern dterr_t*
dthttpclient_get(dthttpclient_handle self,
  const char* url,
  dtkvp_list_t* query_params,
  dttimeout_millis_t timeout_milliseconds,
  dthttpclient_response_t* out_response);

// ----------------------------------------------------------------------------
// run a POST request to the specified URL with the given payload, waiting for completion up to the specified timeout.
extern dterr_t*
dthttpclient_post(dthttpclient_handle self,
  const char* url,
  dtkvp_list_t* query_params,
  dtbuffer_t* payload,
  const char* content_type,
  dttimeout_millis_t timeout_milliseconds,
  dthttpclient_response_t* out_response);

// -----------------------------------------------------------------------------
// Dispose httpclient resources.  Implies release.  No error if already disposed.
extern void
dthttpclient_dispose(dthttpclient_handle self);
