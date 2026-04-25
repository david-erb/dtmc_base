// test_dtmc_linux_dtbufferqueue_linux.c
#include <stdbool.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtcore/dttimeout.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dttasker.h>

#define TAG "app_main"

// If you want silence in CI:
// #define dtlog_debug(TAG, ...)

// -------------------------------------------------------------------------------
// task's operating variables
typedef struct task1_t
{
    int initial_get_milliseconds;            // delay before producing
    dtbufferqueue_handle bufferqueue_handle; // queue under test
    dtbuffer_t buffer;                       // buffer object to enqueue
    int buffer_value;                        // payload wrapped by buffer
    dtcpu_t main_waited_timing;
    dtcpu_t task_worked_timing;
} task1_t;

// -------------------------------------------------------------------------------
// producer task entry
static dterr_t*
task1_tasker_entrypoint(void* self_arg, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    task1_t* self = (task1_t*)self_arg;

    dtcpu_microseconds_t initial_get_microseconds = self->initial_get_milliseconds * 1000ULL;
    dtlog_debug(TAG, "task1 starting, will wait %" DTCPU_MICROSECONDS_PRI " ms before producing", initial_get_microseconds);

    // signal ready so the test can proceed
    DTERR_C(dttasker_ready(tasker_handle));

    // simulate some setup work, hog cpu for a while
    dtcpu_mark(&self->task_worked_timing);
    dtcpu_busywait_microseconds(initial_get_microseconds);
    dtcpu_mark(&self->task_worked_timing);

    // wrap a pointer payload into a dtbuffer_t that lives in this task struct
    DTERR_C(dtbuffer_wrap(&self->buffer, &self->buffer_value, sizeof(self->buffer_value)));

    // enqueue the buffer
    // self->micros_at_put = dtcpu_now_microseconds();
    DTERR_C(dtbufferqueue_put(self->bufferqueue_handle, &self->buffer, DTTIMEOUT_FOREVER, NULL));

cleanup:
    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dtbufferqueue_simple(void)
{
    dterr_t* dterr = NULL;
    dttasker_handle tasker_handle = NULL;
    dtbufferqueue_handle bufferqueue_handle = NULL;

    DTERR_C(dtcpu_sysinit());

    // Create & configure the queue (capacity 1)
    DTERR_C(dtbufferqueue_create(&bufferqueue_handle, 1, false));

    // Start producer task
    task1_t _task1 = { 0 }, *task1 = &_task1;
    task1->initial_get_milliseconds = 100;
    task1->bufferqueue_handle = bufferqueue_handle;

    {
        dttasker_config_t c = (dttasker_config_t){ 0 };
        c.name = "task1";
        c.tasker_entry_point_fn = task1_tasker_entrypoint;
        c.tasker_entry_point_arg = task1;
        c.stack_size = 4096;                        // pthreads may raise to minimum
        c.priority = DTTASKER_PRIORITY_URGENT_HIGH; // higher priority than main

        DTERR_C(dttasker_create(&tasker_handle, &c));
    }

    dtcpu_mark(&task1->main_waited_timing);
    DTERR_C(dttasker_start(tasker_handle));

    // Consumer: wait a longer time for the buffer
    dtbuffer_t* buffer = NULL;
    DTERR_C(dtbufferqueue_get(bufferqueue_handle, &buffer, 500, NULL));
    dtcpu_mark(&task1->main_waited_timing);

    // We should have gotten the exact same dtbuffer_t* the producer enqueued
    DTUNITTEST_ASSERT_PTR(buffer, ==, &task1->buffer);

    dtcpu_microseconds_t waited_microseconds = dtcpu_elapsed_microseconds(&task1->main_waited_timing);
    dtcpu_microseconds_t worked_microseconds = dtcpu_elapsed_microseconds(&task1->task_worked_timing);

    // Basic timing sanity:
    // 1) Producer posted roughly ~100ms after we began waiting (allow slack)
    DTUNITTEST_ASSERT_UINT64(worked_microseconds, >=, 80 * 1000ULL);
    DTUNITTEST_ASSERT_UINT64(worked_microseconds, <=, 120 * 1000ULL);

    // 2) We woke after the post
    DTUNITTEST_ASSERT_UINT64(waited_microseconds, >=, 80 * 1000ULL);
    DTUNITTEST_ASSERT_UINT64(waited_microseconds, <=, 120 * 1000ULL);
    dtlog_debug(TAG,
      "task_worked=%" DTCPU_MICROSECONDS_PRI "us, main_waited=%" DTCPU_MICROSECONDS_PRI "us",
      worked_microseconds,
      waited_microseconds);

    // wait again, this one should time out
    dtcpu_mark(&task1->main_waited_timing);
    bool was_timeout = false;
    dterr = dtbufferqueue_get(bufferqueue_handle, &buffer, 100, &was_timeout);
    dtcpu_mark(&task1->main_waited_timing);
    DTUNITTEST_ASSERT_TRUE(was_timeout);
    DTUNITTEST_ASSERT_UINT64(waited_microseconds, >=, 80 * 1000ULL);
    DTUNITTEST_ASSERT_UINT64(waited_microseconds, <=, 120 * 1000ULL);

cleanup:
    // give the producer a moment to exit and the tasker to mark STOPPED
    dtruntime_sleep_milliseconds(1);

    dttasker_dispose(tasker_handle);
    dtbufferqueue_dispose(bufferqueue_handle);

    return dterr;
}

// --------------------------------------------------------------------------------------------
// Entry point for all dtbufferqueue unit tests
void
test_dtmc_base_dtbufferqueue(DTUNITTEST_SUITE_ARGS)
{

    DTUNITTEST_RUN_TEST(test_dtmc_base_dtbufferqueue_simple);
}
