#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtguid.h>
#include <dtcore/dtguidable.h>
#include <dtcore/dtguidable_pool.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dttasker.h>
#include <dtmc_base/dttasker_registry.h>

#define TAG "dttasker_registry"

#define dtlog_debug(TAG, ...)

dttasker_registry_t dttasker_registry_global_instance = { 0 };

#define head_format "  %-2s %-48s %-10s %4s %4s %5s  %3s"
#define item_format "  %2" PRIu32 " %-48s %-10s %3d  %3d %6d %3d%%"

// -----------------------------------------------------------------------------
dterr_t*
dttasker_registry_init(dttasker_registry_t* self)
{
    dterr_t* dterr = NULL;

    memset(self, 0, sizeof(*self));

    // max number of entries in task table
    DTERR_C(dtguidable_pool_init(&self->pool, 30));

    self->is_initialized = true;

    goto cleanup;
cleanup:
    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dttasker_registry_insert(dttasker_registry_t* self, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;

    if (!self || !tasker_handle)
        return dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "bad args self=%p tasker_handle=%p", self, tasker_handle);

    dttasker_info_t info;
    DTERR_C(dttasker_get_info(tasker_handle, &info));

    dtguid_t guid;
    DTERR_C(dtguidable_get_guid((dtguidable_handle)tasker_handle, &guid));

    dtguidable_handle found = NULL;
    DTERR_C(dtguidable_pool_search(&self->pool, &guid, &found));
    if (found)
    {
        char guid_str[37];
        dtguid_to_string(&guid, guid_str, sizeof(guid_str));

        return dterr_new(
          DTERR_EXISTS, DTERR_LOC, NULL, "tasker named \"%s\" has same GUID \"%s\" already in registry", info.name, guid_str);
    }

    DTERR_C(dtguidable_pool_insert(&self->pool, (dtguidable_handle)tasker_handle));

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "unable to insert task in tasker registry");
    return dterr;
}

// -----------------------------------------------------------------------------
static dterr_t*
dttasker_registry_format_as_table_for_core(dttasker_registry_t* self, int core, char** out_str)
{
    dterr_t* dterr = NULL;
    // we start with the string that's passed and append to it (if null we start fresh)
    char* s = *out_str;
    char* t = "\n";

    int32_t count;
    DTERR_C(dtguidable_pool_count(&self->pool, &count));
    if (count == 0)
    {
        *out_str = dtstr_concat_format(s, "  %s", "(no tasks)");
        return NULL;
    }

    for (uint32_t i = 0; i < self->pool.max_items; i++)
    {
        dtguidable_handle item = self->pool.items[i];
        if (!item)
            continue;

        dttasker_handle tasker_handle = (dttasker_handle)item;
        dttasker_info_t info;
        DTERR_C(dttasker_get_info(tasker_handle, &info));

        if (info.core != core)
            continue;

        const char* name = info.name ? info.name : "(null)";
        const char* status = dttasker_state_to_string(info.status);

        s = dtstr_concat_format(
          s, t, item_format, i, name, status, info.core, info.priority, info.stack_used_bytes, info.cpu_percent_used);
    }

    *out_str = s;

cleanup:
    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dttasker_registry_format_as_table(dttasker_registry_t* self, char** out_str)
{
    dterr_t* dterr = NULL;
    // we start with the string that's passed and append to it (if null we start fresh)
    char* s = *out_str;
    char* t = "\n";

    if (!self || !out_str)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "bad args self=%p out_str=%p", self, out_str);
        goto cleanup;
    }

    s = dtstr_concat_format(s, t, head_format, "  ", "name", "status", "core", "prio", "stack", "cpu");
    s = dtstr_concat_format(s, t, head_format, "--", "----", "------", "----", "----", "-----", "---");

    for (int core = -1; core <= 1; core++)
    {
        DTERR_C(dttasker_registry_format_as_table_for_core(self, core, &s));
    }

    *out_str = s;

cleanup:
    return dterr;
}

// -----------------------------------------------------------------------------
void
dttasker_registry_dispose(dttasker_registry_t* self)
{
    if (!self)
        return;

    for (uint32_t i = 0; i < self->pool.max_items; i++)
    {
        dtguidable_handle item = self->pool.items[i];
        if (!item)
            continue;

        dttasker_handle tasker_handle = (dttasker_handle)item;

        dttasker_dispose(tasker_handle);
    }

    dtguidable_pool_dispose(&self->pool);
    memset(self, 0, sizeof(*self));
}
