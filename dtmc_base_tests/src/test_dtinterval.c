#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtinterval.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dtinterval"

// --------------------------------------------------------------------------------------
// Shared tick-recording callback

#define TICK_MAX 3

typedef struct tick_ctx_t
{
    int                      count;
    int                      max;
    dtruntime_milliseconds_t fire_time[TICK_MAX];
} tick_ctx_t;

static dterr_t*
tick_callback(void* context, int* should_pause)
{
    tick_ctx_t* ctx = (tick_ctx_t*)context;
    if (ctx->count < TICK_MAX)
        ctx->fire_time[ctx->count] = dtruntime_now_milliseconds();
    ctx->count++;
    if (ctx->count >= ctx->max)
        *should_pause = 1;
    return NULL;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_ticks
//
// Simple end-to-end demonstration: fire 3 ticks via the callback mechanism and
// confirm each inter-tick gap is within ±20% of the configured period.
// Suitable as a first example of how to use any dtinterval implementation.

dterr_t*
test_dtmc_base_dtinterval_ticks(dtinterval_handle h, int32_t interval_ms)
{
    dterr_t* dterr = NULL;
    tick_ctx_t ctx = { .max = TICK_MAX };

    dtruntime_milliseconds_t t0 = dtruntime_now_milliseconds();

    DTERR_C(dtinterval_set_callback(h, tick_callback, &ctx));
    DTERR_C(dtinterval_start(h)); // blocks until ctx.count reaches ctx.max

    DTUNITTEST_ASSERT_INT(ctx.count, ==, TICK_MAX);

    // first tick should fire ~interval_ms after start()
    dtruntime_milliseconds_t delta0 = ctx.fire_time[0] - t0;
    DTUNITTEST_ASSERT_INT(delta0, >, interval_ms * 80 / 100);
    DTUNITTEST_ASSERT_INT(delta0, <, interval_ms * 120 / 100);

    // subsequent ticks should each be ~interval_ms apart
    for (int i = 1; i < TICK_MAX; i++)
    {
        dtruntime_milliseconds_t delta = ctx.fire_time[i] - ctx.fire_time[i - 1];
        DTUNITTEST_ASSERT_INT(delta, >, interval_ms * 80 / 100);
        DTUNITTEST_ASSERT_INT(delta, <, interval_ms * 120 / 100);
    }

cleanup:
    if (dterr)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_ticks failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_null_guards
//
// Confirms that set_callback rejects a NULL callback pointer and a NULL context
// pointer, returning DTERR_ARGUMENT_NULL in each case.

dterr_t*
test_dtmc_base_dtinterval_null_guards(dtinterval_handle h)
{
    dterr_t* dterr = NULL;

    {
        dterr_t* err = dtinterval_set_callback(h, NULL, h);
        DTUNITTEST_ASSERT_DTERR(err, DTERR_ARGUMENT_NULL);
    }

    {
        dterr_t* err = dtinterval_set_callback(h, tick_callback, NULL);
        DTUNITTEST_ASSERT_DTERR(err, DTERR_ARGUMENT_NULL);
    }

cleanup:
    if (dterr)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_null_guards failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_callback_error_propagates
//
// Confirms that an error returned by the callback surfaces from start()
// with the same error code.

static dterr_t*
deliberate_error_callback(void* context, int* should_pause)
{
    (void)context;
    (void)should_pause;
    return dterr_new(DTERR_DELIBERATE, DTERR_LOC, NULL, "deliberate test failure");
}

dterr_t*
test_dtmc_base_dtinterval_callback_error_propagates(dtinterval_handle h)
{
    dterr_t* dterr = NULL;

    DTERR_C(dtinterval_set_callback(h, deliberate_error_callback, h));

    {
        dterr_t* err = dtinterval_start(h);
        DTUNITTEST_ASSERT_DTERR(err, DTERR_DELIBERATE);
    }

cleanup:
    if (dterr)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_callback_error_propagates failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_accuracy_under_load
//
// Key guard for hardware-backed implementations: confirms that callback
// execution time does NOT accumulate into the tick period.
//
// With interval_ms=50 and 30ms of work per callback, a software-sleep-based
// scheduler produces 80ms gaps.  A hardware timer fires on its own schedule,
// so gaps stay at ~50ms.  The upper bound (130% of interval) is tight enough
// to catch the 80ms software-accumulation behaviour.

#define ACCURACY_WORK_MS 30

typedef struct load_ctx_t
{
    int                      count;
    int                      max;
    dtruntime_milliseconds_t fire_time[TICK_MAX];
    int32_t                  work_ms;
} load_ctx_t;

static dterr_t*
load_callback(void* context, int* should_pause)
{
    load_ctx_t* ctx = (load_ctx_t*)context;
    if (ctx->count < TICK_MAX)
        ctx->fire_time[ctx->count] = dtruntime_now_milliseconds();
    ctx->count++;
    if (ctx->count >= ctx->max)
        *should_pause = 1;
    else
        dtruntime_sleep_milliseconds(ctx->work_ms);
    return NULL;
}

dterr_t*
test_dtmc_base_dtinterval_accuracy_under_load(dtinterval_handle h, int32_t interval_ms)
{
    dterr_t* dterr = NULL;
    load_ctx_t ctx = { .max = TICK_MAX, .work_ms = ACCURACY_WORK_MS };

    DTERR_C(dtinterval_set_callback(h, load_callback, &ctx));
    DTERR_C(dtinterval_start(h));

    DTUNITTEST_ASSERT_INT(ctx.count, ==, TICK_MAX);

    for (int i = 1; i < TICK_MAX; i++)
    {
        dtruntime_milliseconds_t delta = ctx.fire_time[i] - ctx.fire_time[i - 1];
        DTUNITTEST_ASSERT_INT(delta, >, interval_ms * 70 / 100);
        // strictly below interval_ms + ACCURACY_WORK_MS (the software-accumulation value)
        DTUNITTEST_ASSERT_INT(delta, <, interval_ms * 130 / 100);
    }

cleanup:
    if (dterr)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_accuracy_under_load failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
// test_dtmc_base_dtinterval_pause_is_idempotent
//
// Confirms that calling pause() before start(), and again a second time,
// returns no error in either case.

dterr_t*
test_dtmc_base_dtinterval_pause_is_idempotent(dtinterval_handle h)
{
    dterr_t* dterr = NULL;

    DTERR_C(dtinterval_pause(h));
    DTERR_C(dtinterval_pause(h));

cleanup:
    if (dterr)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "test_dtmc_base_dtinterval_pause_is_idempotent failed");
    return dterr;
}
