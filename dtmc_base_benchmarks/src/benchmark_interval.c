#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtbusywork.h>
#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtinterval.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dttasker.h>

#include <dtmc_base_benchmarks/benchmark_helpers.h>
#include <dtmc_base_benchmarks/benchmark_interval.h>

#define TAG "dtmc_base_benchmark_interval"

// --------------------------------------------------------------------------------------
// Histogram: 23 bins, 100us each, nominal bin [0,+100us) at index 11 with 10 bins on
// each side, plus underflow (<-1000us) and overflow (>+1100us) at the ends.
//
//  Bin  0 :   < -1000us         (early overflow)
//  Bin  1 : -1000 to  -900us
//  ...
//  Bin 10 :  -100 to     0us
//  Bin 11 :     0 to  +100us    (nominal — delta=0 lands here)
//  Bin 12 :  +100 to  +200us
//  ...
//  Bin 21 : +1000 to +1100us
//  Bin 22 :   > +1100us         (late overflow)

#define HISTOGRAM_BINS     23
#define NOMINAL_BIN        11
#define COLLECTION_SECONDS 10
#define BAR_MAX_WIDTH      40

static const char* const BIN_LABEL[HISTOGRAM_BINS] = {
    "    < -1000us",
    "-1000 to  -900us",
    " -900 to  -800us",
    " -800 to  -700us",
    " -700 to  -600us",
    " -600 to  -500us",
    " -500 to  -400us",
    " -400 to  -300us",
    " -300 to  -200us",
    " -200 to  -100us",
    " -100 to     0us",
    "    0 to  +100us",   // nominal
    " +100 to  +200us",
    " +200 to  +300us",
    " +300 to  +400us",
    " +400 to  +500us",
    " +500 to  +600us",
    " +600 to  +700us",
    " +700 to  +800us",
    " +800 to  +900us",
    " +900 to +1000us",
    "+1000 to +1100us",
    "    > +1100us",
};

// --------------------------------------------------------------------------------------

typedef struct benchmark_t
{
    benchmark_config_t config;

    dtbusywork_handle busywork_handle;
    dttasker_handle   tasker_handle;

    // written only from the tick callback, read after start() returns
    dtcpu_t  tick_mark;              // call mark() each tick; elapsed() gives tick-to-tick interval
    uint64_t start_ms;               // dtruntime_now_milliseconds() at first tick (0 = not yet started)
    uint64_t elapsed_ms;             // total collection time, set when collection ends
    int32_t  tick_count;
    int32_t  bins[HISTOGRAM_BINS];

} benchmark_t;

static dterr_t*
_tick_callback(void* context, int* should_pause);

static void
_print_histogram(benchmark_t* self);

// --------------------------------------------------------------------------------------

static int32_t
_delta_to_bin(int32_t delta_us)
{
    if (delta_us <  -1000) return 0;                             // underflow
    if (delta_us <      0) return 1 + (delta_us + 1000) / 100;  // bins 1-10
    if (delta_us <    100) return 11;                            // nominal
    if (delta_us <   1100) return 12 + (delta_us - 100) / 100;  // bins 12-21
    return 22;                                                   // overflow
}

// --------------------------------------------------------------------------------------

dterr_t*
benchmark_describe(benchmark_t* self, char** out_description)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_description);

    char* d = *out_description;
    const char* s = "\n#    ";

    d = dtstr_concat_format(d, s, "Measures per-tick jitter of a periodic interval timer under CPU load.");
    d = dtstr_concat_format(d, s, "Nominal interval: %" PRId32 "us", self->config.nominal_interval_us);
    d = dtstr_concat_format(d, s, "Collection period: %d seconds", COLLECTION_SECONDS);
    d = dtstr_concat_format(d,
      s,
      "Busywork load task runs on core %" PRId32 " during measurement.",
      self->config.app_core);

    *out_description = d;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------

dterr_t*
benchmark_create(benchmark_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    DTERR_C(dtheaper_alloc_and_zero(sizeof(benchmark_t), "benchmark_t", (void**)self));
    DTERR_C(dtcpu_sysinit());

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------

dterr_t*
benchmark_configure(benchmark_t* self, benchmark_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);
    DTERR_ASSERT_NOT_NULL(config->interval_handle);

    if (config->nominal_interval_us <= 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "nominal_interval_us must be > 0");
        goto cleanup;
    }

    self->config = *config;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------

dterr_t*
benchmark_start(benchmark_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    {
        dttasker_config_t c = { 0 };
        c.name                   = "benchmark";
        c.tasker_entry_point_fn  = benchmark_entrypoint;
        c.tasker_entry_point_arg = self;
        c.priority               = DTTASKER_PRIORITY_NORMAL_MEDIUM;
        c.core                   = self->config.app_core;
        c.stack_size             = 4096;

        DTERR_C(dttasker_create(&self->tasker_handle, &c));
    }

    DTERR_C(dttasker_start(self->tasker_handle));

    while (true)
    {
        dtruntime_sleep_milliseconds(1000);
        dttasker_info_t info = { 0 };
        DTERR_C(dttasker_get_info(self->tasker_handle, &info));
        if (info.dterr != NULL)
        {
            dterr = dterr_new(DTERR_FAIL, DTERR_LOC, info.dterr, "benchmark task reported an error");
            goto cleanup;
        }
        if (info.status == STOPPED)
            break;
    }

cleanup:
    dttasker_dispose(self->tasker_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------

dterr_t*
benchmark_entrypoint(void* opaque_self, dttasker_handle tasker_handle)
{
    dterr_t* dterr = NULL;
    benchmark_t* self = (benchmark_t*)opaque_self;
    DTERR_ASSERT_NOT_NULL(self);

    {
        char* description = NULL;
        DTERR_C(dtmc_base_benchmark_helpers_decorate_description(
          (void*)self, (benchmark_describe_fn)benchmark_describe, &description));
        dtlog_info(TAG, "%s", description);
        dtstr_dispose(description);
    }

    {
        dtbusywork_config_t c = { 0 };
        DTERR_C(dtbusywork_create(&self->busywork_handle, &c));
        DTERR_C(dtbusywork_add_worker(self->busywork_handle, self->config.app_core, DTTASKER_PRIORITY_BACKGROUND_MEDIUM));
        DTERR_C(dtbusywork_start(self->busywork_handle));
    }

    {
        char* s = NULL;
        DTERR_C(dtruntime_register_tasks(&dttasker_registry_global_instance));
        DTERR_C(dttasker_registry_format_as_table(&dttasker_registry_global_instance, &s));
        dtlog_info(TAG, "Tasks:\n%s", s);
        dtstr_dispose(s);
    }

    DTERR_C(dttasker_ready(tasker_handle));

    dtlog_info(TAG, "collecting for %d seconds...", COLLECTION_SECONDS);

    DTERR_C(dtinterval_set_callback(self->config.interval_handle, _tick_callback, self));
    DTERR_C(dtinterval_start(self->config.interval_handle)); // blocks until collection is done

    DTERR_C(dtbusywork_snapshot(self->busywork_handle));
    DTERR_C(dtbusywork_stop(self->busywork_handle));

    _print_histogram(self);

    {
        char* str = NULL;
        DTERR_C(dtbusywork_summarize(self->busywork_handle, &str));
        dtlog_info(TAG, "\n%s", str);
        dtstr_dispose(str);
    }

cleanup:
    dtbusywork_dispose(self->busywork_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------

void
benchmark_dispose(benchmark_t* self)
{
    if (!self)
        return;
    dtheaper_free(self);
}

// --------------------------------------------------------------------------------------
// Tick callback.
//
// dtcpu_mark() is called first on every tick: it shifts the previous snapshot
// to ->old and captures a fresh cycle count into ->new.  The first call has
// old==0 (zero-initialised struct), so that elapsed reading is discarded.
// From the second tick onward, elapsed_microseconds() returns the exact
// tick-to-tick interval.

static dterr_t*
_tick_callback(void* context, int* should_pause)
{
    benchmark_t* self = (benchmark_t*)context;

    dtcpu_mark(&self->tick_mark);
    *should_pause = 0;

    if (self->start_ms == 0)
    {
        // first tick: baseline established; old==0 so elapsed is meaningless
        self->start_ms = dtruntime_now_milliseconds();
        return NULL;
    }

    dtcpu_microseconds_t actual_us = dtcpu_elapsed_microseconds(&self->tick_mark);
    int32_t delta_us = (int32_t)actual_us - self->config.nominal_interval_us;
    self->bins[_delta_to_bin(delta_us)]++;
    self->tick_count++;

    uint64_t now_ms = dtruntime_now_milliseconds();
    if (now_ms - self->start_ms >= (uint64_t)COLLECTION_SECONDS * 1000ULL)
    {
        self->elapsed_ms  = now_ms - self->start_ms;
        *should_pause = 1;
    }

    return NULL;
}

// --------------------------------------------------------------------------------------

static void
_print_histogram(benchmark_t* self)
{
    int32_t max_count = 1;
    for (int32_t i = 0; i < HISTOGRAM_BINS; i++)
    {
        if (self->bins[i] > max_count)
            max_count = self->bins[i];
    }

    dtlog_info(TAG,
      "=== Interval Jitter Histogram"
      " (%d.%01ds, %" PRId32 " ticks, nominal %" PRId32 "us) ===",
      (int)(self->elapsed_ms / 1000),
      (int)((self->elapsed_ms % 1000) / 100),
      self->tick_count,
      self->config.nominal_interval_us);

    dtlog_info(TAG, "%-18s  %6s  %s", "delta from nominal", "count", "distribution");

    for (int32_t i = 0; i < HISTOGRAM_BINS; i++)
    {
        int32_t count   = self->bins[i];
        int32_t bar_len = (count * BAR_MAX_WIDTH + max_count / 2) / max_count;

        char bar[BAR_MAX_WIDTH + 1];
        for (int32_t j = 0; j < bar_len; j++)
            bar[j] = '#';
        bar[bar_len] = '\0';

        const char* suffix = (i == NOMINAL_BIN) ? "  <- nominal" : "";

        dtlog_info(TAG, "%-18s  %6" PRId32 "  %s%s", BIN_LABEL[i], count, bar, suffix);
    }
}
