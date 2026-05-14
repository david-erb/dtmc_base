// test_dtmc_base_dthttpclient.c
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtkvp.h>
#include <dtcore/dtlog.h>
#include <dtcore/dttimeout.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dthttpclient.h>

#define TAG "test_dtmc_base_dthttpclient"

#define DTHTTPCLIENT_TEST_BASE_URL_ENV "DTHTTPCLIENT_TEST_BASE_URL"

// Example:
//   export DTHTTPCLIENT_TEST_BASE_URL="https://httpbin.org"

// -------------------------------------------------------------------------------
static void
test_dtmc_base_dthttpclient_response_dispose(dthttpclient_response_t* response)
{
    if (response == NULL)
        return;

    if (response->body != NULL)
    {
        dtbuffer_dispose(response->body);
        response->body = NULL;
    }

    response->status_code = 0;
    response->content_type[0] = '\0';
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dthttpclient_simple(void)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dthttpclient_response_t response = { 0 };
    dtkvp_list_t query_params = { 0 };
    char url[512];

    const char* base_url = getenv(DTHTTPCLIENT_TEST_BASE_URL_ENV);
    if (base_url == NULL || base_url[0] == '\0')
    {
        base_url = "https://httpbin.org";
        dtlog_debug(TAG, "Using default test server: %s", base_url);
    }

    snprintf(url, sizeof(url), "%s/get", base_url);

    DTERR_C(dthttpclient_create(&httpclient_handle));

    DTERR_C(dtkvp_list_init(&query_params));

    DTERR_C(dtkvp_list_set(&query_params, "mode", "happy"));
    DTERR_C(dtkvp_list_set(&query_params, "count", "1"));

    DTERR_C(dthttpclient_get(httpclient_handle, url, &query_params, 5000, &response));

    DTUNITTEST_ASSERT_INT(response.status_code, ==, 200);
    DTUNITTEST_ASSERT_NOT_NULL(response.body);
    DTUNITTEST_ASSERT_INT(response.body->length, >, 0);
    DTUNITTEST_ASSERT_NOT_NULL(response.body->payload);

cleanup:
    test_dtmc_base_dthttpclient_response_dispose(&response);

    dtkvp_list_dispose(&query_params);

    dthttpclient_dispose(httpclient_handle);

    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dthttpclient_create_dispose(void)
{
    dterr_t* dterr = NULL;
    dthttpclient_handle httpclient_handle = NULL;

    DTERR_C(dthttpclient_create(&httpclient_handle));
    DTUNITTEST_ASSERT_NOT_NULL(httpclient_handle);

cleanup:
    dthttpclient_dispose(httpclient_handle);

    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dthttpclient_get_rejects_nowait(void)
{
    dterr_t* dterr = NULL;
    dterr_t* err = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dthttpclient_response_t response = { 0 };

    DTERR_C(dthttpclient_create(&httpclient_handle));

    err = dthttpclient_get(httpclient_handle, "http://127.0.0.1/", NULL, DTTIMEOUT_NOWAIT, &response);

    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADARG);
    DTUNITTEST_ASSERT_NULL(response.body);
    DTUNITTEST_ASSERT_INT(response.status_code, ==, 0);

cleanup:
    if (err != NULL)
        dterr_dispose(err);

    test_dtmc_base_dthttpclient_response_dispose(&response);
    dthttpclient_dispose(httpclient_handle);

    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dthttpclient_post_rejects_nowait(void)
{
    dterr_t* dterr = NULL;
    dterr_t* err = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dthttpclient_response_t response = { 0 };

    DTERR_C(dthttpclient_create(&httpclient_handle));

    err = dthttpclient_post(httpclient_handle, "http://127.0.0.1/", NULL, NULL, "text/plain", DTTIMEOUT_NOWAIT, &response);

    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADARG);
    DTUNITTEST_ASSERT_NULL(response.body);
    DTUNITTEST_ASSERT_INT(response.status_code, ==, 0);

cleanup:
    if (err != NULL)
        dterr_dispose(err);

    test_dtmc_base_dthttpclient_response_dispose(&response);
    dthttpclient_dispose(httpclient_handle);

    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dthttpclient_get_null_url_is_argument_null(void)
{
    dterr_t* dterr = NULL;
    dterr_t* err = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dthttpclient_response_t response = { 0 };

    DTERR_C(dthttpclient_create(&httpclient_handle));

    err = dthttpclient_get(httpclient_handle, NULL, NULL, 1000, &response);

    DTUNITTEST_ASSERT_DTERR(err, DTERR_ARGUMENT_NULL);

cleanup:
    if (err != NULL)
        dterr_dispose(err);

    test_dtmc_base_dthttpclient_response_dispose(&response);
    dthttpclient_dispose(httpclient_handle);

    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dthttpclient_get_null_response_is_argument_null(void)
{
    dterr_t* dterr = NULL;
    dterr_t* err = NULL;
    dthttpclient_handle httpclient_handle = NULL;

    DTERR_C(dthttpclient_create(&httpclient_handle));

    err = dthttpclient_get(httpclient_handle, "http://127.0.0.1/", NULL, 1000, NULL);

    DTUNITTEST_ASSERT_DTERR(err, DTERR_ARGUMENT_NULL);

cleanup:
    if (err != NULL)
        dterr_dispose(err);

    dthttpclient_dispose(httpclient_handle);

    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dthttpclient_post_too_long_content_type_is_overflow(void)
{
    dterr_t* dterr = NULL;
    dterr_t* err = NULL;
    dthttpclient_handle httpclient_handle = NULL;
    dthttpclient_response_t response = { 0 };
    char content_type[DTHTTPCLIENT_MAX_CONTENT_TYPE_BYTES + 64];

    memset(content_type, 'a', sizeof(content_type) - 1);
    content_type[sizeof(content_type) - 1] = '\0';

    DTERR_C(dthttpclient_create(&httpclient_handle));

    err = dthttpclient_post(httpclient_handle, "http://127.0.0.1/", NULL, NULL, content_type, 1000, &response);

    DTUNITTEST_ASSERT_DTERR(err, DTERR_OVERFLOW);
    DTUNITTEST_ASSERT_NULL(response.body);
    DTUNITTEST_ASSERT_INT(response.status_code, ==, 0);

cleanup:
    if (err != NULL)
        dterr_dispose(err);

    test_dtmc_base_dthttpclient_response_dispose(&response);
    dthttpclient_dispose(httpclient_handle);

    return dterr;
}

// --------------------------------------------------------------------------------------------
// Entry point for all dthttpclient unit tests
void
test_dtmc_base_dthttpclient(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpclient_simple);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpclient_create_dispose);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpclient_get_rejects_nowait);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpclient_post_rejects_nowait);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpclient_get_null_url_is_argument_null);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpclient_get_null_response_is_argument_null);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpclient_post_too_long_content_type_is_overflow);
}