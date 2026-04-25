#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtbusywork.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dtbusywork"

#define dtlog_debug(TAG, ...)

// ------------------------------------------------------------------------
// Example: basic one-shot encode/decode using the dtbusywork facade.

static dterr_t*
test_dtmc_base_dtbusywork_simple_example_basic_functioning(void)
{
    dterr_t* dterr = NULL;
    dtbusywork_handle busywork_handle = NULL;
    char* summary = NULL;

    {
        dtbusywork_config_t c = { 0 };
        DTERR_C(dtbusywork_create(&busywork_handle, &c));
    }

    DTERR_C(dtbusywork_add_worker(busywork_handle, 0, DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION));
    DTERR_C(dtbusywork_add_worker(busywork_handle, 0, DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION));
    DTERR_C(dtbusywork_add_worker(busywork_handle, 0, DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION));

    DTERR_C(dtbusywork_start(busywork_handle));

    // let them run for a bit
    dtruntime_sleep_milliseconds(1000);

    // snapshot the cpu usage
    dtlog_debug(TAG, "taking snapshot of busywork tasks");
    DTERR_C(dtbusywork_snapshot(busywork_handle));

    // stop the busywork tasks
    dtlog_debug(TAG, "stopping busywork tasks");
    DTERR_C(dtbusywork_stop(busywork_handle));

    {
        DTERR_C(dtbusywork_summarize(busywork_handle, &summary));
        dtlog_info(TAG, "busywork summary:\n%s", summary);

        // summary should report these worker names
        DTUNITTEST_ASSERT_HAS_SUBSTRING(summary, "busywork-1");
        DTUNITTEST_ASSERT_HAS_SUBSTRING(summary, "busywork-2");
        DTUNITTEST_ASSERT_HAS_SUBSTRING(summary, "busywork-3");
    }

cleanup:
    dtstr_dispose(summary);
    dtbusywork_dispose(busywork_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Suite entry point

void
test_dtmc_base_dtbusywork(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtbusywork_simple_example_basic_functioning);
}
