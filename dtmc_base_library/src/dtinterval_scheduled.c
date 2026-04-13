#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>

#include <dtmc_base/dtinterval.h>
#include <dtmc_base/dtinterval_scheduled.h>

#define TAG "dtinterval_scheduled"

DTINTERVAL_INIT_VTABLE(dtinterval_scheduled);

typedef struct dtinterval_scheduled_t
{
    DTINTERVAL_COMMON_MEMBERS;

    dtinterval_scheduled_config_t config;

    dtinterval_callback_fn callback_fn; // function to call periodically
    void* callback_context;             // context to pass to the callback function

    bool _should_pause;

    bool _is_malloced;

} dtinterval_scheduled_t;

// --------------------------------------------------------------------------------------
extern dterr_t*
dtinterval_scheduled_create(dtinterval_scheduled_t** self_ptr)
{
    dterr_t* dterr = NULL;
    dtinterval_scheduled_t* self = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    DTERR_C(dtheaper_alloc_and_zero(sizeof(dtinterval_scheduled_t), "dtinterval_scheduled_t", (void**)self_ptr));

    self = *self_ptr;
    DTERR_C(dtinterval_scheduled_init(self));

    self->_is_malloced = true;

cleanup:

    if (dterr != NULL)
    {
        dtheaper_free(self);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtinterval_scheduled_init(dtinterval_scheduled_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_INTERVAL_MODEL_ESPIDF;

    // set the vtable for this model number
    DTERR_C(dtinterval_set_vtable(self->model_number, &dtinterval_scheduled_vt));

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "failed to initialize scheduled interval");
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtinterval_scheduled_configure(dtinterval_scheduled_t* self, dtinterval_scheduled_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    self->config = *config;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtinterval_scheduled_set_callback(dtinterval_scheduled_t* self DTINTERVAL_SET_CALLBACK_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(callback);
    DTERR_ASSERT_NOT_NULL(context);

    self->callback_fn = callback;
    self->callback_context = context;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// blocking call to peridically call the callback function
dterr_t*
dtinterval_scheduled_start(dtinterval_scheduled_t* self DTINTERVAL_START_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // Anchor all tick targets to this origin so the phase never drifts.
    const dtruntime_milliseconds_t start_tick = dtruntime_now_milliseconds() + self->config.interval_milliseconds;
    dtruntime_milliseconds_t next_tick = start_tick;

    while (true)
    {
        if (self->_should_pause)
            break;

        // Sleep only the time remaining until the next scheduled tick so that
        // callback execution time does not accumulate into the period.
        dtruntime_milliseconds_t now = dtruntime_now_milliseconds();
        if (now < next_tick)
        {
            dtruntime_sleep_milliseconds(next_tick - now);
        }

        if (self->_should_pause)
            break;

        int should_pause = 0;
        DTERR_C(self->callback_fn(self->callback_context, &should_pause));
        self->_should_pause = should_pause;

        // Recompute the next tick on the original phase, skipping any ticks
        // that were missed while the callback ran.
        dtruntime_milliseconds_t now_after = dtruntime_now_milliseconds();
        dtruntime_milliseconds_t ticks_elapsed = (now_after - start_tick) / self->config.interval_milliseconds + 1;
        next_tick = start_tick + ticks_elapsed * self->config.interval_milliseconds;
    }

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "scheduled interval interrupted");
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtinterval_scheduled_pause(dtinterval_scheduled_t* self DTINTERVAL_PAUSE_ARGS)
{
    dterr_t* dterr = NULL;

    self->_should_pause = true;

    return dterr;
}

// --------------------------------------------------------------------------------------
void
dtinterval_scheduled_dispose(dtinterval_scheduled_t* self)
{
    if (self == NULL)
        return;

    if (self->_is_malloced)
    {
        dtheaper_free(self);
    }
    else
    {
        memset(self, 0, sizeof(*self));
    }
}
