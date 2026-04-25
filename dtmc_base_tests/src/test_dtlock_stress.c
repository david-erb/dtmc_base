#include <stdio.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtlock.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dttasker.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dtlock"

#define dtlog_debug(...)

#define STRESS_ITERATIONS 100000

typedef struct test_dtlock_stress_context_t
{
    dtlock_handle lock_handle;
    int32_t shared_counter;
    int32_t worker_number;
    int32_t worker_iterations[3];
} test_dtlock_stress_context_t;

// ------------------------------------------------------------------------
static dterr_t*
task_stress_worker(void* context, dttasker_handle tasker)
{
    dterr_t* dterr = NULL;
    test_dtlock_stress_context_t* self = context;
    DTERR_ASSERT_NOT_NULL(self);
    int32_t worker_number = self->worker_number++;
    dtlog_debug(TAG, "task_stress_worker %" PRId32 " starting", worker_number);

    DTERR_C(dttasker_ready(tasker));
    dtruntime_sleep_milliseconds(10); // let all tasks get ready

    for (int i = 0; i < STRESS_ITERATIONS; i++)
    {
        self->worker_iterations[worker_number]++;
        DTERR_C(dtlock_acquire(self->lock_handle));
        self->shared_counter++;
        DTERR_C(dtlock_release(self->lock_handle));

        dtruntime_sleep_milliseconds(0); // encourage task switch
    }

cleanup:
    dtlog_debug(TAG, "task_stress_worker %" PRId32 " done at shared_counter=%" PRId32, worker_number, self->shared_counter);
    return dterr;
}

// ------------------------------------------------------------------------
dterr_t*
test_dtmc_base_dtlock_stress(dttasker_handle t1, dttasker_handle t2, dttasker_handle t3)
{
    dterr_t* dterr = NULL;
    test_dtlock_stress_context_t ctx = { 0 };

    DTERR_C(dtlock_create(&ctx.lock_handle));

    DTERR_C(dttasker_set_entry_point(t1, task_stress_worker, &ctx));
    DTERR_C(dttasker_set_entry_point(t2, task_stress_worker, &ctx));
    DTERR_C(dttasker_set_entry_point(t3, task_stress_worker, &ctx));

    DTERR_C(dttasker_start(t1));
    DTERR_C(dttasker_start(t2));
    DTERR_C(dttasker_start(t3));

    dttasker_info_t i1, i2, i3;
    dtruntime_milliseconds_t start_time = dtruntime_now_milliseconds();
    while (true)
    {
        if (dtruntime_now_milliseconds() - start_time > 10000)
        {
            dtlog_debug(TAG,
              "worker_iterations [0]=%" PRId32 ", [1]=%" PRId32 ", [2]=%" PRId32,
              ctx.worker_iterations[0],
              ctx.worker_iterations[1],
              ctx.worker_iterations[2]);
            dterr = dterr_new(DTERR_TIMEOUT, DTERR_LOC, NULL, "dtlock stress test timed out");
            goto cleanup;
        }

        DTERR_C(dttasker_get_info(t1, &i1));
        DTERR_C(dttasker_get_info(t2, &i2));
        DTERR_C(dttasker_get_info(t3, &i3));

        if (i1.status != RUNNING && i2.status != RUNNING && i3.status != RUNNING)
            break;

        dtruntime_sleep_milliseconds(10);
    }

    if (i1.dterr)
    {
        dterr = i1.dterr;
        goto cleanup;
    }
    if (i2.dterr)
    {
        dterr = i2.dterr;
        goto cleanup;
    }
    if (i3.dterr)
    {
        dterr = i3.dterr;
        goto cleanup;
    }

    int32_t expected = 3 * STRESS_ITERATIONS;
    DTUNITTEST_ASSERT_INT(ctx.shared_counter, ==, expected);

cleanup:
    dtlock_dispose(ctx.lock_handle);
    return dterr;
}