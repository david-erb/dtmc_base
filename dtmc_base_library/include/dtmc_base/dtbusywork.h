/*
 * dtbusywork -- CPU load generator for per-core utilization measurement.
 *
 * Spawns configurable background tasks that spin continuously at the lowest
 * scheduler priority, consuming idle CPU time. A snapshot function samples
 * cumulative dttasker CPU-usage counters; subtracting busy-idle from total
 * elapsed time yields an accurate utilization figure even on RTOS schedulers
 * that lack built-in profiling. Summarize produces a human-readable report.
 *
 * cdox v1.0.2
 */

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>

#include <dtmc_base/dtcpu.h>

#include <dtmc_base/dttasker.h>

#define DTBUSYWORK_WORKER_TASK_PRIORITY_DEFAULT DTTASKER_PRIORITY_BACKGROUND_LOWEST

typedef struct dtbusywork_config_t
{
    // how long to stay busy between checking stop signal
    dtcpu_microseconds_t busypoll_microseconds;

    // busywork worker task priority
    dttasker_priority_t worker_task_priority;
} dtbusywork_config_t;

// -----------------------------------------------------------------------------
// Opaque handles

// Opaque handle used by dispatcher functions.
struct dtbusywork_handle_t;
typedef struct dtbusywork_handle_t* dtbusywork_handle;

// -----------------------------------------------------------------------------
// functions
extern dterr_t*
dtbusywork_create(dtbusywork_handle* self, dtbusywork_config_t* config);

// add busy worker, but don't start it yet
extern dterr_t*
dtbusywork_add_worker(dtbusywork_handle self_handle, int32_t core, dttasker_priority_t priority);

// kick off the busywork tasks
extern dterr_t*
dtbusywork_start(dtbusywork_handle self);

// take a snapshot of current task cpu usages into the runtime task registry
extern dterr_t*
dtbusywork_snapshot(dtbusywork_handle self_handle);

// stop the busywork tasks
extern dterr_t*
dtbusywork_stop(dtbusywork_handle self);

// make a string summary of busywork cpu usage
extern dterr_t*
dtbusywork_summarize(dtbusywork_handle self, char** s);

// dispose busywork manager and its resources
extern void
dtbusywork_dispose(dtbusywork_handle self);
