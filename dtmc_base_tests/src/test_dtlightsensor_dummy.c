#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtlightsensor.h>
#include <dtmc_base/dtlightsensor_dummy.h>

// --------------------------------------------------------------------------------------------
// Unit test: Ensure the dummy client records method calls
static dterr_t*
test_dtmc_base_dtlightsensor_dummy_basic_usage(void)
{
    dterr_t* dterr = NULL;
    dtlightsensor_dummy_t* dummy = NULL;
    dtlightsensor_handle handle = NULL;

    DTERR_C(dtlightsensor_dummy_create(&dummy));
    handle = (dtlightsensor_handle)dummy;

    // Configure
    dtlightsensor_dummy_config_t config = { .placeholder = 42 };
    DTERR_C(dtlightsensor_dummy_configure(dummy, &config));
    DTUNITTEST_ASSERT_INT(dummy->config.placeholder, ==, 42);

    DTERR_C(dtlightsensor_activate(handle));
    int64_t sample = 0;
    DTERR_C(dtlightsensor_sample(handle, &sample));

    // Validate call counts
    DTUNITTEST_ASSERT_INT(dummy->_create_call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(dummy->_init_call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(dummy->_activate_call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(dummy->_sample_call_count, ==, 1);
    DTUNITTEST_ASSERT_UINT64(sample, ==, 1);

cleanup:
    dtlightsensor_dispose(handle);

    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtlightsensor_dummy(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtlightsensor_dummy_basic_usage);
}
