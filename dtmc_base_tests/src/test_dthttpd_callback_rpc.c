#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtkvp.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtrpc.h>
#include <dtcore/dtrpc_registry.h>
#include <dtcore/dtunittest.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dthttpd_callback_rpc.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dthttpd_callback_rpc"

// model number unique to this test file; idempotent to re-register same vtable pointer
#define STUB_MODEL 100

// -------------------------------------------------------------------------
typedef struct test_dtmc_base_dthttpd_callback_stub_t
{
    int32_t model_number;
    const char* accept_endpoint; // endpoint key must match; NULL = always refuse
    const char* response_key;    // key set in response when accepted; NULL = set nothing
    const char* response_value;
    const char* echo_request_key; // when non-NULL, reads this key from request and copies it to response
    int32_t call_count;
} test_dtmc_base_dthttpd_callback_stub_t;

DTRPC_DECLARE_API(test_dtmc_base_dthttpd_callback_stub);
DTRPC_INIT_VTABLE(test_dtmc_base_dthttpd_callback_stub);

// -------------------------------------------------------------------------
extern dterr_t*
test_dtmc_base_dthttpd_callback_stub_call(test_dtmc_base_dthttpd_callback_stub_t* self DTRPC_CALL_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    self->call_count += 1;

    if (self->accept_endpoint == NULL)
    {
        *was_refused = true;
        goto cleanup;
    }

    const char* endpoint = NULL;
    DTERR_C(dtkvp_list_get(request_kvp_list, "endpoint", &endpoint));
    if (endpoint == NULL || strcmp(endpoint, self->accept_endpoint) != 0)
    {
        *was_refused = true;
        goto cleanup;
    }

    *was_refused = false;

    if (self->response_key != NULL)
        DTERR_C(dtkvp_list_set(response_kvp_list, self->response_key, self->response_value));

    if (self->echo_request_key != NULL)
    {
        const char* val = NULL;
        DTERR_C(dtkvp_list_get(request_kvp_list, self->echo_request_key, &val));
        if (val != NULL)
            DTERR_C(dtkvp_list_set(response_kvp_list, self->echo_request_key, val));
    }

cleanup:
    return dterr;
}

// -------------------------------------------------------------------------
extern void
test_dtmc_base_dthttpd_callback_stub_dispose(test_dtmc_base_dthttpd_callback_stub_t* self)
{
    if (self != NULL)
        dtheaper_free(self);
}

// -------------------------------------------------------------------------
static dterr_t*
_stub_create(test_dtmc_base_dthttpd_callback_stub_t** self_ptr,
  const char* accept_endpoint,
  const char* response_key,
  const char* response_value,
  const char* echo_request_key)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    DTERR_C(dtrpc_set_vtable(STUB_MODEL, &test_dtmc_base_dthttpd_callback_stub_vt));
    DTERR_C(dtheaper_alloc_and_zero(
      (int32_t)sizeof(test_dtmc_base_dthttpd_callback_stub_t), "test_dtmc_base_dthttpd_callback_stub_t", (void**)self_ptr));

    test_dtmc_base_dthttpd_callback_stub_t* self = *self_ptr;
    self->model_number = STUB_MODEL;
    self->accept_endpoint = accept_endpoint;
    self->response_key = response_key;
    self->response_value = response_value;
    self->echo_request_key = echo_request_key;

cleanup:
    return dterr;
}

// -------------------------------------------------------------------------
// Example: single registered RPC accepts the call; response is 200 with urlencoded body.
static dterr_t*
test_dtmc_base_dthttpd_callback_rpc__example_single_rpc_accepted(void)
{
    dterr_t* dterr = NULL;
    dtrpc_registry_t* registry = NULL;
    test_dtmc_base_dthttpd_callback_stub_t* stub = NULL;
    dtbuffer_t payload = { 0 };
    dtbuffer_t* response = NULL;
    const char* content_type = NULL;
    int32_t status_code = 0;

    DTERR_C(dtrpc_registry_create(&registry));
    DTERR_C(_stub_create(&stub, "/rpc", "status", "ok", NULL));
    DTERR_C(dtrpc_registry_add(registry, (dtrpc_handle)stub));

    DTERR_C(dtbuffer_wrap(&payload, (void*)"", 0));
    DTERR_C(dthttpd_callback_rpc(registry, "/rpc", &payload, &response, &content_type, &status_code));

    DTUNITTEST_ASSERT_INT(status_code, ==, 200);
    DTUNITTEST_ASSERT_EQUAL_STRING(content_type, "text/plain; charset=utf-8");
    DTUNITTEST_ASSERT_NOT_NULL(response);
    DTUNITTEST_ASSERT_INT(stub->call_count, ==, 1);

    const char* expected = "status=ok";
    DTUNITTEST_ASSERT_INT(response->length, ==, (int32_t)strlen(expected));
    DTUNITTEST_ASSERT_EQUAL_BYTES(response->payload, expected, strlen(expected));

cleanup:
    if (response != NULL)
        dtbuffer_dispose(response);
    dtrpc_dispose((dtrpc_handle)stub);
    dtrpc_registry_dispose(registry);
    return dterr;
}

// -------------------------------------------------------------------------
// No RPCs in registry: every call returns 404 with a fixed error body.
static dterr_t*
test_dtmc_base_dthttpd_callback_rpc__no_rpcs_returns_404(void)
{
    dterr_t* dterr = NULL;
    dtrpc_registry_t* registry = NULL;
    dtbuffer_t payload = { 0 };
    dtbuffer_t* response = NULL;
    const char* content_type = NULL;
    int32_t status_code = 0;

    DTERR_C(dtrpc_registry_create(&registry));

    DTERR_C(dtbuffer_wrap(&payload, (void*)"", 0));
    DTERR_C(dthttpd_callback_rpc(registry, "/anything", &payload, &response, &content_type, &status_code));

    DTUNITTEST_ASSERT_INT(status_code, ==, 404);
    DTUNITTEST_ASSERT_NOT_NULL(response);

    const char* expected = "unknown post received";
    DTUNITTEST_ASSERT_INT(response->length, ==, (int32_t)strlen(expected));
    DTUNITTEST_ASSERT_EQUAL_BYTES(response->payload, expected, strlen(expected));

cleanup:
    if (response != NULL)
        dtbuffer_dispose(response);
    dtrpc_registry_dispose(registry);
    return dterr;
}

// -------------------------------------------------------------------------
// RPC that always refuses: falls through to 404.
static dterr_t*
test_dtmc_base_dthttpd_callback_rpc__rpc_refuses_returns_404(void)
{
    dterr_t* dterr = NULL;
    dtrpc_registry_t* registry = NULL;
    test_dtmc_base_dthttpd_callback_stub_t* stub = NULL;
    dtbuffer_t payload = { 0 };
    dtbuffer_t* response = NULL;
    const char* content_type = NULL;
    int32_t status_code = 0;

    DTERR_C(dtrpc_registry_create(&registry));
    DTERR_C(_stub_create(&stub, NULL, NULL, NULL, NULL)); // accept_endpoint=NULL → always refuse
    DTERR_C(dtrpc_registry_add(registry, (dtrpc_handle)stub));

    DTERR_C(dtbuffer_wrap(&payload, (void*)"", 0));
    DTERR_C(dthttpd_callback_rpc(registry, "/rpc", &payload, &response, &content_type, &status_code));

    DTUNITTEST_ASSERT_INT(status_code, ==, 404);
    DTUNITTEST_ASSERT_INT(stub->call_count, ==, 1);

cleanup:
    if (response != NULL)
        dtbuffer_dispose(response);
    dtrpc_dispose((dtrpc_handle)stub);
    dtrpc_registry_dispose(registry);
    return dterr;
}

// -------------------------------------------------------------------------
// Two RPCs registered: first refuses (wrong endpoint), second accepts.
static dterr_t*
test_dtmc_base_dthttpd_callback_rpc__first_refuses_second_accepts(void)
{
    dterr_t* dterr = NULL;
    dtrpc_registry_t* registry = NULL;
    test_dtmc_base_dthttpd_callback_stub_t* stub1 = NULL;
    test_dtmc_base_dthttpd_callback_stub_t* stub2 = NULL;
    dtbuffer_t payload = { 0 };
    dtbuffer_t* response = NULL;
    const char* content_type = NULL;
    int32_t status_code = 0;

    DTERR_C(dtrpc_registry_create(&registry));
    DTERR_C(_stub_create(&stub1, "/other", "from", "first", NULL));
    DTERR_C(_stub_create(&stub2, "/cmd", "from", "second", NULL));
    DTERR_C(dtrpc_registry_add(registry, (dtrpc_handle)stub1));
    DTERR_C(dtrpc_registry_add(registry, (dtrpc_handle)stub2));

    DTERR_C(dtbuffer_wrap(&payload, (void*)"", 0));
    DTERR_C(dthttpd_callback_rpc(registry, "/cmd", &payload, &response, &content_type, &status_code));

    DTUNITTEST_ASSERT_INT(status_code, ==, 200);
    DTUNITTEST_ASSERT_INT(stub1->call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(stub2->call_count, ==, 1);

    const char* expected = "from=second";
    DTUNITTEST_ASSERT_INT(response->length, ==, (int32_t)strlen(expected));
    DTUNITTEST_ASSERT_EQUAL_BYTES(response->payload, expected, strlen(expected));

cleanup:
    if (response != NULL)
        dtbuffer_dispose(response);
    dtrpc_dispose((dtrpc_handle)stub2);
    dtrpc_dispose((dtrpc_handle)stub1);
    dtrpc_registry_dispose(registry);
    return dterr;
}

// -------------------------------------------------------------------------
// Urlencoded payload is parsed into the request KVP list and visible to the RPC.
static dterr_t*
test_dtmc_base_dthttpd_callback_rpc__payload_kvp_forwarded_to_rpc(void)
{
    dterr_t* dterr = NULL;
    dtrpc_registry_t* registry = NULL;
    test_dtmc_base_dthttpd_callback_stub_t* stub = NULL;
    dtbuffer_t payload = { 0 };
    dtbuffer_t* response = NULL;
    const char* content_type = NULL;
    int32_t status_code = 0;

    DTERR_C(dtrpc_registry_create(&registry));
    DTERR_C(_stub_create(&stub, "/cmd", NULL, NULL, "mode")); // echo "mode" key from request
    DTERR_C(dtrpc_registry_add(registry, (dtrpc_handle)stub));

    const char* body = "mode=run";
    DTERR_C(dtbuffer_wrap(&payload, (void*)body, (int32_t)strlen(body)));
    DTERR_C(dthttpd_callback_rpc(registry, "/cmd", &payload, &response, &content_type, &status_code));

    DTUNITTEST_ASSERT_INT(status_code, ==, 200);
    DTUNITTEST_ASSERT_NOT_NULL(response);

    const char* expected = "mode=run";
    DTUNITTEST_ASSERT_INT(response->length, ==, (int32_t)strlen(expected));
    DTUNITTEST_ASSERT_EQUAL_BYTES(response->payload, expected, strlen(expected));

cleanup:
    if (response != NULL)
        dtbuffer_dispose(response);
    dtrpc_dispose((dtrpc_handle)stub);
    dtrpc_registry_dispose(registry);
    return dterr;
}

// -------------------------------------------------------------------------
void
test_dtmc_base_dthttpd_callback_rpc(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpd_callback_rpc__example_single_rpc_accepted);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpd_callback_rpc__no_rpcs_returns_404);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpd_callback_rpc__rpc_refuses_returns_404);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpd_callback_rpc__first_refuses_second_accepts);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dthttpd_callback_rpc__payload_kvp_forwarded_to_rpc);
}
