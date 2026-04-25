#include <stdio.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtgpiopin.h>
#include <dtmc_base/dtgpiopin_dummy.h>

#include <dtmc_base_tests.h>

// -----------------------------------------------------------------------------
// Test-local helpers

typedef struct edge_counter_t
{
    int total;
    int rising;
    int falling;
    dtgpiopin_edge_t last;
} edge_counter_t;

static void
edge_cb_count(dtgpiopin_edge_t edge, void* ctx)
{
    edge_counter_t* c = (edge_counter_t*)ctx;
    if (!c)
        return;
    c->total++;
    c->last = edge;
    if (edge == DTGPIOPIN_IRQ_RISING)
        c->rising++;
    if (edge == DTGPIOPIN_IRQ_FALLING)
        c->falling++;
}

static void
edge_cb_plus10(dtgpiopin_edge_t edge, void* ctx)
{
    (void)edge;
    edge_counter_t* c = (edge_counter_t*)ctx;
    if (!c)
        return;
    c->total += 10;
}

// -----------------------------------------------------------------------------
// 1) Heap lifecycle -> configure -> handle-only facade calls

static dterr_t*
test_dtgpiopin_dummy_basic_facade_flow(void)
{
    dterr_t* dterr = NULL;
    dtgpiopin_handle h = NULL;

    // Create + configure using the concrete pointer
    dtgpiopin_dummy_t* obj = NULL;
    DTERR_C(dtgpiopin_dummy_create(&obj));

    dtgpiopin_dummy_config_t cfg = {
        .pin_number = 5,
        .mode = DTGPIOPIN_MODE_OUTPUT,
        .pull = DTGPIOPIN_PULL_NONE,
        .drive = DTGPIOPIN_DRIVE_DEFAULT,
    };
    DTERR_C(dtgpiopin_dummy_configure(obj, &cfg));

    // From here on, use only the opaque handle + facade
    h = (dtgpiopin_handle)obj;

    edge_counter_t ec = { 0 };

    // Attach callback but keep IRQs disabled initially
    DTERR_C(dtgpiopin_attach(h, edge_cb_count, &ec));

    // Write before enable -> no callbacks
    DTERR_C(dtgpiopin_write(h, true));
    DTUNITTEST_ASSERT_INT(ec.total, ==, 0);

    // Enable IRQs, then toggle -> callbacks appear
    DTERR_C(dtgpiopin_enable(h, true));
    DTERR_C(dtgpiopin_write(h, false)); // FALLING
    DTUNITTEST_ASSERT_INT(ec.total, ==, 1);
    DTUNITTEST_ASSERT_INT(ec.falling, ==, 1);

    // Read current level
    bool level = true;
    DTERR_C(dtgpiopin_read(h, &level));
    DTUNITTEST_ASSERT_TRUE(level == false);

cleanup:
    dtgpiopin_dispose(h);
    return dterr;
}

// -----------------------------------------------------------------------------
// 2) Error: write() while configured as INPUT should fail

static dterr_t*
test_dtgpiopin_dummy_write_in_input_mode_is_error(void)
{
    dterr_t* dterr = NULL;
    dtgpiopin_handle h = NULL;

    dtgpiopin_dummy_t* obj = NULL;
    DTERR_C(dtgpiopin_dummy_create(&obj));

    dtgpiopin_dummy_config_t cfg = {
        .pin_number = 2,
        .mode = DTGPIOPIN_MODE_INPUT,
        .pull = DTGPIOPIN_PULL_UP,
        .drive = DTGPIOPIN_DRIVE_DEFAULT,
    };
    DTERR_C(dtgpiopin_dummy_configure(obj, &cfg));

    h = (dtgpiopin_handle)obj;

    // Attempt write -> expect DTERR_FAIL per dummy policy
    dterr = dtgpiopin_write(h, true);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_FAIL);

cleanup:
    dtgpiopin_dispose(h);
    return NULL; // swallow error after assertion, keeps runner happy
}

// -----------------------------------------------------------------------------
// 3) IRQ enable masking: disabled -> no callbacks; enabled -> edges delivered

static dterr_t*
test_dtgpiopin_dummy_irq_masking(void)
{
    dterr_t* dterr = NULL;
    dtgpiopin_handle h = NULL;

    dtgpiopin_dummy_t* obj = NULL;
    DTERR_C(dtgpiopin_dummy_create(&obj));

    dtgpiopin_dummy_config_t cfg = {
        .pin_number = 7,
        .mode = DTGPIOPIN_MODE_OUTPUT,
        .pull = DTGPIOPIN_PULL_NONE,
        .drive = DTGPIOPIN_DRIVE_DEFAULT,
    };
    DTERR_C(dtgpiopin_dummy_configure(obj, &cfg));

    h = (dtgpiopin_handle)obj;

    edge_counter_t ec = { 0 };
    DTERR_C(dtgpiopin_attach(h, edge_cb_count, &ec));

    // Still disabled
    DTERR_C(dtgpiopin_write(h, true));
    DTERR_C(dtgpiopin_write(h, false));
    DTUNITTEST_ASSERT_INT(ec.total, ==, 0);

    // Enable and toggle
    DTERR_C(dtgpiopin_enable(h, true));
    DTERR_C(dtgpiopin_write(h, true));
    DTERR_C(dtgpiopin_write(h, false));

    DTUNITTEST_ASSERT_INT(ec.total, ==, 2);
    DTUNITTEST_ASSERT_INT(ec.rising, ==, 1);
    DTUNITTEST_ASSERT_INT(ec.falling, ==, 1);

cleanup:
    dtgpiopin_dispose(h);
    return dterr;
}

// -----------------------------------------------------------------------------
// 4) Callback replacement should stop calling the previous one

static dterr_t*
test_dtgpiopin_dummy_callback_replacement(void)
{
    dterr_t* dterr = NULL;
    dtgpiopin_handle h = NULL;

    dtgpiopin_dummy_t* obj = NULL;
    DTERR_C(dtgpiopin_dummy_create(&obj));

    dtgpiopin_dummy_config_t cfg = {
        .pin_number = 11,
        .mode = DTGPIOPIN_MODE_OUTPUT,
        .pull = DTGPIOPIN_PULL_NONE,
        .drive = DTGPIOPIN_DRIVE_DEFAULT,
    };
    DTERR_C(dtgpiopin_dummy_configure(obj, &cfg));

    h = (dtgpiopin_handle)obj;

    edge_counter_t c1 = { 0 };
    edge_counter_t c2 = { 0 };

    DTERR_C(dtgpiopin_attach(h, edge_cb_count, &c1));
    DTERR_C(dtgpiopin_enable(h, true));
    DTERR_C(dtgpiopin_write(h, true)); // fires c1
    DTUNITTEST_ASSERT_INT(c1.total, ==, 1);

    // Replace with different callback
    DTERR_C(dtgpiopin_attach(h, edge_cb_plus10, &c2));
    DTERR_C(dtgpiopin_write(h, false)); // should fire only c2
    DTUNITTEST_ASSERT_INT(c1.total, ==, 1);
    DTUNITTEST_ASSERT_INT(c2.total, ==, 10);

cleanup:
    dtgpiopin_dispose(h);
    return dterr;
}

// -----------------------------------------------------------------------------
// 5) Behavior knobs: prove we can steer delivery without breaking opacity
//    - start level high
//    - suppress auto edge on write
//    - emit edge when enabling (level-derived)

static dterr_t*
test_dtgpiopin_dummy_behavior_knobs(void)
{
    dterr_t* dterr = NULL;
    dtgpiopin_handle h = NULL;

    dtgpiopin_dummy_t* obj = NULL;
    DTERR_C(dtgpiopin_dummy_create(&obj));

    // Set desired behavior BEFORE configure/enable
    dtgpiopin_dummy_behavior_t bh = { 0 };
    // (fields defined below in "Add these to behavior")
    bh.start_level_high = true;
    bh.auto_edge_on_write = false;
    bh.emit_edge_on_enable = true;
    DTERR_C(dtgpiopin_dummy_set_behavior(obj, &bh));

    dtgpiopin_dummy_config_t cfg = {
        .pin_number = 9,
        .mode = DTGPIOPIN_MODE_OUTPUT,
        .pull = DTGPIOPIN_PULL_NONE,
        .drive = DTGPIOPIN_DRIVE_DEFAULT,
    };
    DTERR_C(dtgpiopin_dummy_configure(obj, &cfg));

    h = (dtgpiopin_handle)obj;

    // With start_level_high, read() should see 'true' before any writes
    bool level = false;
    DTERR_C(dtgpiopin_read(h, &level));
    DTUNITTEST_ASSERT_TRUE(level == true);

    edge_counter_t ec = { 0 };
    DTERR_C(dtgpiopin_attach(h, edge_cb_count, &ec));

    // Enable should immediately emit an edge based on current level (RISING here)
    DTERR_C(dtgpiopin_enable(h, true));
    DTUNITTEST_ASSERT_INT(ec.total, ==, 1);
    DTUNITTEST_ASSERT_INT(ec.rising, ==, 1);

    // With auto_edge_on_write=false, toggling should NOT emit callbacks
    DTERR_C(dtgpiopin_write(h, false));
    DTERR_C(dtgpiopin_write(h, true));
    DTUNITTEST_ASSERT_INT(ec.total, ==, 1); // unchanged

cleanup:
    dtgpiopin_dispose(h);
    return dterr;
}

// -----------------------------------------------------------------------------
// Suite entry point

void
test_dtmc_base_dtgpiopin_dummy(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtgpiopin_dummy_basic_facade_flow);
    DTUNITTEST_RUN_TEST(test_dtgpiopin_dummy_write_in_input_mode_is_error);
    DTUNITTEST_RUN_TEST(test_dtgpiopin_dummy_irq_masking);
    DTUNITTEST_RUN_TEST(test_dtgpiopin_dummy_callback_replacement);
    DTUNITTEST_RUN_TEST(test_dtgpiopin_dummy_behavior_knobs);
}
