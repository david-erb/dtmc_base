#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dttimeout.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dthttpclient.h>
#include <dtmc_base/dthttpd.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base_tests.h>

extern dtledger_t* dterr_ledger;

#define TAG "test_dtmc_base_dthttpd_simple"

// --------------------------------------------------------------------------------------------
// Helper: allocate a dtbuffer_t response for use in POST callbacks.
// Ownership transfers to the server, which disposes it via dtbuffer_dispose.
static dterr_t*
_make_response(const char* text, dtbuffer_t** out_response)
{
    dterr_t* dterr = NULL;
    int32_t len = (int32_t)strlen(text);

    DTERR_C(dtbuffer_create(out_response, len));
    memcpy((*out_response)->payload, text, (size_t)len);

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
_POST_simple_callback(
  void* context,
  const char* request_path,
  const dtbuffer_t* payload,
  dtbuffer_t** out_response,
  const char** out_content_type,
  int32_t* out_status_code)
{
    dterr_t* dterr = NULL;

    (void)context;

    DTERR_ASSERT_NOT_NULL(request_path);
    DTERR_ASSERT_NOT_NULL(payload);

    DTUNITTEST_ASSERT_EQUAL_STRING(request_path, "/test");
    DTUNITTEST_ASSERT_INT(payload->length, ==, (int32_t)strlen("hello from dthttpclient"));
    DTUNITTEST_ASSERT_EQUAL_BYTES(payload->payload, "hello from dthttpclient", strlen("hello from dthttpclient"));

    DTERR_C(_make_response("POST ok", out_response));
    *out_content_type = "text/plain";
    *out_status_code = 200;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
_POST_json_callback(
  void* context,
  const char* request_path,
  const dtbuffer_t* payload,
  dtbuffer_t** out_response,
  const char** out_content_type,
  int32_t* out_status_code)
{
    dterr_t* dterr = NULL;

    (void)context;

    DTERR_ASSERT_NOT_NULL(request_path);
    DTERR_ASSERT_NOT_NULL(payload);

    DTUNITTEST_ASSERT_EQUAL_STRING(request_path, "/api/data");
    DTUNITTEST_ASSERT_INT(payload->length, ==, (int32_t)strlen("{\"key\":\"value\"}"));
    DTUNITTEST_ASSERT_EQUAL_BYTES(payload->payload, "{\"key\":\"value\"}", strlen("{\"key\":\"value\"}"));

    DTERR_C(_make_response("{\"result\":\"ok\"}", out_response));
    *out_content_type = "application/json";
    *out_status_code = 201;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
_POST_empty_callback(
  void* context,
  const char* request_path,
  const dtbuffer_t* payload,
  dtbuffer_t** out_response,
  const char** out_content_type,
  int32_t* out_status_code)
{
    dterr_t* dterr = NULL;

    (void)context;

    DTERR_ASSERT_NOT_NULL(request_path);
    DTERR_ASSERT_NOT_NULL(payload);

    DTUNITTEST_ASSERT_EQUAL_STRING(request_path, "/empty");
    DTUNITTEST_ASSERT_INT(payload->length, ==, 0);

    DTERR_C(_make_response("empty ok", out_response));
    *out_content_type = "text/plain";
    *out_status_code = 200;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
_POST_count_callback(
  void* context,
  const char* request_path,
  const dtbuffer_t* payload,
  dtbuffer_t** out_response,
  const char** out_content_type,
  int32_t* out_status_code)
{
    dterr_t* dterr = NULL;
    int32_t* count = (int32_t*)context;

    (void)payload;

    DTERR_ASSERT_NOT_NULL(request_path);
    DTERR_ASSERT_NOT_NULL(count);

    DTUNITTEST_ASSERT_EQUAL_STRING(request_path, "/count");

    (*count)++;

    DTERR_C(_make_response("counted", out_response));
    *out_content_type = "text/plain";
    *out_status_code = 200;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
test_dtmc_base_dthttpd_simple(dthttpd_handle httpd_handle)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dtbuffer_t request_body = { 0 };
    dthttpclient_response_t response = { 0 };

    const char* request_text = "hello from dthttpclient";

    DTERR_C(dthttpd_set_callback(httpd_handle, _POST_simple_callback, NULL));
    DTERR_C(dthttpd_loop(httpd_handle));
    dtruntime_sleep_milliseconds(100);

    DTERR_C(dthttpclient_create(&httpclient_handle));
    DTERR_C(dtbuffer_wrap(&request_body, (void*)request_text, (int32_t)strlen(request_text)));
    DTERR_C(
      dthttpclient_post(httpclient_handle, "http://127.0.0.1:8080/test", NULL, &request_body, "text/plain", 3000, &response));

    DTUNITTEST_ASSERT_INT(response.status_code, ==, 200);
    DTUNITTEST_ASSERT_NOT_NULL(response.body);
    DTUNITTEST_ASSERT_INT(response.body->length, ==, (int32_t)strlen("POST ok"));
    DTUNITTEST_ASSERT_EQUAL_BYTES(response.body->payload, "POST ok", strlen("POST ok"));
    DTUNITTEST_ASSERT_EQUAL_STRING(response.content_type, "text/plain");

    DTERR_C(dthttpd_stop(httpd_handle));
    DTERR_C(dthttpd_join(httpd_handle, DTTIMEOUT_FOREVER, NULL));

cleanup:
    if (response.body != NULL)
        dtbuffer_dispose(response.body);
    dthttpclient_dispose(httpclient_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
test_dtmc_base_dthttpd_post_json(dthttpd_handle httpd_handle)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dtbuffer_t request_body = { 0 };
    dthttpclient_response_t response = { 0 };

    DTERR_C(dthttpd_set_callback(httpd_handle, _POST_json_callback, NULL));
    DTERR_C(dthttpd_loop(httpd_handle));
    dtruntime_sleep_milliseconds(100);

    DTERR_C(dthttpclient_create(&httpclient_handle));
    DTERR_C(dtbuffer_wrap(&request_body, (void*)"{\"key\":\"value\"}", (int32_t)strlen("{\"key\":\"value\"}")));
    DTERR_C(dthttpclient_post(
      httpclient_handle, "http://127.0.0.1:8080/api/data", NULL, &request_body, "application/json", 3000, &response));

    DTUNITTEST_ASSERT_INT(response.status_code, ==, 201);
    DTUNITTEST_ASSERT_NOT_NULL(response.body);
    DTUNITTEST_ASSERT_INT(response.body->length, ==, (int32_t)strlen("{\"result\":\"ok\"}"));
    DTUNITTEST_ASSERT_EQUAL_BYTES(response.body->payload, "{\"result\":\"ok\"}", strlen("{\"result\":\"ok\"}"));
    DTUNITTEST_ASSERT_EQUAL_STRING(response.content_type, "application/json");

    DTERR_C(dthttpd_stop(httpd_handle));
    DTERR_C(dthttpd_join(httpd_handle, DTTIMEOUT_FOREVER, NULL));

cleanup:
    if (response.body != NULL)
        dtbuffer_dispose(response.body);
    dthttpclient_dispose(httpclient_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
test_dtmc_base_dthttpd_post_empty_body(dthttpd_handle httpd_handle)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dthttpclient_response_t response = { 0 };

    DTERR_C(dthttpd_set_callback(httpd_handle, _POST_empty_callback, NULL));
    DTERR_C(dthttpd_loop(httpd_handle));
    dtruntime_sleep_milliseconds(100);

    DTERR_C(dthttpclient_create(&httpclient_handle));
    DTERR_C(dthttpclient_post(httpclient_handle, "http://127.0.0.1:8080/empty", NULL, NULL, "text/plain", 3000, &response));

    DTUNITTEST_ASSERT_INT(response.status_code, ==, 200);
    DTUNITTEST_ASSERT_NOT_NULL(response.body);
    DTUNITTEST_ASSERT_INT(response.body->length, ==, (int32_t)strlen("empty ok"));
    DTUNITTEST_ASSERT_EQUAL_BYTES(response.body->payload, "empty ok", strlen("empty ok"));

    DTERR_C(dthttpd_stop(httpd_handle));
    DTERR_C(dthttpd_join(httpd_handle, DTTIMEOUT_FOREVER, NULL));

cleanup:
    if (response.body != NULL)
        dtbuffer_dispose(response.body);
    dthttpclient_dispose(httpclient_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
test_dtmc_base_dthttpd_get_not_found(dthttpd_handle httpd_handle)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dthttpclient_response_t response = { 0 };

    DTERR_C(dthttpd_set_callback(httpd_handle, NULL, NULL));
    DTERR_C(dthttpd_loop(httpd_handle));
    dtruntime_sleep_milliseconds(100);

    DTERR_C(dthttpclient_create(&httpclient_handle));
    DTERR_C(dthttpclient_get(httpclient_handle, "http://127.0.0.1:8080/anything", NULL, 3000, &response));

    DTUNITTEST_ASSERT_INT(response.status_code, ==, 404);

    DTERR_C(dthttpd_stop(httpd_handle));
    DTERR_C(dthttpd_join(httpd_handle, DTTIMEOUT_FOREVER, NULL));

cleanup:
    if (response.body != NULL)
        dtbuffer_dispose(response.body);
    dthttpclient_dispose(httpclient_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
test_dtmc_base_dthttpd_post_no_callback(dthttpd_handle httpd_handle)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dtbuffer_t request_body = { 0 };
    dthttpclient_response_t response = { 0 };

    DTERR_C(dthttpd_set_callback(httpd_handle, NULL, NULL));
    DTERR_C(dthttpd_loop(httpd_handle));
    dtruntime_sleep_milliseconds(100);

    DTERR_C(dthttpclient_create(&httpclient_handle));
    DTERR_C(dtbuffer_wrap(&request_body, (void*)"anything", (int32_t)strlen("anything")));
    DTERR_C(
      dthttpclient_post(httpclient_handle, "http://127.0.0.1:8080/test", NULL, &request_body, "text/plain", 3000, &response));

    DTUNITTEST_ASSERT_INT(response.status_code, ==, 405);

    DTERR_C(dthttpd_stop(httpd_handle));
    DTERR_C(dthttpd_join(httpd_handle, DTTIMEOUT_FOREVER, NULL));

cleanup:
    if (response.body != NULL)
        dtbuffer_dispose(response.body);
    dthttpclient_dispose(httpclient_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
test_dtmc_base_dthttpd_post_sequential(dthttpd_handle httpd_handle)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dtbuffer_t request_body = { 0 };
    dthttpclient_response_t response = { 0 };
    int32_t call_count = 0;

    DTERR_C(dthttpd_set_callback(httpd_handle, _POST_count_callback, &call_count));
    DTERR_C(dthttpd_loop(httpd_handle));
    dtruntime_sleep_milliseconds(100);

    DTERR_C(dthttpclient_create(&httpclient_handle));
    DTERR_C(dtbuffer_wrap(&request_body, (void*)"ping", (int32_t)strlen("ping")));

    DTERR_C(
      dthttpclient_post(httpclient_handle, "http://127.0.0.1:8080/count", NULL, &request_body, "text/plain", 3000, &response));
    DTUNITTEST_ASSERT_INT(response.status_code, ==, 200);
    if (response.body != NULL)
    {
        dtbuffer_dispose(response.body);
        response.body = NULL;
    }

    DTERR_C(
      dthttpclient_post(httpclient_handle, "http://127.0.0.1:8080/count", NULL, &request_body, "text/plain", 3000, &response));
    DTUNITTEST_ASSERT_INT(response.status_code, ==, 200);
    if (response.body != NULL)
    {
        dtbuffer_dispose(response.body);
        response.body = NULL;
    }

    DTUNITTEST_ASSERT_INT(call_count, ==, 2);

    DTERR_C(dthttpd_stop(httpd_handle));
    DTERR_C(dthttpd_join(httpd_handle, DTTIMEOUT_FOREVER, NULL));

cleanup:
    if (response.body != NULL)
        dtbuffer_dispose(response.body);
    dthttpclient_dispose(httpclient_handle);
    return dterr;
}
