/*
 * dthttpd_callback_rpc -- A dthttpd POST callback that dispatches to a user-provided callback.
 *
 * This is a simple implementation of a dthttpd_post_callback_t that dispatches to a user-provided callback.
 * The user callback is expected to handle the request and produce a response, content type, and status code.
 *
 * cdox v1.0.2
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dtkvp.h>
#include <dtcore/dtrpc.h>
#include <dtcore/dtrpc_registry.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dthttpd.h>
#include <dtmc_base/dthttpd_callback_rpc.h>

// --------------------------------------------------------------------------------------
dterr_t*
dthttpd_callback_rpc(void* context,
  const char* request_path,
  const dtbuffer_t* payload,
  dtbuffer_t** out_response,
  const char** out_content_type,
  int32_t* out_status_code)
{
    dterr_t* dterr = NULL;
    dtrpc_registry_t* self = (dtrpc_registry_t*)context;
    dtkvp_list_t request_kvp_list = { 0 }, response_kvp_list = { 0 };
    char* tmp_str = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    // add string terminator to payload for easier processing
    tmp_str = (char*)malloc((size_t)payload->length + 1);
    memcpy(tmp_str, payload->payload, (size_t)payload->length);
    tmp_str[payload->length] = '\0';

    DTERR_C(dtkvp_list_init(&request_kvp_list));
    // seed with the endpoint path, so that RPCs can use that to determine if they should handle the call
    DTERR_C(dtkvp_list_set(&request_kvp_list, "endpoint", request_path));
    // parse the payload into a kvp list (expecting urlencoded form data)
    DTERR_C(dtkvp_list_urldecode(&request_kvp_list, tmp_str));
    free(tmp_str);
    tmp_str = NULL;

    dtrpc_handle rpc_handle = NULL;
    int32_t index = 0;
    bool was_refused = false;
    DTERR_C(dtkvp_list_init(&response_kvp_list));
    while (true)
    {
        // try each registered RPC until one accepts the call (or we run out of RPCs to try)
        DTERR_C(dtrpc_registry_get(self, index, &rpc_handle));
        if (rpc_handle == NULL)
        {
            was_refused = true;
            break;
        }

        index++;

        DTERR_C(dtrpc_call(rpc_handle, &request_kvp_list, &was_refused, &response_kvp_list));

        if (!was_refused)
        {
            break;
        }
    }

    if (!was_refused)
    {
        // turn the response kvp list into a urlencoded string and return that as the response body
        DTERR_C(dtkvp_list_urlencode(&response_kvp_list, &tmp_str));
        int32_t resp_len = (int32_t)strlen(tmp_str);
        DTERR_C(dtbuffer_create(out_response, resp_len));
        memcpy((*out_response)->payload, tmp_str, (size_t)resp_len);
        free(tmp_str);
        tmp_str = NULL;
        *out_content_type = "text/plain; charset=utf-8";
        *out_status_code = 200;
    }
    else
    {
        const char* text = "unknown post received";
        int32_t text_len = (int32_t)strlen(text);
        DTERR_C(dtbuffer_create(out_response, text_len));
        memcpy((*out_response)->payload, text, (size_t)text_len);
        *out_content_type = "text/plain; charset=utf-8";
        *out_status_code = 404;
    }

cleanup:
    if (tmp_str != NULL)
        free(tmp_str);
    dtkvp_list_dispose(&request_kvp_list);
    dtkvp_list_dispose(&response_kvp_list);
    return dterr;
}
