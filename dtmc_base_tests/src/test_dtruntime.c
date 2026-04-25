#include <stdlib.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>

#include <dtcore/dtunittest.h>

#include <dtmc_base/dtruntime.h>

#define TAG "test_dtruntime"

// ----------------------------------------------------------------------------------

static dterr_t*
test_dtmc_base_dtruntime_format_environment(void)
{
    dterr_t* dterr = NULL;

    char* out_string = NULL;
    DTERR_C(dtruntime_format_environment_as_table(&out_string));
    printf("%s\n", out_string);

cleanup:
    dtstr_dispose(out_string);
    return dterr;
}

// ----------------------------------------------------------------------------------

static dterr_t*
test_dtmc_base_dtruntime_milliseconds(void)
{
    dterr_t* dterr = NULL;

    dtruntime_milliseconds_t start = dtruntime_now_milliseconds();
    dtruntime_sleep_milliseconds(100);
    dtruntime_milliseconds_t end = dtruntime_now_milliseconds();
    dtruntime_milliseconds_t elapsed = end - start;

    DTUNITTEST_ASSERT_UINT32(elapsed, >=, 99);
    DTUNITTEST_ASSERT_UINT32(elapsed, <=, 110);

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
void
test_dtmc_base_dtruntime(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtruntime_milliseconds);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtruntime_format_environment);
}
