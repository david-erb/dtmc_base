#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtkvp.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dttimeseries.h>
#include <dtmc_base/dttimeseries_beat.h>
#include <dtmc_base/dttimeseries_browngrav.h>
#include <dtmc_base/dttimeseries_damped_sine.h>
#include <dtmc_base/dttimeseries_sine.h>
#include <dtmc_base/dttimeseries_steady.h>

#include <dtmc_base_tests.h>

#define TAG "test_dtmc_base_dttimeseries"

// ------------------------------------------------------------------------
// Example: Small, readable "happy path" showing basic formatting & cleanup.
static dterr_t*
test_dtmc_base_dttimeseries_example_basic(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_steady_t* o = NULL;
        DTERR_C(dttimeseries_steady_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 1, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 0.0, 0.0);

    DTERR_C(dtkvp_list_init(kvp_list));

    DTERR_C(dtkvp_list_set(kvp_list, "value", "3.14"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));
    DTERR_C(dttimeseries_read(timeseries_handle, 1, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 3.14, 0.0);

    DTERR_C(dtkvp_list_set(kvp_list, "value", "2.718"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));
    DTERR_C(dttimeseries_read(timeseries_handle, 1, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 2.718, 0.0);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Example: Sine wave reads at quarter-period intervals to verify amplitude and zero crossings.
static dterr_t*
test_dtmc_base_dttimeseries_sine_example_basic(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_sine_t* o = NULL;
        DTERR_C(dttimeseries_sine_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    // Default state: zero amplitude, reads as 0.
    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 0.0, 0.0);

    DTERR_C(dtkvp_list_init(kvp_list));

    // 1 Hz, amplitude 2.0: period = 1 000 000 us.
    DTERR_C(dtkvp_list_set(kvp_list, "frequency", "1.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "amplitude", "2.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    // t=0: sin(0) = 0
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 0.0, 0.0);

    // t=250 000 us (quarter period): sin(π/2) = 1 → value = 2.0
    DTERR_C(dttimeseries_read(timeseries_handle, 250000, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 2.0, 0.0);

    // t=500 000 us (half period): sin(π) = 0
    DTERR_C(dttimeseries_read(timeseries_handle, 500000, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, <, 0.0, 1e-9);

    // t=750 000 us (three-quarter period): sin(3π/2) = -1 → value = -2.0
    DTERR_C(dttimeseries_read(timeseries_handle, 750000, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, -2.0, 0.0);

    // t=1 000 000 us (full period): sin(2π) = 0
    DTERR_C(dttimeseries_read(timeseries_handle, 1000000, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, <, 0.0, 1e-9);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Example: Damped sine reads verify zero crossings, peak amplitude, and decay.
// Configuration: frequency=1.0 Hz, amplitude=2.0, decay=2.0 (1/s).
// At decay=2 the envelope at the first quarter-period (t=0.25 s) is 2*e^(-0.5) ≈ 1.2131,
// and at the second quarter-period (t=0.75 s) it is 2*e^(-1.5) ≈ 0.4463, showing clear
// attenuation each cycle.
static dterr_t*
test_dtmc_base_dttimeseries_damped_sine_example_basic(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_damped_sine_t* o = NULL;
        DTERR_C(dttimeseries_damped_sine_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    // Default state: zero amplitude, reads as 0.
    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 0.0, 0.0);

    DTERR_C(dtkvp_list_init(kvp_list));

    // 1 Hz, amplitude 2.0, decay 2.0 (1/s).
    DTERR_C(dtkvp_list_set(kvp_list, "frequency", "1.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "amplitude", "2.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "decay", "2.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    // t=0: envelope * sin(0) = 0
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 0.0, 0.0);

    // t=250 000 us (1st quarter period): 2*e^(-0.5)*sin(π/2) = 2*e^(-0.5) ≈ 1.2131
    double value_first_peak;
    DTERR_C(dttimeseries_read(timeseries_handle, 250000, &value_first_peak));
    DTUNITTEST_ASSERT_DOUBLE(value_first_peak, <, 1.2131, 1e-4);

    // t=750 000 us (2nd quarter period): 2*e^(-1.5)*sin(3π/2) = -2*e^(-1.5) ≈ -0.4463
    double value_second_peak;
    DTERR_C(dttimeseries_read(timeseries_handle, 750000, &value_second_peak));
    DTUNITTEST_ASSERT_DOUBLE(value_second_peak, <, -0.4463, 1e-4);

    // The first peak magnitude exceeds the second: decay is working.
    // Expected difference ≈ 0.767; these bounds rule out any plausible arithmetic error.
    DTUNITTEST_ASSERT_DOUBLE(value_first_peak, >, -value_second_peak, 0.7);
    DTUNITTEST_ASSERT_DOUBLE(value_first_peak, <, -value_second_peak, 0.9);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Example: With zero noise the walk is pinned to attraction_point; with noise the
// output varies but stays bounded near it over many calls.
static dterr_t*
test_dtmc_base_dttimeseries_browngrav_example_basic(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_browngrav_t* o = NULL;
        DTERR_C(dttimeseries_browngrav_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    DTERR_C(dtkvp_list_init(kvp_list));

    // Zero noise: every read returns attraction_point exactly.
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_point", "50"));
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_strength", "10"));
    DTERR_C(dtkvp_list_set(kvp_list, "noise_intensity", "0"));
    DTERR_C(dtkvp_list_set(kvp_list, "seed", "0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 50.0, 0.0);
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 50.0, 0.0);

    // With noise: values vary but remain within a reasonable band after 100 steps.
    DTERR_C(dtkvp_list_set(kvp_list, "noise_intensity", "5"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    double sum = 0.0;
    for (int i = 0; i < 100; i++)
    {
        DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
        sum += value;
    }
    double mean = sum / 100.0;
    DTUNITTEST_ASSERT_DOUBLE(mean, <, 50.0, 20.0);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// decay=0 boundary: e^(-0*t) = 1, so damped_sine must equal plain sine exactly.
static dterr_t*
test_dtmc_base_dttimeseries_damped_sine_zero_decay(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle sine_handle = NULL;
    dttimeseries_handle damped_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_sine_t* o = NULL;
        DTERR_C(dttimeseries_sine_create(&o));
        sine_handle = (dttimeseries_handle)o;
    }
    {
        dttimeseries_damped_sine_t* o = NULL;
        DTERR_C(dttimeseries_damped_sine_create(&o));
        damped_handle = (dttimeseries_handle)o;
    }

    DTERR_C(dtkvp_list_init(kvp_list));
    DTERR_C(dtkvp_list_set(kvp_list, "frequency", "1.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "amplitude", "2.0"));
    DTERR_C(dttimeseries_configure(sine_handle, kvp_list));

    DTERR_C(dtkvp_list_set(kvp_list, "decay", "0.0"));
    DTERR_C(dttimeseries_configure(damped_handle, kvp_list));

    dtcpu_microseconds_t timestamps[] = { 0, 250000, 500000, 750000, 1000000 };
    for (int i = 0; i < 5; i++)
    {
        double sine_value, damped_value;
        DTERR_C(dttimeseries_read(sine_handle, timestamps[i], &sine_value));
        DTERR_C(dttimeseries_read(damped_handle, timestamps[i], &damped_value));
        DTUNITTEST_ASSERT_DOUBLE(damped_value, ==, sine_value, 0.0);
    }

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(sine_handle);
    dttimeseries_dispose(damped_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Reconfiguring with the same seed resets the walk: the sequence must be identical.
static dterr_t*
test_dtmc_base_dttimeseries_browngrav_determinism(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_browngrav_t* o = NULL;
        DTERR_C(dttimeseries_browngrav_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    DTERR_C(dtkvp_list_init(kvp_list));
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_point", "100"));
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_strength", "5"));
    DTERR_C(dtkvp_list_set(kvp_list, "noise_intensity", "10"));
    DTERR_C(dtkvp_list_set(kvp_list, "seed", "42"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    double first_run[10];
    for (int i = 0; i < 10; i++)
        DTERR_C(dttimeseries_read(timeseries_handle, 0, &first_run[i]));

    // Reconfigure with identical params: walk must reset to produce the same sequence.
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    for (int i = 0; i < 10; i++)
    {
        double value;
        DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
        DTUNITTEST_ASSERT_DOUBLE(value, ==, first_run[i], 0.0);
    }

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// With no "scale" key in the config, scale must default to 1.0 (not the 0.0
// that comes from zero-initializing the allocation).
static dterr_t*
test_dtmc_base_dttimeseries_browngrav_scale_default(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_browngrav_t* o = NULL;
        DTERR_C(dttimeseries_browngrav_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    DTERR_C(dtkvp_list_init(kvp_list));

    // Configure without a "scale" key. Zero noise pins the output to attraction_point.
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_point", "75"));
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_strength", "10"));
    DTERR_C(dtkvp_list_set(kvp_list, "noise_intensity", "0"));
    DTERR_C(dtkvp_list_set(kvp_list, "seed", "0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    // scale=1.0 default: value = 0.0 + 1.0 * 75 = 75.0
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 75.0, 0.0);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// offset shifts the raw value additively; scale multiplies it; together they
// produce offset + scale * raw.  Zero noise keeps the raw output at
// attraction_point so the arithmetic is exact.
static dterr_t*
test_dtmc_base_dttimeseries_browngrav_offset_and_scale(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_browngrav_t* o = NULL;
        DTERR_C(dttimeseries_browngrav_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    DTERR_C(dtkvp_list_init(kvp_list));
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_point", "50"));
    DTERR_C(dtkvp_list_set(kvp_list, "attraction_strength", "10"));
    DTERR_C(dtkvp_list_set(kvp_list, "noise_intensity", "0"));
    DTERR_C(dtkvp_list_set(kvp_list, "seed", "0"));

    // offset only: 10.0 + 1.0 * 50 = 60.0
    DTERR_C(dtkvp_list_set(kvp_list, "offset", "10.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 60.0, 0.0);

    // scale only (clear offset): 0.0 + 2.0 * 50 = 100.0
    DTERR_C(dtkvp_list_set(kvp_list, "offset", "0.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "scale", "2.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 100.0, 0.0);

    // both together: 10.0 + 2.0 * 50 = 110.0
    DTERR_C(dtkvp_list_set(kvp_list, "offset", "10.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 110.0, 0.0);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Example: at t=0 both sines are 0 regardless of frequency; at t=quarter-period
// with f1=f2 the two sines are always in phase and sum to 2*amplitude.
static dterr_t*
test_dtmc_base_dttimeseries_beat_example_basic(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_beat_t* o = NULL;
        DTERR_C(dttimeseries_beat_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    DTERR_C(dtkvp_list_init(kvp_list));

    // At t=0 both sin terms are 0, so value == offset regardless of f1/f2.
    DTERR_C(dtkvp_list_set(kvp_list, "f1", "3.7"));
    DTERR_C(dtkvp_list_set(kvp_list, "f2", "5.1"));
    DTERR_C(dtkvp_list_set(kvp_list, "amplitude", "1.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "offset", "0.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 0.0, 0.0);

    // Non-zero offset shifts the baseline.
    DTERR_C(dtkvp_list_set(kvp_list, "offset", "5.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));
    DTERR_C(dttimeseries_read(timeseries_handle, 0, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 5.0, 0.0);

    // f1=f2=1.0: both sines are always in phase; at the quarter-period
    // (t=250000 us) each equals 1, so value = 0 + 1.0*(1+1) = 2.0.
    DTERR_C(dtkvp_list_set(kvp_list, "f1", "1.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "f2", "1.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "offset", "0.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));
    DTERR_C(dttimeseries_read(timeseries_handle, 250000, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 2.0, 0.0);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
// Destructive interference: f1=1.5 Hz, f2=0.5 Hz, t=0.5 s.
// sin(2π*1.5*0.5) + sin(2π*0.5*0.5) = sin(1.5π) + sin(0.5π) = -1 + 1 = 0.
// The two sines cancel exactly, so value == offset.
static dterr_t*
test_dtmc_base_dttimeseries_beat_destructive(void)
{
    dterr_t* dterr = NULL;
    dttimeseries_handle timeseries_handle = NULL;
    dtkvp_list_t _kvp_list = { 0 }, *kvp_list = &_kvp_list;

    {
        dttimeseries_beat_t* o = NULL;
        DTERR_C(dttimeseries_beat_create(&o));
        timeseries_handle = (dttimeseries_handle)o;
    }

    DTERR_C(dtkvp_list_init(kvp_list));
    DTERR_C(dtkvp_list_set(kvp_list, "f1", "1.5"));
    DTERR_C(dtkvp_list_set(kvp_list, "f2", "0.5"));
    DTERR_C(dtkvp_list_set(kvp_list, "amplitude", "2.0"));
    DTERR_C(dtkvp_list_set(kvp_list, "offset", "7.0"));
    DTERR_C(dttimeseries_configure(timeseries_handle, kvp_list));

    double value;
    DTERR_C(dttimeseries_read(timeseries_handle, 500000, &value));
    DTUNITTEST_ASSERT_DOUBLE(value, ==, 7.0, 0.0);

cleanup:
    dtkvp_list_dispose(kvp_list);
    dttimeseries_dispose(timeseries_handle);
    return dterr;
}

// ------------------------------------------------------------------------
void
test_dtmc_base_dttimeseries(DTUNITTEST_SUITE_ARGS)
{
    // Examples first (teachable, self-contained).
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_example_basic);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_sine_example_basic);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_damped_sine_example_basic);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_browngrav_example_basic);

    // Edge cases.
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_damped_sine_zero_decay);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_browngrav_determinism);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_browngrav_scale_default);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_browngrav_offset_and_scale);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_beat_example_basic);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dttimeseries_beat_destructive);
}