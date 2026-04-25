#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dterr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtdotstar.h>
#include <dtmc_base/dtdotstar_dummy.h>

// If you want silence in CI:
#define dtlog_debug(TAG, ...)
// --------------------------------------------------------------------------------------

static dterr_t*
test_dtmc_base_dtdotstar_dummy_basic(void)
{
    dterr_t* dterr = NULL;
    dtdotstar_dummy_t* dotstar_object = NULL;
    dtdotstar_handle dotstar_handle = NULL;

    {
        dtdotstar_dummy_t* o = NULL;

        // Create instance
        DTERR_C(dtdotstar_dummy_create(&o));
        dotstar_handle = (dtdotstar_handle)o;

        // Configure instance with known values
        dtdotstar_dummy_config_t c = { 0 };
        DTERR_C(dtdotstar_dummy_configure(o, &c));

        dotstar_object = o;
    }

    // Test connect increments _connect_call_count
    int32_t prev_connect_count = dotstar_object->_connect_call_count;
    DTERR_C(dtdotstar_connect(dotstar_handle));
    DTUNITTEST_ASSERT_INT(dotstar_object->_connect_call_count, ==, prev_connect_count + 1);

cleanup:
    dtdotstar_dispose(dotstar_handle);

    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtdotstar_dummy_basic failed");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Runs all tests
void
test_dtmc_base_dtdotstar_dummy(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtdotstar_dummy_basic);
}
