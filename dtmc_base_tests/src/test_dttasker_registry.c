#include <stdlib.h>

#include <dtcore/dterr.h>
#include <dtcore/dtguid.h>
#include <dtcore/dtguidable.h>
#include <dtcore/dtguidable_pool.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>
#include <dtcore/dttimeout.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dttasker.h>
#include <dtmc_base/dttasker_registry.h>

#define TAG "test_dtmc_base_dttasker_registry"

// comment out the logging here
// #define dtlog_debug(TAG, ...)

// -------------------------------------------------------------------------------
// tasks's operating variables
typedef struct busy_t
{
    uint64_t busy_microseconds; // how long to stay busy
    const char* name;
    dtsemaphore_handle signal_semaphore;
    dtsemaphore_handle busy_done_semaphore;
} busy_t;

// -------------------------------------------------------------------------------
// this executes the main logic of the task
static dterr_t*
busy_tasker_entrypoint(void* self_arg, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    busy_t* self = (busy_t*)self_arg;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->name);
    DTERR_ASSERT_NOT_NULL(self->signal_semaphore);
    DTERR_ASSERT_NOT_NULL(self->busy_done_semaphore);

    DTERR_C(dttasker_ready(tasker_handle));

    // wait to start
    dtlog_debug(TAG, "%s: waiting for signal to start busy work", self->name);
    DTERR_C(dtsemaphore_wait(self->signal_semaphore, DTTIMEOUT_FOREVER, NULL));

    // hog cpu for a while
    dtlog_debug(TAG, "%s: starting busy work", self->name);
    dtcpu_busywait_microseconds(self->busy_microseconds);

    // signal done
    dtlog_debug(TAG, "%s: done with busy work", self->name);
    DTERR_C(dtsemaphore_post(self->busy_done_semaphore));

    // poll until asked to stop
    dtlog_debug(TAG, "%s: polling until stop requested", self->name);
    bool should_stop = false;
    while (!should_stop)
    {
        DTERR_C(dttasker_poll(tasker_handle, &should_stop));
        if (!should_stop)
            dtruntime_sleep_milliseconds(10);
    }

    goto cleanup;

cleanup:
    dtlog_debug(TAG, "%s: exiting", self->name);

    return dterr;
}

// -------------------------------------------------------------------------------
static dterr_t*
test_dtmc_base_dttasker_registry_busy(void)
{
    dterr_t* dterr = NULL;
    char* registry_table_string = NULL;
    char* environment_table_string = NULL;

    dtsemaphore_handle signal_semaphore = NULL;
    dtsemaphore_handle busy_done_semaphore = NULL;
    dttasker_handle busy1_tasker_handle = NULL;
    dttasker_handle busy2_tasker_handle = NULL;

    dttasker_registry_t busy_time_registry;
    DTERR_C(dttasker_registry_init(&busy_time_registry));

    DTERR_C(dtsemaphore_create(&signal_semaphore, 0, 2));
    DTERR_C(dtsemaphore_create(&busy_done_semaphore, 0, 2));

    // ----
#define BUSY1_MILLISECONDS 200

    busy_t _busy1 = { 0 }, *busy1 = &_busy1;
    busy1->name = "busy1";
    busy1->busy_microseconds = BUSY1_MILLISECONDS * 1000; // 200 milliseconds
    busy1->signal_semaphore = signal_semaphore;
    busy1->busy_done_semaphore = busy_done_semaphore;

    {
        dttasker_config_t c = { 0 };
        c.name = busy1->name;                             // name of the task
        c.tasker_entry_point_fn = busy_tasker_entrypoint; // main function for the task
        c.tasker_entry_point_arg = busy1;                 // self pointer for the main function
        c.stack_size = 4096;                              // stack size for the task
        c.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;     // priority of the task
        c.core = 0;                                       // core affinity for the task

        DTERR_C(dttasker_create(&busy1_tasker_handle, &c));
    }

    // ----
#define BUSY2_MILLISECONDS 200

    busy_t _busy2 = { 0 }, *busy2 = &_busy2;
    busy2->name = "busy2";
    busy2->busy_microseconds = BUSY2_MILLISECONDS * 1000; // 200 milliseconds
    busy2->signal_semaphore = signal_semaphore;
    busy2->busy_done_semaphore = busy_done_semaphore;

    {
        dttasker_config_t c = { 0 };
        c.name = busy2->name;                             // name of the task
        c.tasker_entry_point_fn = busy_tasker_entrypoint; // main function for the task
        c.tasker_entry_point_arg = busy2;                 // self pointer for the main function
        c.stack_size = 4096;                              // stack size for the task
        c.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;     // priority of the task
        c.core = 0;                                       // core affinity for the task

        DTERR_C(dttasker_create(&busy2_tasker_handle, &c));
    }

    dtlog_debug(TAG, "starting busy tasks");
    DTERR_C(dttasker_start(busy1_tasker_handle));
    DTERR_C(dttasker_start(busy2_tasker_handle));

    // register tasks now
    DTERR_C(dtruntime_register_tasks(&busy_time_registry));

    // tell tasks to get busy
    dtlog_debug(TAG, "signalling busy tasks to get busy");
    DTERR_C(dtsemaphore_post(signal_semaphore));
    DTERR_C(dtsemaphore_post(signal_semaphore));

    // wait for tasks to finish busy work
    dtlog_debug(TAG, "waiting for busy tasks to finish");
    {
        bool was_timeout = false;
        DTERR_C(dtsemaphore_wait(busy_done_semaphore, 1000, &was_timeout));
        DTUNITTEST_ASSERT_TRUE(!was_timeout);
        DTERR_C(dtsemaphore_wait(busy_done_semaphore, 1000, &was_timeout));
        DTUNITTEST_ASSERT_TRUE(!was_timeout);
    }

    // register tasks again to update their status
    DTERR_C(dtruntime_register_tasks(&busy_time_registry));

    // request tasks to stop and wait for them to exit
    dtlog_debug(TAG, "stopping busy tasks");
    DTERR_C(dttasker_stop(busy1_tasker_handle));
    DTERR_C(dttasker_stop(busy2_tasker_handle));

    {
        bool was_timeout = false;
        DTERR_C(dttasker_join(busy1_tasker_handle, 1000, &was_timeout));
        DTUNITTEST_ASSERT_TRUE(!was_timeout);
        DTERR_C(dttasker_join(busy2_tasker_handle, 1000, &was_timeout));
        DTUNITTEST_ASSERT_TRUE(!was_timeout);
    }

    // tasks should be finished with no error
    {
        dttasker_info_t state;

        DTERR_C(dttasker_get_info(busy1_tasker_handle, &state));
        DTUNITTEST_ASSERT_INT(state.status, ==, STOPPED);
        DTUNITTEST_ASSERT_NULL(state.dterr);

        DTERR_C(dttasker_get_info(busy2_tasker_handle, &state));
        DTUNITTEST_ASSERT_INT(state.status, ==, STOPPED);
        DTUNITTEST_ASSERT_NULL(state.dterr);
    }

    //  ==== the measured registered tasks ====
    {
        char* s = NULL;
        DTERR_C(dttasker_registry_format_as_table(&busy_time_registry, &s));
        dtlog_debug(TAG, "tasks as measured during busy time:\n%s", s);
        dtstr_dispose(s);
    }

cleanup:
    if (dterr != NULL)
    {
        dttasker_stop(busy1_tasker_handle);
        dttasker_stop(busy2_tasker_handle);
        bool was_timeout = false;
        dttasker_join(busy1_tasker_handle, 500, &was_timeout);
        dttasker_join(busy2_tasker_handle, 500, &was_timeout);
    }

    // dispose tasks before global registry so auto-deregister sees a live registry
    dttasker_dispose(busy1_tasker_handle);
    dttasker_dispose(busy2_tasker_handle);
    dttasker_registry_dispose(&dttasker_registry_global_instance);
    dtstr_dispose(registry_table_string);
    dtstr_dispose(environment_table_string);
    dttasker_registry_dispose(&busy_time_registry);
    dtsemaphore_dispose(signal_semaphore);
    dtsemaphore_dispose(busy_done_semaphore);

    return dterr;
}

// --------------------------------------------------------------------------------------------

void
test_dtmc_base_dttasker_registry(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttasker_registry_busy);
}
