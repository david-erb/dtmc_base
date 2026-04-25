// test_dtmc_base_dtnetprofile.c
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>

#include <dtcore/dtunittest.h>
#include <dtmc_base/dtnetprofile.h>

// The dtnetprofile API under test provides:
//   dtnetprofile_add_*(), dtnetprofile_get(), dtnetprofile_to_string(),
//   dtnetprofile_pack_length(), dtnetprofile_pack(), dtnetprofile_unpack(), dtnetprofile_dispose()
// Implementation stores *newline-separated* lists in a single C string: "a\nb\nc\0".

// --- helpers ----------------------------------------------------------------

static void
fill_sample(dtnetprofile_t* p)
{
    memset(p, 0, sizeof(*p));
    dtnetprofile_add_name(p, "alpha");
    dtnetprofile_add_name(p, "beta");
    dtnetprofile_add_url(p, "coap://fd00::1:5683");
    dtnetprofile_add_role(p, "sensor");
    dtnetprofile_add_role(p, "edge");
    dtnetprofile_add_subscription(p, "foo/in");
    dtnetprofile_add_publication(p, "bar/out");
    dtnetprofile_add_state(p, "ready");
    dtnetprofile_add_state(p, "ok");
}

// --- tests ------------------------------------------------------------------

static dterr_t*
test_dtmc_base_dtnetprofile_to_string_empty(void)
{
    dterr_t* dterr = NULL;

    char out[64];
    dtnetprofile_t p = { 0 };

    dtnetprofile_to_string(&p, out, (int)sizeof(out));
    DTUNITTEST_ASSERT_EQUAL_STRING(out, "(empty)"); // empty lists render as (empty)

cleanup:
    dtnetprofile_dispose(&p);
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtnetprofile_add_and_get(void)
{
    dterr_t* dterr = NULL;
    dtnetprofile_t p = { 0 };

    dtnetprofile_add_name(&p, "one");
    dtnetprofile_add_name(&p, "two");

    const char* v = (const char*)0xDEADBEEF;
    dtnetprofile_get(&p, p.name_list, 0, &v);
    DTUNITTEST_ASSERT_EQUAL_STRING(v, "one");
    dtnetprofile_get(&p, p.name_list, 1, &v);
    DTUNITTEST_ASSERT_EQUAL_STRING(v, "two");
    dtnetprofile_get(&p, p.name_list, 2, &v); // out of range -> NULL
    DTUNITTEST_ASSERT_NULL(v);

cleanup:
    dtnetprofile_dispose(&p);
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtnetprofile_to_string_labels_and_items(void)
{
    dterr_t* dterr = NULL;
    dtnetprofile_t p = { 0 };
    fill_sample(&p);

    char out[256];
    dtnetprofile_to_string(&p, out, (int)sizeof(out));
    // Should contain labeled lines for non-empty lists. Exact order matches implementation.
    DTUNITTEST_ASSERT_TRUE(strstr(out, "names: alpha, beta\n") != NULL);
    DTUNITTEST_ASSERT_TRUE(strstr(out, "urls: coap://fd00::1:5683\n") != NULL);
    DTUNITTEST_ASSERT_TRUE(strstr(out, "roles: sensor, edge\n") != NULL);
    DTUNITTEST_ASSERT_TRUE(strstr(out, "subscriptions: foo/in\n") != NULL);
    DTUNITTEST_ASSERT_TRUE(strstr(out, "publications: bar/out\n") != NULL);
    DTUNITTEST_ASSERT_TRUE(strstr(out, "states: ready, ok") != NULL);

cleanup:
    dtnetprofile_dispose(&p);
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtnetprofile_pack_unpack_roundtrip_real_packer(void)
{
    dterr_t* dterr = NULL;
    uint8_t* buffer = NULL;

    dtnetprofile_t a = { 0 };
    dtnetprofile_t b = { 0 };
    fill_sample(&a);

    // Allocate buffer for packing using real pack_length
    int32_t pack_len = dtnetprofile_pack_length(&a);
    DTUNITTEST_ASSERT_INT(pack_len, >, 0);

    buffer = (uint8_t*)malloc((size_t)pack_len);
    DTUNITTEST_ASSERT_NOT_NULL(buffer);
    memset(buffer, 0, (size_t)pack_len);

    // Pack
    int32_t wrote = dtnetprofile_pack(&a, buffer, 0, pack_len);
    DTUNITTEST_ASSERT_INT(wrote, ==, pack_len);

    // Unpack into a fresh struct
    int32_t read = dtnetprofile_unpack(&b, buffer, 0, pack_len);
    DTUNITTEST_ASSERT_INT(read, ==, pack_len);

    // Compare human-readable forms (simple and robust)
    char sa[256], sb[256];
    dtnetprofile_to_string(&a, sa, (int)sizeof(sa));
    dtnetprofile_to_string(&b, sb, (int)sizeof(sb));
    DTUNITTEST_ASSERT_EQUAL_STRING(sa, sb);

cleanup:
    if (buffer)
        free(buffer);
    dtnetprofile_dispose(&a);
    dtnetprofile_dispose(&b);
    return dterr;
}

static dterr_t*
test_dtmc_base_dtnetprofile_dispose_nulls_fields(void)
{
    dterr_t* dterr = NULL;
    dtnetprofile_t p = { 0 };
    fill_sample(&p);

    DTUNITTEST_ASSERT_NOT_NULL(p.name_list);
    DTUNITTEST_ASSERT_NOT_NULL(p.url_list);
    DTUNITTEST_ASSERT_NOT_NULL(p.role_list);
    DTUNITTEST_ASSERT_NOT_NULL(p.subscription_list);
    DTUNITTEST_ASSERT_NOT_NULL(p.publication_list);
    DTUNITTEST_ASSERT_NOT_NULL(p.state_list);

    dtnetprofile_dispose(&p); // frees and NULLs all lists

    DTUNITTEST_ASSERT_NULL(p.name_list);
    DTUNITTEST_ASSERT_NULL(p.url_list);
    DTUNITTEST_ASSERT_NULL(p.role_list);
    DTUNITTEST_ASSERT_NULL(p.subscription_list);
    DTUNITTEST_ASSERT_NULL(p.publication_list);
    DTUNITTEST_ASSERT_NULL(p.state_list);

cleanup:
    dtnetprofile_dispose(&p);
    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtnetprofile(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetprofile_to_string_empty);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetprofile_add_and_get);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetprofile_to_string_labels_and_items);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetprofile_pack_unpack_roundtrip_real_packer);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtnetprofile_dispose_nulls_fields);
}
