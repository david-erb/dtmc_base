#include <dtcore/dterr.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtgpiopin.h>

#include <dtmc_base_demos/demo_gpiopin_blink.h>

#define TAG "dtmc_base_demo_gpiopin_blink"

// the demo's privates
typedef struct dtmc_base_demo_gpiopin_blink_t
{
    // demo configuration
    dtmc_base_demo_gpiopin_blink_config_t config;
} dtmc_base_demo_gpiopin_blink_t;

// --------------------------------------------------------------------------------------
// Create a new instance, allocating memory as needed
dterr_t*
dtmc_base_demo_gpiopin_blink_create(dtmc_base_demo_gpiopin_blink_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    *self = (dtmc_base_demo_gpiopin_blink_t*)malloc(sizeof(dtmc_base_demo_gpiopin_blink_t));
    if (*self == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM,
          DTERR_LOC,
          NULL,
          "failed to allocate %zu bytes for dtmc_base_demo_gpiopin_blink_t",
          sizeof(dtmc_base_demo_gpiopin_blink_t));
        goto cleanup;
    }

    memset(*self, 0, sizeof(dtmc_base_demo_gpiopin_blink_t));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
dtmc_base_demo_gpiopin_blink_configure( //
  dtmc_base_demo_gpiopin_blink_t* self,
  dtmc_base_demo_gpiopin_blink_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    self->config = *config;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the demo logic forever
dterr_t*
dtmc_base_demo_gpiopin_blink_start(dtmc_base_demo_gpiopin_blink_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    // attach the hardware resource
    DTERR_C(dtgpiopin_attach(self->config.gpiopin_handle, NULL, NULL))

    int32_t iteration = 0;
    bool level = false;

    while (true)
    {
        DTERR_C(dtgpiopin_write(self->config.gpiopin_handle, level));
        dtlog_info(TAG, "blink %" PRId32 ": set level to %s", iteration++, level ? "HIGH" : "LOW");

        {
            char* s = NULL;
            DTERR_C(dtgpiopin_concat_format(self->config.gpiopin_handle, s, NULL, &s));
            dtlog_debug(TAG, "%s", s);
            dtstr_dispose(s);
        }

        level = !level;
        dtruntime_sleep_milliseconds(2000);
    }
cleanup:

    return dterr;
}

// --------------------------------------------------------------------------------------
// Stop, unregister and dispose of the demo instance resources
void
dtmc_base_demo_gpiopin_blink_dispose(dtmc_base_demo_gpiopin_blink_t* self)
{
    if (self == NULL)
        return;

    memset(self, 0, sizeof(dtmc_base_demo_gpiopin_blink_t));

    free(self);
}