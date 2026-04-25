#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtinterval.h>
#include <dtmc_base/dtinterval_scheduled.h>
#include <dtmc_base/dtruntime.h>

#define TAG "test_dtinterval_scheduled"

// If you want silence in CI:
#define dtlog_debug(TAG, ...)

// --------------------------------------------------------------------------------------
// Shared callback for normal-paced tests

#define CALLBACK_MAX 3
typedef struct callback_context_t
{
    int call_count;
    int callback_max;
    dtruntime_milliseconds_t start_milliseconds;
    dtruntime_milliseconds_t interval_milliseconds[CALLBACK_MAX];
} callback_context_t;

static dterr_t*
scheduled_interval_callback(void* context, int* should_pause)
{
    dterr_t* dterr = NULL;

    callback_context_t* ctx = (callback_context_t*)context;

    ctx->interval_milliseconds[ctx->call_count] = dtruntime_now_milliseconds();

    if (ctx->call_count == ctx->callback_max - 1)
        *should_pause = 1;

    ctx->call_count++;

    goto cleanup;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_scheduled_basic
//
// Creates a heap-allocated instance, fires 3 ticks at 100ms, and confirms each
// inter-tick delta is within ±20% of the configured interval.

static dterr_t*
test_dtmc_base_dtinterval_scheduled_basic(void)
{
    dterr_t* dterr = NULL;
    dtinterval_handle interval_handle = NULL;

    callback_context_t callback_context = { 0 };
    callback_context.callback_max = CALLBACK_MAX;

    int interval_milliseconds = 100;

    {
        dtinterval_scheduled_t* o = NULL;

        // Create instance
        DTERR_C(dtinterval_scheduled_create(&o));
        interval_handle = (dtinterval_handle)o;

        // Configure instance with known values
        dtinterval_scheduled_config_t c = { 0 };
        c.interval_milliseconds = interval_milliseconds;
        DTERR_C(dtinterval_scheduled_configure(o, &c));
    }

    // set up callback
    DTERR_C(dtinterval_set_callback(interval_handle, scheduled_interval_callback, &callback_context));

    // time before starting (to test that first callback is after one interval)
    callback_context.start_milliseconds = dtruntime_now_milliseconds();

    // start timer
    DTERR_C(dtinterval_start(interval_handle));

    // check results
    DTUNITTEST_ASSERT_INT(callback_context.call_count, ==, 3);

    dtruntime_milliseconds_t last_milliseconds = callback_context.start_milliseconds;
    for (int i = 0; i < CALLBACK_MAX; i++)
    {
        dtlog_debug(TAG,
          "callback %d at %" DTRUNTIME_MILLISECONDS_PRI " milliseconds (delta %" DTRUNTIME_MILLISECONDS_PRI " milliseconds)",
          i,
          callback_context.interval_milliseconds[i],
          callback_context.interval_milliseconds[i] - last_milliseconds);
        dtruntime_milliseconds_t delta = callback_context.interval_milliseconds[i] - last_milliseconds;
        DTUNITTEST_ASSERT_INT(delta, >, interval_milliseconds * 80 / 100);  // allow 20% slop
        DTUNITTEST_ASSERT_INT(delta, <, interval_milliseconds * 120 / 100); // allow 20% slop
        last_milliseconds = callback_context.interval_milliseconds[i];
    }

cleanup:
    dtinterval_dispose(interval_handle);

    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_scheduled_basic failed");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_scheduled_null_guards
//
// Verifies that set_callback rejects a NULL callback pointer and a NULL context
// pointer, returning DTERR_ARGUMENT_NULL in each case.

static dterr_t*
test_dtmc_base_dtinterval_scheduled_null_guards(void)
{
    dterr_t* dterr = NULL;
    dtinterval_scheduled_t* o = NULL;

    DTERR_C(dtinterval_scheduled_create(&o));

    {
        dterr_t* err = dtinterval_scheduled_set_callback(o, NULL, o);
        DTUNITTEST_ASSERT_DTERR(err, DTERR_ARGUMENT_NULL);
    }

    {
        dterr_t* err = dtinterval_scheduled_set_callback(o, scheduled_interval_callback, NULL);
        DTUNITTEST_ASSERT_DTERR(err, DTERR_ARGUMENT_NULL);
    }

cleanup:
    dtinterval_scheduled_dispose(o);
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_scheduled_null_guards failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_scheduled_callback_error
//
// Verifies that an error returned by the callback propagates out of
// dtinterval_scheduled_start with the same error code.

static dterr_t*
deliberate_error_callback(void* context, int* should_pause)
{
    (void)context;
    (void)should_pause;
    return dterr_new(DTERR_DELIBERATE, DTERR_LOC, NULL, "deliberate test failure");
}

static dterr_t*
test_dtmc_base_dtinterval_scheduled_callback_error(void)
{
    dterr_t* dterr = NULL;
    dtinterval_scheduled_t* o = NULL;

    DTERR_C(dtinterval_scheduled_create(&o));

    dtinterval_scheduled_config_t c = { .interval_milliseconds = 10 };
    DTERR_C(dtinterval_scheduled_configure(o, &c));
    DTERR_C(dtinterval_scheduled_set_callback(o, deliberate_error_callback, o));

    {
        dterr_t* err = dtinterval_scheduled_start(o);
        DTUNITTEST_ASSERT_DTERR(err, DTERR_DELIBERATE);
    }

cleanup:
    dtinterval_scheduled_dispose(o);
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_scheduled_callback_error failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_scheduled_phase_stability
//
// Verifies the phase-stable tick-skipping behaviour: when a slow callback
// overruns its interval, the scheduler resumes on the original phase grid
// rather than firing immediately or waiting a full extra interval.
//
// Setup: interval = 100ms.
//   tick 0 fires at ~t0+100ms; callback sleeps 250ms and returns at ~t0+350ms.
//   ticks_elapsed = (350-100)/100 + 1 = 3, so next_tick = t0+400ms.
//   tick 1 must therefore fire ~50ms after tick 0 finishes, not immediately.

#define PHASE_STABILITY_INTERVAL_MILLISECONDS 100
#define PHASE_STABILITY_SLOW_SLEEP_MILLISECONDS 250

typedef struct phase_stability_ctx_t
{
    int call_count;
    dtruntime_milliseconds_t fire_time[2];
    dtruntime_milliseconds_t done_time[2];
} phase_stability_ctx_t;

static dterr_t*
phase_stability_callback(void* context, int* should_pause)
{
    dterr_t* dterr = NULL;
    phase_stability_ctx_t* ctx = (phase_stability_ctx_t*)context;
    int i = ctx->call_count;

    ctx->fire_time[i] = dtruntime_now_milliseconds();

    if (i == 0)
        dtruntime_sleep_milliseconds(PHASE_STABILITY_SLOW_SLEEP_MILLISECONDS);

    ctx->done_time[i] = dtruntime_now_milliseconds();

    if (i == 1)
        *should_pause = 1;

    ctx->call_count++;

    goto cleanup;
cleanup:
    return dterr;
}

static dterr_t*
test_dtmc_base_dtinterval_scheduled_phase_stability(void)
{
    dterr_t* dterr = NULL;
    dtinterval_scheduled_t* o = NULL;
    phase_stability_ctx_t ctx = { 0 };

    DTERR_C(dtinterval_scheduled_create(&o));

    dtinterval_scheduled_config_t c = { .interval_milliseconds = PHASE_STABILITY_INTERVAL_MILLISECONDS };
    DTERR_C(dtinterval_scheduled_configure(o, &c));
    DTERR_C(dtinterval_scheduled_set_callback(o, phase_stability_callback, &ctx));
    DTERR_C(dtinterval_scheduled_start(o));

    DTUNITTEST_ASSERT_INT(ctx.call_count, ==, 2);

    // The gap between tick 0 finishing and tick 1 firing should be ~50ms.
    // If the scheduler fired immediately (no phase correction) the gap would be ~0.
    // If it waited a full extra interval the gap would be ~100ms.
    dtruntime_milliseconds_t gap = ctx.fire_time[1] - ctx.done_time[0];
    DTUNITTEST_ASSERT_INT(gap, >, 20);  // not immediate -- phase correction applied
    DTUNITTEST_ASSERT_INT(gap, <, 100); // not a full extra interval

cleanup:
    dtinterval_scheduled_dispose(o);
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_scheduled_phase_stability failed");
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Runs all tests
void
test_dtmc_base_dtinterval_scheduled(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtinterval_scheduled_basic);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtinterval_scheduled_null_guards);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtinterval_scheduled_callback_error);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtinterval_scheduled_phase_stability);
}
