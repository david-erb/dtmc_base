// test_dtmc_base_dtlock_base.c
#include <stdbool.h>

#include <dtcore/dterr.h>
#include <dtcore/dtledger.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dttasker.h>
#include <dtmc_base_tests.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dtlock_base"

// If you want it silent during CI:
// #define dtlog_info(TAG, ...)

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtlock_base_simple(void)
{
    dterr_t* dterr = NULL;

    dttasker_handle tasker1_handle = NULL;
    dttasker_handle tasker2_handle = NULL;

    {
        dttasker_config_t c = (dttasker_config_t){ 0 };
        c.name = "task1";
        c.stack_size = 4096;
        c.priority = DTTASKER_PRIORITY_NORMAL_LOW;
        DTERR_C(dttasker_create(&tasker1_handle, &c));
    }

    {
        dttasker_config_t c = (dttasker_config_t){ 0 };
        c.name = "task2";
        c.stack_size = 4096;
        c.priority = DTTASKER_PRIORITY_NORMAL_LOW;
        DTERR_C(dttasker_create(&tasker2_handle, &c));
    }

    DTERR_C(test_dtmc_base_dtlock_simple(tasker1_handle, tasker2_handle));

cleanup:

    dttasker_dispose(tasker2_handle);
    dttasker_dispose(tasker1_handle);
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtlock_base_stress(void)
{
    dterr_t* dterr = NULL;

    dttasker_handle tasker1_handle = NULL;
    dttasker_handle tasker2_handle = NULL;
    dttasker_handle tasker3_handle = NULL;

    {
        dttasker_config_t c = (dttasker_config_t){ 0 };
        c.name = "task1";
        c.stack_size = 4096;
        c.priority = DTTASKER_PRIORITY_NORMAL_LOW;
        DTERR_C(dttasker_create(&tasker1_handle, &c));
    }

    {
        dttasker_config_t c = (dttasker_config_t){ 0 };
        c.name = "task2";
        c.stack_size = 4096;
        c.priority = DTTASKER_PRIORITY_NORMAL_LOW;
        DTERR_C(dttasker_create(&tasker2_handle, &c));
    }

    {
        dttasker_config_t c = (dttasker_config_t){ 0 };
        c.name = "task3";
        c.stack_size = 4096;
        c.priority = DTTASKER_PRIORITY_NORMAL_LOW;
        DTERR_C(dttasker_create(&tasker3_handle, &c));
    }

    DTERR_C(test_dtmc_base_dtlock_stress(tasker1_handle, tasker2_handle, tasker3_handle));

cleanup:

    dttasker_dispose(tasker3_handle);
    dttasker_dispose(tasker2_handle);
    dttasker_dispose(tasker1_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Entry point for all dtlock unit tests
void
test_dtmc_base_dtlock(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtlock_base_simple);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtlock_base_stress);
}
