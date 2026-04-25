#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtdebouncer.h>
#include <dtmc_base_tests.h>

static dtdebouncer_milliseconds_t g_now_ms = 0;

// Simple test time source that returns a controllable millisecond counter.
static dtdebouncer_milliseconds_t
test_time_now_ms(void)
{
    return g_now_ms;
}
// ------------------------------------------------------------------------

// Captures callback invocations for verification in tests.
typedef struct callback_capture_t
{
    bool last_state;
    uint32_t call_count;
} callback_capture_t;

static void
test_capture_callback(bool new_state, void* context)
{
    callback_capture_t* capture = (callback_capture_t*)context;
    if (capture == NULL)
    {
        return;
    }

    capture->last_state = new_state;
    capture->call_count++;
}
// ------------------------------------------------------------------------

// Example: basic button press with bouncing before a clean rising edge.
static dterr_t*
test_dtmc_base_dtdebouncer_example_simple_rising_edge(void)
{
    dterr_t* dterr = NULL;

    dtdebouncer_t debouncer = { 0 };
    callback_capture_t capture = { 0 };

    g_now_ms = 100;
    dterr = dtdebouncer_init(&debouncer,
      false, // initial_state (button released)
      30,    // debounce_ms
      test_capture_callback,
      &capture,
      test_time_now_ms);
    DTUNITTEST_ASSERT_NULL(dterr);

    // Initial stable low; confirming no edges yet.
    g_now_ms = 105;
    dtdebouncer_update(&debouncer, false);
    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == false);
    DTUNITTEST_ASSERT_UINT32(debouncer.raw_edge_count, ==, 0);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 0);

    // Noisy rising edge: bounce around the threshold.
    g_now_ms = 110;
    dtdebouncer_update(&debouncer, true); // first high candidate
    g_now_ms = 115;
    dtdebouncer_update(&debouncer, false); // bounce low
    g_now_ms = 120;
    dtdebouncer_update(&debouncer, true); // bounce high again

    DTUNITTEST_ASSERT_UINT32(debouncer.raw_edge_count, ==, 3);
    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == false);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 0);
    DTUNITTEST_ASSERT_UINT32(capture.call_count, ==, 0);

    // Hold high long enough to satisfy debounce_ms.
    g_now_ms = 155; // 35 ms after last high candidate at 120 ms
    dtdebouncer_update(&debouncer, true);

    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == true);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 1);
    DTUNITTEST_ASSERT_UINT32(debouncer.rise_count, ==, 1);
    DTUNITTEST_ASSERT_UINT32(debouncer.fall_count, ==, 0);
    DTUNITTEST_ASSERT_UINT32(capture.call_count, ==, 1);
    DTUNITTEST_ASSERT_TRUE(capture.last_state == true);

cleanup:
    return dterr;
}
// ------------------------------------------------------------------------

// Example: noisy signal that never stays stable long enough to change state.
static dterr_t*
test_dtmc_base_dtdebouncer_example_noise_only(void)
{
    dterr_t* dterr = NULL;

    dtdebouncer_t debouncer = { 0 };
    callback_capture_t capture = { 0 };

    g_now_ms = 0;
    dterr = dtdebouncer_init(&debouncer,
      false, // initial_state low
      20,    // debounce_ms
      test_capture_callback,
      &capture,
      test_time_now_ms);
    DTUNITTEST_ASSERT_NULL(dterr);

    // Inject short pulses that are always shorter than debounce_ms.
    g_now_ms = 5;
    dtdebouncer_update(&debouncer, true); // pulse high
    g_now_ms = 10;
    dtdebouncer_update(&debouncer, false); // back low
    g_now_ms = 15;
    dtdebouncer_update(&debouncer, true); // another short high
    g_now_ms = 20;
    dtdebouncer_update(&debouncer, false); // back low again

    // Even though we saw raw edges, debounced state stays low.
    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == false);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 0);
    DTUNITTEST_ASSERT_UINT32(capture.call_count, ==, 0);
    DTUNITTEST_ASSERT_UINT32(debouncer.raw_edge_count, ==, 4);

cleanup:
    return dterr;
}
// ------------------------------------------------------------------------

// Null self pointer should produce an assertion error.
static dterr_t*
test_dtmc_base_dtdebouncer_01_init_null_self(void)
{
    dterr_t* dterr = NULL;

    dtdebouncer_t* self = NULL;
    g_now_ms = 0;

    dterr = dtdebouncer_init(self, false, 10, NULL, NULL, test_time_now_ms);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_ARGUMENT_NULL);

cleanup:
    return dterr;
}
// ------------------------------------------------------------------------

// Null time function should produce an assertion error.
static dterr_t*
test_dtmc_base_dtdebouncer_02_init_null_time_fn(void)
{
    dterr_t* dterr = NULL;

    dtdebouncer_t debouncer = { 0 };

    dterr = dtdebouncer_init(&debouncer, false, 10, NULL, NULL, NULL);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_ARGUMENT_NULL);

cleanup:
    return dterr;
}
// ------------------------------------------------------------------------

// Verify one debounced rising edge followed by one debounced falling edge.
static dterr_t*
test_dtmc_base_dtdebouncer_03_rise_and_fall_transitions(void)
{
    dterr_t* dterr = NULL;

    dtdebouncer_t debouncer = { 0 };
    callback_capture_t capture = { 0 };

    g_now_ms = 0;
    dterr = dtdebouncer_init(&debouncer,
      false, // start low
      20,    // debounce_ms
      test_capture_callback,
      &capture,
      test_time_now_ms);
    DTUNITTEST_ASSERT_NULL(dterr);

    // Rising edge: raw goes high and stays there long enough.
    g_now_ms = 5;
    dtdebouncer_update(&debouncer, true); // candidate high
    g_now_ms = 30;                        // 25 ms after candidate
    dtdebouncer_update(&debouncer, true); // should commit rising edge

    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == true);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 1);
    DTUNITTEST_ASSERT_UINT32(debouncer.rise_count, ==, 1);
    DTUNITTEST_ASSERT_UINT32(debouncer.fall_count, ==, 0);
    DTUNITTEST_ASSERT_UINT32(capture.call_count, ==, 1);
    DTUNITTEST_ASSERT_TRUE(capture.last_state == true);

    // Falling edge: raw goes low and stays low long enough.
    g_now_ms = 35;
    dtdebouncer_update(&debouncer, false); // candidate low
    g_now_ms = 60;                         // 25 ms after candidate
    dtdebouncer_update(&debouncer, false); // should commit falling edge

    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == false);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 2);
    DTUNITTEST_ASSERT_UINT32(debouncer.rise_count, ==, 1);
    DTUNITTEST_ASSERT_UINT32(debouncer.fall_count, ==, 1);
    DTUNITTEST_ASSERT_UINT32(capture.call_count, ==, 2);
    DTUNITTEST_ASSERT_TRUE(capture.last_state == false);

cleanup:
    return dterr;
}
// ------------------------------------------------------------------------

// Zero debounce_ms should allow quick acceptance after a single stable sample.
static dterr_t*
test_dtmc_base_dtdebouncer_04_zero_debounce_accepts_quickly(void)
{
    dterr_t* dterr = NULL;

    dtdebouncer_t debouncer = { 0 };
    callback_capture_t capture = { 0 };

    g_now_ms = 0;
    dterr = dtdebouncer_init(&debouncer,
      false,
      0, // debounce_ms == 0
      test_capture_callback,
      &capture,
      test_time_now_ms);
    DTUNITTEST_ASSERT_NULL(dterr);

    // First high sample: becomes candidate but not yet committed.
    g_now_ms = 10;
    dtdebouncer_update(&debouncer, true);
    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == false);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 0);
    DTUNITTEST_ASSERT_UINT32(capture.call_count, ==, 0);

    // Second high sample with same raw state: should commit immediately
    // because debounce_ms == 0.
    g_now_ms = 11;
    dtdebouncer_update(&debouncer, true);

    DTUNITTEST_ASSERT_TRUE(debouncer.stable_state == true);
    DTUNITTEST_ASSERT_UINT32(debouncer.debounced_edge_count, ==, 1);
    DTUNITTEST_ASSERT_UINT32(capture.call_count, ==, 1);
    DTUNITTEST_ASSERT_TRUE(capture.last_state == true);

cleanup:
    return dterr;
}
// ------------------------------------------------------------------------

// Suite entry point for dtdebouncer tests.
void
test_dtmc_base_dtdebouncer(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtdebouncer_example_simple_rising_edge);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtdebouncer_example_noise_only);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtdebouncer_01_init_null_self);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtdebouncer_02_init_null_time_fn);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtdebouncer_03_rise_and_fall_transitions);
    DTUNITTEST_RUN_TEST(test_dtmc_base_dtdebouncer_04_zero_debounce_accepts_quickly);
}
