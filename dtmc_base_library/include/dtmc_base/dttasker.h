/*
 * dttasker -- Cross-platform task facade with priority bands and CPU accounting.
 *
 * Manages the full lifecycle of a task: create, start, ready signaling,
 * cooperative stop request, join with timeout, and dispose. A three-band
 * priority system (background, normal, urgent) maps to platform-specific
 * scheduler priorities at implementation time. The info structure exposes
 * stack usage, core affinity, cumulative CPU microseconds, and percent
 * utilization, enabling runtime diagnostics without platform-specific
 * profiling tools.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtguid.h>
#include <dtcore/dtguidable.h>
#include <dtcore/dttimeout.h>

#include <dtcore/dtledger.h>

#include <dtmc_base/dtcpu.h>

DTLEDGER_DECLARE(dttasker)

// opaque handle for dispatch calls
struct dttasker_handle_t;
typedef struct dttasker_handle_t* dttasker_handle;

typedef enum
{
    UNINITIALIZED = 0, // not initialized
    INITIALIZED,       // initialized but not started
    STARTING,          // starting the task
    RUNNING,           // task is running
    STOPPED,           // task has stopped
} dttasker_status_t;

// --------------------------------------------------------------------------------
// priority levels
typedef enum dttasker_priority_t
{
    // default chosen by particular situation
    DTTASKER_PRIORITY_DEFAULT_FOR_SITUATION = 0,

    // enumeration start marker
    DTTASKER_PRIORITY__START = 100,

    // BACKGROUND band
    DTTASKER_PRIORITY_BACKGROUND_LOWEST,
    DTTASKER_PRIORITY_BACKGROUND_LOW,
    DTTASKER_PRIORITY_BACKGROUND_MEDIUM,
    DTTASKER_PRIORITY_BACKGROUND_HIGH,
    DTTASKER_PRIORITY_BACKGROUND_HIGHEST,

    // NORMAL band
    DTTASKER_PRIORITY_NORMAL_LOWEST,
    DTTASKER_PRIORITY_NORMAL_LOW,
    DTTASKER_PRIORITY_NORMAL_MEDIUM,
    DTTASKER_PRIORITY_NORMAL_HIGH,
    DTTASKER_PRIORITY_NORMAL_HIGHEST,

    // URGENT band (still a task/thread, not an ISR)
    DTTASKER_PRIORITY_URGENT_LOWEST,
    DTTASKER_PRIORITY_URGENT_LOW,
    DTTASKER_PRIORITY_URGENT_MEDIUM,
    DTTASKER_PRIORITY_URGENT_HIGH,
    DTTASKER_PRIORITY_URGENT_HIGHEST,

    DTTASKER_PRIORITY__COUNT
} dttasker_priority_t;

// --------------------------------------------------------------------------------
// information about a tasker
typedef struct dttasker_info_t
{
    char* name;
    char _name[32]; // storage for name
    dttasker_status_t status;
    dterr_t* dterr;
    int core;
    dttasker_priority_t priority;
    int stack_used_bytes;
    int stack_total_bytes;
    dtcpu_microseconds_t used_microseconds; // cumululative used cpu at last time we checked
    dtcpu_microseconds_t time_microseconds; // last time we checked
    int cpu_percent_used;                   // percent cpu used at the last time we checked
} dttasker_info_t;

// callbacks
typedef dterr_t* (*dttasker_entry_point_fn)(void* self, dttasker_handle self_handle);
typedef dterr_t* (*dttasker_info_callback_t)(void* caller_object, dttasker_info_t* tasker_info);

typedef struct dttasker_config_t
{
    const char* name;                              // name of the task
    dttasker_entry_point_fn tasker_entry_point_fn; // main function for the task
    void* tasker_entry_point_arg;                  // self pointer for the main function
    int32_t stack_size;                            // stack size for the task
    dttasker_priority_t priority;                  // priority of the task
    int32_t core;                                  // core affinity for the task

    // optional for status changes on this tasker
    dttasker_info_callback_t tasker_info_callback;
    void* tasker_info_callback_context;
} dttasker_config_t;

// --------------------------------------------------------------------------------------
// create a new dttasker instance, allocating memory as needed and access system resources
extern dterr_t*
dttasker_create(dttasker_handle* self_handle, dttasker_config_t* config);
// --------------------------------------------------------------------------------------
// set the entry point function and argument to be called when the task starts
extern dterr_t*
dttasker_set_entry_point(dttasker_handle self_handle, dttasker_entry_point_fn entry_point_function, void* entry_point_arg);
// --------------------------------------------------------------------------------------
// start the task running, creating the thread
extern dterr_t*
dttasker_start(dttasker_handle self_handle);
// --------------------------------------------------------------------------------------
// signal that the task is ready (called from client code within the task)
extern dterr_t*
dttasker_ready(dttasker_handle self_handle);
// --------------------------------------------------------------------------------------
// request stop of the task (called from outside the task)
dterr_t*
dttasker_stop(dttasker_handle self_handle);
// --------------------------------------------------------------------------------------
// wait for task to stop (called from outside the task)
dterr_t*
dttasker_join(dttasker_handle self_handle, dttimeout_millis_t timeout_millis, bool* was_timeout);
// --------------------------------------------------------------------------------------
// poll if task is supposed to stop (called from client code within the task)
dterr_t*
dttasker_poll(dttasker_handle self_handle, bool* should_stop);
// --------------------------------------------------------------------------------------
extern dterr_t*
dttasker_set_priority(dttasker_handle self_handle, dttasker_priority_t priority);
// --------------------------------------------------------------------------------------
// get current task info/status
extern dterr_t*
dttasker_get_info(dttasker_handle self_handle, dttasker_info_t* info);
// --------------------------------------------------------------------------------------
// set current task info/status
extern dterr_t*
dttasker_set_info(dttasker_handle self_handle, dttasker_info_t* info);
// ------------------------------------------------------------------------
extern dterr_t*
dttasker_get_guid(dttasker_handle self_handle, dtguid_t* guid);
// --------------------------------------------------------------------------------------
extern void
dttasker_dispose(dttasker_handle self_handle);
// --------------------------------------------------------------------------------------
// helpers
extern const char*
dttasker_state_to_string(dttasker_status_t status);

extern const char*
dttasker_priority_enum_to_string(dttasker_priority_t p);

extern bool
dttasker_priority_enum_is_urgent(dttasker_priority_t p);
extern bool
dttasker_priority_enum_is_normal(dttasker_priority_t p);
extern bool
dttasker_priority_enum_is_background(dttasker_priority_t p);
extern bool
dttasker_priority_enum_is_valid(dttasker_priority_t p);
extern dterr_t*
dttasker_validate_priority_enum(dttasker_priority_t p);

// --------------------------------------------------------------------------------------
// dtguidable api
DTGUIDABLE_DECLARE_API_EX(dttasker, _handle);