#include <inttypes.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dtbytes.h>
#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dtsemaphore.h>
#include <dtmc_base/dttasker.h>
#include <dtmc_base/dttasker_registry.h>

#include <dtmc_base/dtbusywork.h>

#define TAG "dtbusywork"
// #define dtlog_debug(TAG, ...)

// structure one for each worker
typedef struct dtbusywork_worker_t
{
    dtbusywork_config_t* config;
    dttasker_handle tasker_handle;
    dtsemaphore_handle trigger_semaphore_handle;
    int32_t worker_number;
    char worker_name[32];
} dtbusywork_worker_t;

// structure for the busywork manager
#define DTBUSYWORK_MAX_WORKERS 10
typedef struct dtbusywork_manager_t
{
    dtbusywork_config_t config;
    dtbusywork_worker_t workers[DTBUSYWORK_MAX_WORKERS];
    dtsemaphore_handle trigger_semaphore_handle;
    dttasker_registry_t tasker_registry;
} dtbusywork_manager_t;

static char*
_worker_to_string(const dtbusywork_worker_t* worker, char* str, int32_t str_size)
{
    snprintf(str,
      str_size,
      "worker @%p #%" PRId32 " \"%s\" task %p",
      worker,
      worker->worker_number,
      worker->worker_name,
      worker->tasker_handle);
    return str;
}

// -------------------------------------------------------------------------------
// this executes the background busy work
extern dterr_t*
dtbusywork__tasker_entrypoint(void* self_arg, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    dtbusywork_worker_t* self = (dtbusywork_worker_t*)self_arg;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(tasker_handle);

#ifndef dtlog_debug
    char worker_str[128] = { 0 };
    dtlog_debug(TAG, "dtbusywork %s ready and waiting for trigger", _worker_to_string(self, worker_str, sizeof(worker_str)));
#endif

    // tell caller we are viable
    DTERR_C(dttasker_ready(tasker_handle));

    // wait for trigger before we start getting busy
    for (;;)
    {
        bool should_stop = false;
        DTERR_C(dttasker_poll(tasker_handle, &should_stop));
        if (should_stop)
            goto cleanup;

        bool was_timeout = false;
        DTERR_C(dtsemaphore_wait(self->trigger_semaphore_handle, 10, &was_timeout));
        if (!was_timeout)
            break;
    }

    dtlog_debug(TAG,
      "dtbusywork %s triggered and starting busy loop with naps at %" DTCPU_MICROSECONDS_PRI " microseconds",
      _worker_to_string(self, worker_str, sizeof(worker_str)),
      self->config->busypoll_microseconds);

    for (;;)
    {
        bool should_stop = false;
        DTERR_C(dttasker_poll(tasker_handle, &should_stop));
        if (should_stop)
        {
            dtlog_debug(TAG, "%s observed stop=true, exiting", _worker_to_string(self, worker_str, sizeof(worker_str)));
            break;
        }

        dtcpu_busywait_microseconds(self->config->busypoll_microseconds);
    }

cleanup:
    dtlog_debug(TAG, "dtbusywork worker %s exiting", _worker_to_string(self, worker_str, sizeof(worker_str)));

    return dterr;
}

// --------------------------------------------------------------------------------------
// create the busywork manager instance
extern dterr_t*
dtbusywork_create(dtbusywork_handle* self_handle, dtbusywork_config_t* config)
{
    dterr_t* dterr = NULL;
    dtbusywork_manager_t* self = NULL;
    DTERR_ASSERT_NOT_NULL(self_handle);
    DTERR_ASSERT_NOT_NULL(config);

    DTERR_C(dtheaper_alloc_and_zero(sizeof(dtbusywork_manager_t), "dtbusywork_t", (void**)&self));

    self->config = *config;

    // default for worker busy poll time
    if (self->config.busypoll_microseconds == 0)
    {
        self->config.busypoll_microseconds = 500; // default to 0.5 millisecond
    }

    // default for worker task priority
    if (self->config.worker_task_priority < 0 || self->config.worker_task_priority >= DTTASKER_PRIORITY__COUNT)
    {
        self->config.worker_task_priority = DTBUSYWORK_WORKER_TASK_PRIORITY_DEFAULT;
    }

    // semaphore for triggering busy workers to start, no max count
    DTERR_C(dtsemaphore_create(&self->trigger_semaphore_handle, 0, 0));

    // registry where we track cpu usage over time
    DTERR_C(dttasker_registry_init(&self->tasker_registry));

    *self_handle = (dtbusywork_handle)self;

cleanup:
    if (dterr != NULL)
    {
        dtbusywork_dispose((dtbusywork_handle)self);
    }
    return dterr;
}

// --------------------------------------------------------------------------------------
// add worker to busywork manager
// makes a task but the task doesn't start being busy until triggered
dterr_t*
dtbusywork_add_worker(dtbusywork_handle self_handle, int32_t core, dttasker_priority_t priority)
{
    dterr_t* dterr = NULL;
    dtbusywork_manager_t* self = (dtbusywork_manager_t*)self_handle;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->trigger_semaphore_handle);

    DTERR_C(dttasker_validate_priority_enum(priority));

    if (priority == DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION)
    {
        priority = DTBUSYWORK_WORKER_TASK_PRIORITY_DEFAULT;
    }

    int i;
    for (i = 0; i < DTBUSYWORK_MAX_WORKERS; i++)
    {
        dtbusywork_worker_t* worker = &self->workers[i];
        // already started in this slot
        if (worker->config != NULL)
            continue;

        worker->worker_number = i + 1;
        sprintf(worker->worker_name, "busywork-%" PRId32, worker->worker_number);
        worker->config = &self->config;
        worker->trigger_semaphore_handle = self->trigger_semaphore_handle;

        dttasker_config_t c = { 0 };
        c.name = worker->worker_name;                            // name of the task
        c.tasker_entry_point_fn = dtbusywork__tasker_entrypoint; // main function for the task
        c.tasker_entry_point_arg = worker;                       // self pointer for the main function
        c.stack_size = 4096;                                     // stack size for the task
        c.core = core;                                           // core affinity for the task
        c.priority = priority;                                   // priority of the task

        // start tasker which gets it established, but it won't do anything until triggered
        dttasker_handle tasker_handle = NULL;
        DTERR_C(dttasker_create(&tasker_handle, &c));

        worker->tasker_handle = tasker_handle;

#ifndef dtlog_debug
        {
            char worker_str[128];
            dtlog_debug(TAG, "added worker %s", _worker_to_string(worker, worker_str, sizeof(worker_str)));
        }
#endif

        DTERR_C(dttasker_start(tasker_handle));
        break;
    }

    if (i == DTBUSYWORK_MAX_WORKERS)
    {
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "maximum number of busywork workers reached");
        goto cleanup;
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtbusywork_start(dtbusywork_handle self_handle)
{
    dterr_t* dterr = NULL;
    dtbusywork_manager_t* self = (dtbusywork_manager_t*)self_handle;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->trigger_semaphore_handle);

    // register system tasks before they start getting busy
    // DTERR_C(dtruntime_register_tasks(&self->tasker_registry));

    for (int i = 0; i < DTBUSYWORK_MAX_WORKERS; i++)
    {
        dtbusywork_worker_t* worker = &self->workers[i];

        // this slot not added?
        if (worker->config == NULL)
            continue;

        // give semaphore once for each added worker to start working
        DTERR_C(dtsemaphore_post(self->trigger_semaphore_handle));
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// take a snapshot of current task cpu usages into the runtime task registry
dterr_t*
dtbusywork_snapshot(dtbusywork_handle self_handle)
{
    dterr_t* dterr = NULL;
    dtbusywork_manager_t* self = (dtbusywork_manager_t*)self_handle;
    DTERR_ASSERT_NOT_NULL(self);

    // register tasks at this moment in time
    DTERR_C(dtruntime_register_tasks(&self->tasker_registry));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// stop busy tasks and dispose them, after this they have to be re-added to start again
dterr_t*
dtbusywork_stop(dtbusywork_handle self_handle)
{
    dterr_t* dterr = NULL;
    dtbusywork_manager_t* self = (dtbusywork_manager_t*)self_handle;
    DTERR_ASSERT_NOT_NULL(self);

#ifndef dtlog_debug
    char worker_str[128];
    dtlog_debug(TAG, "stopping busywork tasks");
#endif

    for (int i = DTBUSYWORK_MAX_WORKERS - 1; i >= 0; i--)
    {
        dtbusywork_worker_t* worker = &self->workers[i];
        if (worker->config == NULL)
            continue;

        DTERR_C(dttasker_stop(worker->tasker_handle));

        // unblock workers still waiting in the pre-trigger semaphore wait
        DTERR_C(dtsemaphore_post(self->trigger_semaphore_handle));

#ifndef dtlog_debug
        dtlog_debug(TAG, "requested stop on worker %s", _worker_to_string(worker, worker_str, sizeof(worker_str)));
#endif
    }

    for (int i = DTBUSYWORK_MAX_WORKERS - 1; i >= 0; i--)
    {
        dtbusywork_worker_t* worker = &self->workers[i];
        if (worker->config == NULL)
            continue;

        DTERR_C(dttasker_join(worker->tasker_handle, 1000, NULL));

#ifndef dtlog_debug
        dtlog_debug(TAG, "joined worker %s", _worker_to_string(worker, worker_str, sizeof(worker_str)));
#endif
    }

    dtlog_debug(TAG, "finished stopping workers");

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtbusywork_summarize(dtbusywork_handle self_handle, char** s)
{
    dterr_t* dterr = NULL;
    dtbusywork_manager_t* self = (dtbusywork_manager_t*)self_handle;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dttasker_registry_format_as_table(&self->tasker_registry, s));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
void
dtbusywork_dispose(dtbusywork_handle self_handle)
{
    dtbusywork_manager_t* self = (dtbusywork_manager_t*)self_handle;
    if (self == NULL)
        return;

    // stop signals and joins threads; dispose then frees the task structs
    dtbusywork_stop(self_handle);

#ifndef dtlog_debug
    char worker_str[128];
#endif

    for (int i = DTBUSYWORK_MAX_WORKERS - 1; i >= 0; i--)
    {
        dtbusywork_worker_t* worker = &self->workers[i];
        if (worker->tasker_handle == NULL)
            continue;

#ifndef dtlog_debug
        dtlog_debug(TAG, "disposing worker %s", _worker_to_string(worker, worker_str, sizeof(worker_str)));
#endif

        dttasker_dispose(worker->tasker_handle);
        memset(worker, 0, sizeof(dtbusywork_worker_t));
    }

    dtlog_debug(TAG, "all busywork tasks disposed");

    dttasker_registry_dispose(&self->tasker_registry);
    dtsemaphore_dispose(self->trigger_semaphore_handle);
    dtheaper_free(self);
}
