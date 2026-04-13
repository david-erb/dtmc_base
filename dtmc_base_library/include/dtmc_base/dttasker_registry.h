/*
 * dttasker_registry -- Global registry for tracking active dttasker instances.
 *
 * Maintains a dtguidable_pool of dttasker handles, supporting insertion,
 * name-based lookup, task count, iteration, and formatted table output. A
 * global singleton (dttasker_registry_global_instance) lets dtruntime
 * enumerate all known tasks for diagnostics without requiring a registry
 * pointer to be threaded through the call stack.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <dtcore/dtguidable_pool.h>

#include <dtcore/dterr.h>
#include <dtmc_base/dttasker.h>

typedef struct dttasker_registry_t
{
    dtguidable_pool_t pool;
    bool is_initialized;
} dttasker_registry_t;

extern dttasker_registry_t dttasker_registry_global_instance;

extern dterr_t*
dttasker_registry_init(dttasker_registry_t* self);

extern dterr_t*
dttasker_registry_insert(dttasker_registry_t* self, dttasker_handle tasker_handle);

extern dterr_t*
dttasker_registry_format_as_table(dttasker_registry_t* self, char** out_str);

extern void
dttasker_registry_dispose(dttasker_registry_t* self);

// -------------------------------------------------------------------------------
// Iterate over all known tasks. (subjects with non-empty name).
// Return true from the callback to continue, false to stop early.
typedef bool (*dttasker_registry_task_iter_t)(void* ctx, dttasker_handle tasker_handle);

extern dterr_t*
dttasker_registry_foreach_task(dttasker_registry_t* self, dttasker_registry_task_iter_t it, void* ctx);

// Get the number of recipients currently subscribed to a task.
// If the task does not exist yet, *count_out = 0 and no error.
extern dterr_t*
dttasker_registry_task_count(dttasker_registry_t* self, int* count_out);

// Returns true if a task exists (has a name set), regardless of recipient count.
extern dterr_t*
dttasker_registry_has_task_by_name(dttasker_registry_t* self, const char* task_name, bool* has_out);
