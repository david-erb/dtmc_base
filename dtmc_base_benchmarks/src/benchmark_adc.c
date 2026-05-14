#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtadc.h>
#include <dtmc_base/dtbusywork.h>
#include <dtmc_base/dtruntime.h>
#include <dtmc_base/dttasker.h>

#include <dtmc_base_benchmarks/benchmark_adc.h>
#include <dtmc_base_benchmarks/benchmark_helpers.h>

#define TAG "dtmc_base_benchmark_adc"

// --------------------------------------------------------------------------------------
// Histogram: 32 bins, 100us each, nominal bin [0,+100us) at index 16 with 15 bins to
// the left and 14 bins to the right, plus underflow (<-1500us) and overflow (>+1500us).
//
//  Bin  0 :   < -1500us         (early underflow)
//  Bin  1 : -1500 to -1400us
//  ...
//  Bin 15 :  -100 to     0us
//  Bin 16 :     0 to  +100us    (nominal — delta=0 lands here)
//  Bin 17 :  +100 to  +200us
//  ...
//  Bin 30 : +1400 to +1500us
//  Bin 31 :   > +1500us         (late overflow)

#define HISTOGRAM_BINS 32
#define NOMINAL_BIN 16
#define COLLECTION_SECONDS 10
#define BAR_MAX_WIDTH 40

static const char* const BIN_LABEL[HISTOGRAM_BINS] = {
    "    < -1500us",
    "-1500 to -1400us",
    "-1400 to -1300us",
    "-1300 to -1200us",
    "-1200 to -1100us",
    "-1100 to -1000us",
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
    "    0 to  +100us", // nominal
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
    "+1100 to +1200us",
    "+1200 to +1300us",
    "+1300 to +1400us",
    "+1400 to +1500us",
    "    > +1500us",
};

// --------------------------------------------------------------------------------------

typedef struct benchmark_t
{
    benchmark_config_t config;

    dtbusywork_handle busywork_handle;
    dttasker_handle tasker_handle;

    // written only from the scan callback, read after start() returns
    uint64_t prev_timestamp_ns; // scan->timestamp_ns from the previous scan
    uint64_t start_ms;          // dtruntime_now_milliseconds() at first scan (0 = not yet started)
    uint64_t elapsed_ms;        // total collection time, set when collection ends
    int32_t scan_count;
    int32_t bins[HISTOGRAM_BINS];

} benchmark_t;

static dterr_t*
_scan_callback(void* context, dtadc_scan_t* scan);

static void
_print_histogram(benchmark_t* self);

// --------------------------------------------------------------------------------------

static int32_t
_delta_to_bin(int32_t delta_us)
{
    if (delta_us < -1500)
        return 0; // underflow
    if (delta_us < 0)
        return 1 + (delta_us + 1500) / 100; // bins 1-15
    if (delta_us < 100)
        return 16; // nominal
    if (delta_us < 1500)
        return 17 + (delta_us - 100) / 100; // bins 17-30
    return 31;                              // overflow
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

    d = dtstr_concat_format(d, s, "Measures per-scan jitter of ADC scan delivery under CPU load.");
    d = dtstr_concat_format(d, s, "Nominal scan interval: %" PRId32 "us", self->config.nominal_scan_interval_us);
    d = dtstr_concat_format(d, s, "Collection period: %d seconds", COLLECTION_SECONDS);
    d = dtstr_concat_format(d, s, "Busywork load task runs on core %" PRId32 " during measurement.", self->config.app_core);

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
    DTERR_ASSERT_NOT_NULL(config->adc_handle);

    if (config->nominal_scan_interval_us <= 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "nominal_scan_interval_us must be > 0");
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
        c.name = "benchmark";
        c.tasker_entry_point_fn = benchmark_entrypoint;
        c.tasker_entry_point_arg = self;
        c.priority = DTTASKER_PRIORITY_NORMAL_MEDIUM;
        c.core = self->config.app_core;
        c.stack_size = 4096;

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

    DTERR_C(dtadc_activate(self->config.adc_handle, self, _scan_callback));

    dtruntime_sleep_milliseconds((dtruntime_milliseconds_t)COLLECTION_SECONDS * 1000);
    self->elapsed_ms = dtruntime_now_milliseconds() - self->start_ms;

    DTERR_C(dtadc_deactivate(self->config.adc_handle));

    DTERR_C(dtbusywork_snapshot(self->busywork_handle));
    // DTERR_C(dtbusywork_stop(self->busywork_handle));

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
// Scan callback.
//
// scan->timestamp_ns is stamped at the start of each tick (before ADC reads),
// capturing when the timer fired.  The driver reads the counter peripheral directly
// via counter_get_value(), converting to nanoseconds at the counter's native
// frequency with wrap tracking, so resolution and wrap behaviour are independent
// of CONFIG_SYS_CLOCK_TICKS_PER_SEC.  On the first scan we record the baseline
// and discard; from the second onward the delta gives the scan-to-scan interval.

static dterr_t*
_scan_callback(void* context, dtadc_scan_t* scan)
{
    benchmark_t* self = (benchmark_t*)context;

    if (self->start_ms == 0)
    {
        // first scan: record baseline timestamp, nothing to bin yet
        self->start_ms = dtruntime_now_milliseconds();
        self->prev_timestamp_ns = scan->timestamp_ns;
        return NULL;
    }

    uint64_t delta_ns = scan->timestamp_ns - self->prev_timestamp_ns;
    self->prev_timestamp_ns = scan->timestamp_ns;

    int32_t delta_us = (int32_t)(delta_ns / 1000) - self->config.nominal_scan_interval_us;
    self->bins[_delta_to_bin(delta_us)]++;
    self->scan_count++;

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
      "=== ADC Scan Jitter Histogram"
      " (%d.%01ds, %" PRId32 " scans, nominal %" PRId32 "us) ===",
      (int)(self->elapsed_ms / 1000),
      (int)((self->elapsed_ms % 1000) / 100),
      self->scan_count,
      self->config.nominal_scan_interval_us);

    dtlog_info(TAG, "%-18s  %6s  %s", "delta from nominal", "count", "distribution");

    for (int32_t i = 0; i < HISTOGRAM_BINS; i++)
    {
        int32_t count = self->bins[i];
        int32_t bar_len = (count * BAR_MAX_WIDTH + max_count / 2) / max_count;

        char bar[BAR_MAX_WIDTH + 1];
        for (int32_t j = 0; j < bar_len; j++)
            bar[j] = '#';
        bar[bar_len] = '\0';

        const char* suffix = (i == NOMINAL_BIN) ? "  <- nominal" : "";

        dtlog_info(TAG, "%-18s  %6" PRId32 "  %s%s", BIN_LABEL[i], count, bar, suffix);
    }
}
