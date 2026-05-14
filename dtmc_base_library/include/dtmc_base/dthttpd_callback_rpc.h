#pragma once
/*
 * dthttpd_callback_rpc -- A dthttpd POST callback that dispatches to a user-provided callback.
 *
 * This is a simple implementation of a dthttpd_post_callback_t that dispatches to a user-provided callback.
 * The user callback is expected to handle the request and produce a response, content type, and status code.
 *
 * cdox v1.0.2
 */

#include <dtmc_base/dthttpd.h>

extern dterr_t* dthttpd_callback_rpc(void* context,
  const char* request_path,
  const dtbuffer_t* payload,
  dtbuffer_t** out_response,
  const char** out_content_type,
  int32_t* out_status_code);
