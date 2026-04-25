#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtadc.h>
#include <dtmc_base/dtsemaphore.h>

#include <dtmc_base_demos/demo_adc_print.h>
#include <dtmc_base_demos/demo_helpers.h>

#define TAG demo_name

// the demo's privates
typedef struct demo_t
{
    demo_config_t config;
    dtsemaphore_handle semaphore;
    int32_t scan_count;
} demo_t;

// demo helpers
static dterr_t*
demo__adc_scan_callback_fn(void* self DTADC_SCAN_FN_ARGS);

// --------------------------------------------------------------------------------------
// return a string description of the demo (the returned string is heap allocated)
dterr_t*
demo_describe(demo_t* self, char** out_description)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_description);

    char* d = *out_description;
    char* s = "\n    ";
    d = dtstr_concat_format(d, s, "This demo reads some ADC scans.");

    {
        char t[256];
        DTERR_C(dtadc_to_string(self->config.adc_handle, t, sizeof(t)));
        d = dtstr_concat_format(d, s, "It uses the following ADC: %s.", t);
    }

    d = dtstr_concat_format(
      d, s, "It waits up to %" PRIu32 " ms for %" PRIu32 " scans.", self->config.scan_timeout_ms, self->config.max_scan_count);

    *out_description = d;

cleanup:
    return dterr;
}

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
    DTERR_ASSERT_NOT_NULL(config->adc_handle);

    self->config = *config;

    DTERR_C(dtsemaphore_create(&self->semaphore, 0, 0));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the demo logic reads and prints a few ADC scans, then returns
dterr_t*
demo_start(demo_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // print the description of the demo
    {
        char* description = NULL;
        DTERR_C(dtmc_base_demo_helpers_decorate_description((void*)self, (demo_describe_fn)demo_describe, &description));
        dtlog_info(TAG, "%s", description);
    }

    // activate starts scans coming to callback
    DTERR_C(dtadc_activate(self->config.adc_handle, self, demo__adc_scan_callback_fn));

    dtlog_info(TAG, "scanning started...");

    // wait for a few scans to arrive in the callback
    bool was_timeout = false;
    DTERR_C(dtsemaphore_wait(self->semaphore, self->config.scan_timeout_ms, &was_timeout));
    if (was_timeout)
    {
        dtlog_info(TAG, "timed out waiting for adc scans");
    }

cleanup:
    DTERR_C(dtadc_deactivate(self->config.adc_handle));

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

// --------------------------------------------------------------------------------------
static dterr_t*
demo__adc_scan_callback_fn(void* self DTADC_SCAN_FN_ARGS)
{
    dterr_t* dterr = NULL;
    demo_t* demo = (demo_t*)self;
    DTERR_ASSERT_NOT_NULL(demo);
    DTERR_ASSERT_NOT_NULL(scan);

    dtlog_info(TAG, "adc scan callback: timestamp=%" PRIu64 " ns, channel[0]=%" PRId32, scan->timestamp_ns, scan->channels[0]);

    demo->scan_count++;

    if (demo->scan_count >= demo->config.max_scan_count)
    {
        dtsemaphore_post(demo->semaphore);
    }

cleanup:
    return dterr;
}
