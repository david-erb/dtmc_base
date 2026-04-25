#include <dtcore/dterr.h>

#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtpicosdk_helper.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dttasker.h>
#include <dtmc_base/dttasker_registry.h>

#include <dtmc_base_demos/demo_runtime_info.h>

#define TAG "demo"

// the demo's privates
typedef struct demo_t
{
    demo_config_t config;
} demo_t;

// --------------------------------------------------------------------------------------
// Create a new instance, allocating memory as needed
dterr_t*
demo_create(demo_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dtheaper_alloc_and_zero(sizeof(demo_t), "demo_t", (void**)self));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
demo_configure( //
  demo_t* self,
  demo_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    self->config = *config;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the demo logic, typically returning leaving tasks running and callbacks registered
dterr_t*
demo_start(demo_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    char* s = NULL;

    // ==== the runtime environment ====
    {
        DTERR_C(dtruntime_format_environment_as_table(&s));
        dtlog_info(TAG, "Environment:\n%s", s);
        dtstr_dispose(s);
        s = NULL;
    }

    //  ==== the currently registered tasks ====
    {
        DTERR_C(dttasker_set_priority(NULL, DTTASKER_PRIORITY_NORMAL_MEDIUM));
        DTERR_C(dtruntime_register_tasks(&dttasker_registry_global_instance));
        DTERR_C(dttasker_registry_format_as_table(&dttasker_registry_global_instance, &s));
        dtlog_info(TAG, "Tasks:\n%s", s);
        dtstr_dispose(s);
        s = NULL;
    }

    // ==== the currently registered devices ====
    {
        DTERR_C(dtruntime_format_devices_as_table(&s));
        dtlog_info(TAG, "Devices:\n%s", s);
        dtstr_dispose(s);
        s = NULL;
    }

cleanup:
    dtstr_dispose(s);
    return dterr;
}

// --------------------------------------------------------------------------------------
// Stop, unregister and dispose of the demo instance resources
void
demo_dispose(demo_t* self)
{
    if (self == NULL)
        return;

    dtheaper_free(self);
}