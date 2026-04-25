#include <stdio.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtledger.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtlock.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dttasker.h>

#include <dtmc_base_tests.h>

extern dtledger_t* dterr_ledger;

#define TAG "test_dtmc_base_dtlock"

typedef struct test_dtlock_context_t
{
    dtlock_handle lock_handle;
    dtruntime_milliseconds_t release1_time;
    dtruntime_milliseconds_t acquire2_time;
} test_dtlock_context_t;

//------------------------------------------------------------------------
dterr_t*
task1_entrypoint(void* context, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    test_dtlock_context_t* self = (test_dtlock_context_t*)context;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dtlock_acquire(self->lock_handle));
    DTERR_C(dttasker_ready(tasker_handle));
    dtruntime_now_milliseconds(100);
    self->release1_time = dtruntime_now_milliseconds();

cleanup:
    dtlock_release(self->lock_handle);
    return dterr;
}

//------------------------------------------------------------------------
dterr_t*
task2_entrypoint(void* context, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    test_dtlock_context_t* self = (test_dtlock_context_t*)context;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dttasker_ready(tasker_handle));
    DTERR_C(dtlock_acquire(self->lock_handle));
    self->acquire2_time = dtruntime_now_milliseconds();

cleanup:
    dtlock_release(self->lock_handle);
    return dterr;
}

//------------------------------------------------------------------------
// Example: lock demonstration between two tasks

dterr_t*
test_dtmc_base_dtlock_simple(dttasker_handle tasker1, dttasker_handle tasker2)
{
    dterr_t* dterr = NULL;
    test_dtlock_context_t _self = { 0 }, *self = &_self;
    DTERR_C(dtlock_create(&self->lock_handle));

    DTERR_C(dttasker_set_entry_point(tasker1, task1_entrypoint, self));

    DTERR_C(dttasker_set_entry_point(tasker2, task2_entrypoint, self));

    DTERR_C(dttasker_start(tasker1));

    DTERR_C(dttasker_start(tasker2));

    dttasker_info_t info1, info2;
    while (true)
    {
        DTERR_C(dttasker_get_info(tasker1, &info1));
        DTERR_C(dttasker_get_info(tasker2, &info2));

        if (info1.status != RUNNING && info2.status != RUNNING)
            break;

        dtruntime_sleep_milliseconds(100);
    }

    if (info1.dterr != NULL)
    {
        dterr = info1.dterr;
        goto cleanup;
    }

    if (info2.dterr != NULL)
    {
        dterr = info2.dterr;
        goto cleanup;
    }

    // Verify that task2 acquired the lock after task1 released it.
    DTUNITTEST_ASSERT_INT(self->acquire2_time, >=, self->release1_time);

cleanup:

    dttasker_info_t info;
    dttasker_get_info(tasker1, &info);
    if (info.dterr != NULL)
    {
        dtlog_error(TAG, "task1 error: %s", info.dterr->message);
        dterr_dispose(info.dterr);
    }

    dttasker_get_info(tasker2, &info);
    if (info.dterr != NULL)
    {
        dtlog_error(TAG, "task2 error: %s", info.dterr->message);
        dterr_dispose(info.dterr);
    }
    dtlock_dispose(self->lock_handle);

    return dterr;
}
