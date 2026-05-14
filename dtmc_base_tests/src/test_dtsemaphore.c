// test_dtmc_base_dtsemaphore_base.c
#include <stdbool.h>

#include <dtcore/dterr.h>
#include <dtcore/dtledger.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dttasker.h>

#define TAG "test_dtmc_base_dtsemaphore"

// If you want it silent during CI:
#define dtlog_debug(TAG, ...)

// -------------------------------------------------------------------------------
// task's operating variables
typedef struct task1_t
{
    dtsemaphore_handle semaphore_handle; // semaphore being tested
    dtcpu_t task_busy_timer;
    dtcpu_t post_to_wake_timer;
    int initial_wait_milliseconds; // time before posting the semaphore
    bool did_post;
} task1_t;

// -------------------------------------------------------------------------------
// task main
static dterr_t*
task1_tasker_entrypoint(void* self_arg, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    task1_t* self = (task1_t*)self_arg;

    DTERR_C(dttasker_ready(tasker_handle));

    dtcpu_mark(&self->task_busy_timer);
    dtcpu_busywait_microseconds(self->initial_wait_milliseconds * 1000);
    dtcpu_mark(&self->task_busy_timer);

    dtcpu_mark(&self->post_to_wake_timer);
    self->did_post = true;
    DTERR_C(dtsemaphore_post(self->semaphore_handle));

cleanup:
    dtlog_debug(TAG, "task finished");
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtsemaphore_simple(void)
{
    dterr_t* dterr = NULL;
    dttasker_handle tasker_handle = NULL;

    dtcpu_sysinit();

    // Create & configure semaphore with count=0, max=1
    dtsemaphore_handle semaphore_handle = NULL;
    DTERR_C(dtsemaphore_create(&semaphore_handle, 0, 0));

    // Task setup
    task1_t _task1 = { 0 }, *task1 = &_task1;
    task1->initial_wait_milliseconds = 100;
    task1->semaphore_handle = semaphore_handle;

    {
        dttasker_config_t c = (dttasker_config_t){ 0 };

        c.name = "task1";
        c.tasker_entry_point_fn = task1_tasker_entrypoint;
        c.tasker_entry_point_arg = task1;
        c.stack_size = 4096;                              // pthread may adjust to min
        c.priority = DTTASKER_PRIORITY_BACKGROUND_LOWEST; // lower priority for busy waiter

        DTERR_C(dttasker_create(&tasker_handle, &c));
        DTERR_C(dttasker_start(tasker_handle));
    }

    // Start waiting *now*; record when we started the wait
    dtcpu_t main_wait_timer = { 0 };
    dtcpu_mark(&main_wait_timer);

    // Wait with comfortable timeout (should be released ~100 ms)
    bool was_timeout = false;
    DTERR_C(dtsemaphore_wait(task1->semaphore_handle, 200, &was_timeout));
    dtcpu_mark(&main_wait_timer);
    dtcpu_mark(&task1->post_to_wake_timer);

    // small yield so the task can return cleanly
    dtruntime_sleep_milliseconds(10);

    dtcpu_microseconds_t task_busy_microseconds = dtcpu_elapsed_microseconds(&task1->task_busy_timer);
    dtcpu_microseconds_t elapsed_main_microseconds = dtcpu_elapsed_microseconds(&main_wait_timer);
    dtcpu_microseconds_t post_to_wake_microseconds = dtcpu_elapsed_microseconds(&task1->post_to_wake_timer);

    dtlog_debug(TAG, "task busy %" PRIu64 " microseconds", task_busy_microseconds);
    dtlog_debug(TAG, "semaphore wait %s time out", was_timeout ? "DID" : "did not");
    dtlog_debug(TAG, "semaphore wait woke after total %" PRIu64 " microseconds", elapsed_main_microseconds);
    dtlog_debug(TAG, "time from post to wake was %" PRIu64 " microseconds", post_to_wake_microseconds);

    // --- Assertions ---
    DTUNITTEST_ASSERT_UINT64(task_busy_microseconds, >=, 80 * 1000);
    DTUNITTEST_ASSERT_UINT64(task_busy_microseconds, <=, 300 * 1000);

    // ensure we did not timeout
    DTUNITTEST_ASSERT_TRUE(!was_timeout);

    // Ensure the task actually set posted_time (sanity)
    DTUNITTEST_ASSERT_TRUE(task1->did_post);

    // 1) Post should happen ~100 ms after we began waiting (allow slack)
    //    80–300 ms window accommodates scheduler jitter/VMs.
    DTUNITTEST_ASSERT_UINT64(elapsed_main_microseconds, >=, 80 * 1000);
    DTUNITTEST_ASSERT_UINT64(elapsed_main_microseconds, <=, 300 * 1000);

    // 3) Wake should be reasonably soon after post (scheduler latency bound) 50 ms is generous
    DTUNITTEST_ASSERT_UINT64(post_to_wake_microseconds, <=, 50 * 1000);

cleanup:

    dttasker_dispose(tasker_handle);

    dtsemaphore_dispose(semaphore_handle);

    return dterr;
}

// --------------------------------------------------------------------------------------------
// Entry point for all dtsemaphore unit tests
void
test_dtmc_base_dtsemaphore(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtsemaphore_simple);
}
