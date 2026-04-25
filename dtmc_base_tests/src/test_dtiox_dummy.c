#include <stdio.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtiox_dummy.h>
#include <dtmc_base/dtuart_helpers.h>

#include <dtmc_base_tests.h>

// -----------------------------------------------------------------------------
// 1) Basic heap lifecycle + configure + handle-only facade + loopback read/write
//    Example pattern: create concrete, configure, then use opaque handle only.

static dterr_t*
test_dtiox_dummy_basic_facade_flow(void)
{
    dterr_t* dterr = NULL;
    dtiox_handle h = NULL;

    // Create + configure using the concrete pointer
    dtiox_dummy_t* obj = NULL;
    DTERR_C(dtiox_dummy_create(&obj));

    dtiox_dummy_config_t cfg = { //
        .name = "dummy1",
        .pin_tx = 0,
        .pin_rx = 1,
        .uart_config = dtuart_helper_default_config
    };
    DTERR_C(dtiox_dummy_configure(obj, &cfg));

    // From here on, use only the opaque handle + facade
    h = (dtiox_handle)obj;

    // Optional: enable RX; dummy backend records this but does not depend on it
    DTERR_C(dtiox_enable(h, true));

    // Write a short message and confirm it emerges via non-blocking read.
    const uint8_t msg[] = { 'h', 'e', 'l', 'l', 'o' };
    int32_t written = 0;
    DTERR_C(dtiox_write(h, msg, sizeof(msg), &written));
    DTUNITTEST_ASSERT_UINT64((unsigned long long)written, ==, (unsigned long long)sizeof(msg));

    uint8_t buf[16] = { 0 };
    int32_t read = 0;
    DTERR_C(dtiox_read(h, buf, sizeof(buf), &read));

    DTUNITTEST_ASSERT_UINT64((unsigned long long)read, ==, (unsigned long long)sizeof(msg));
    DTUNITTEST_ASSERT_EQUAL_BYTES(buf, msg, sizeof(msg));

cleanup:
    dtiox_dispose(h);
    return dterr;
}

// -----------------------------------------------------------------------------
// 2) TX overflow should surface an error when capacity is small
//    Pattern: shrink capacity, write more than fits, assert DTERR_FAIL.

static dterr_t*
test_dtiox_dummy_tx_overflow_is_error(void)
{
    dterr_t* dterr = NULL;
    dtiox_handle h = NULL;

    dtiox_dummy_t* obj = NULL;
    DTERR_C(dtiox_dummy_create(&obj));

    // Shrink capacities to force overflow
    dtiox_dummy_behavior_t bh = { 0 };
    bh.loopback = false; // avoid immediate RX interaction
    bh.rx_capacity = 16;
    bh.tx_capacity = 8; // intentionally tiny
    DTERR_C(dtiox_dummy_set_behavior(obj, &bh));

    dtiox_dummy_config_t cfg = { //
        .name = "dummy2",
        .uart_config = dtuart_helper_default_config
    };
    DTERR_C(dtiox_dummy_configure(obj, &cfg));

    h = (dtiox_handle)obj;

    const uint8_t big[32] = { 0 }; // larger than tx_capacity
    int32_t wrote = 0;

    dterr = dtiox_write(h, big, sizeof(big), &wrote);
    DTUNITTEST_ASSERT_DTERR(dterr, DTERR_FAIL);
    // `wrote` can be <= capacity; not asserting exact count, just that it failed.

cleanup:
    dtiox_dispose(h);
    return NULL; // swallow after assertion
}

// -----------------------------------------------------------------------------
// 3) Behavior knobs: loopback OFF for manual inject, then ON for echo via read()
//    - With loopback == false, write() should not populate RX ring.
//    - Manual inject via dtiox_dummy_push_rx_bytes() should be visible via read().
//    - After loopback == true, write() should show up on read().

static dterr_t*
test_dtiox_dummy_behavior_knobs(void)
{
    dterr_t* dterr = NULL;
    dtiox_handle h = NULL;

    dtiox_dummy_t* obj = NULL;
    DTERR_C(dtiox_dummy_create(&obj));

    // Set desired behavior BEFORE configure/enable
    dtiox_dummy_behavior_t bh = { 0 };
    bh.loopback = false; // first, manual inject only
    bh.rx_capacity = 128;
    bh.tx_capacity = 128;
    DTERR_C(dtiox_dummy_set_behavior(obj, &bh));

    dtiox_dummy_config_t cfg = { //
        .name = "dummy3",
        .uart_config = dtuart_helper_default_config
    };
    DTERR_C(dtiox_dummy_configure(obj, &cfg));

    h = (dtiox_handle)obj;

    // Optional enable; backend records but RX ring behavior is explicit in tests
    DTERR_C(dtiox_enable(h, true));

    // With loopback OFF, write() should not produce RX data.
    const uint8_t tx1[] = { 'X', 'Y', 'Z' };
    int32_t w = 0;
    DTERR_C(dtiox_write(h, tx1, sizeof(tx1), &w));
    DTUNITTEST_ASSERT_UINT64((unsigned long long)w, ==, (unsigned long long)sizeof(tx1));

    uint8_t buf[16] = { 0 };
    int32_t rd = 0;
    DTERR_C(dtiox_read(h, buf, sizeof(buf), &rd));
    DTUNITTEST_ASSERT_UINT64((unsigned long long)rd, ==, 0ULL);

    // Manually inject RX bytes and confirm they are seen.
    const uint8_t injected[] = { 'A', 'B', 'C' };
    DTERR_C(dtiox_dummy_push_rx_bytes(obj, injected, sizeof(injected)));

    memset(buf, 0, sizeof(buf));
    rd = 0;
    DTERR_C(dtiox_read(h, buf, sizeof(buf), &rd));
    DTUNITTEST_ASSERT_UINT64((unsigned long long)rd, ==, (unsigned long long)sizeof(injected));
    DTUNITTEST_ASSERT_EQUAL_BYTES(buf, injected, sizeof(injected));

    // Flip loopback ON and verify write -> RX path via read()
    bh.loopback = true;
    DTERR_C(dtiox_dummy_set_behavior(obj, &bh));

    const uint8_t tx2[] = { '1', '2' };
    w = 0;
    DTERR_C(dtiox_write(h, tx2, sizeof(tx2), &w));
    DTUNITTEST_ASSERT_UINT64((unsigned long long)w, ==, (unsigned long long)sizeof(tx2));

    memset(buf, 0, sizeof(buf));
    rd = 0;
    DTERR_C(dtiox_read(h, buf, sizeof(buf), &rd));
    DTUNITTEST_ASSERT_UINT64((unsigned long long)rd, ==, (unsigned long long)sizeof(tx2));
    DTUNITTEST_ASSERT_EQUAL_BYTES(buf, tx2, sizeof(tx2));

cleanup:
    dtiox_dispose(h);
    return dterr;
}

// -----------------------------------------------------------------------------
// Suite entry point

void
test_dtmc_base_dtiox_dummy(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtiox_dummy_basic_facade_flow);
    DTUNITTEST_RUN_TEST(test_dtiox_dummy_tx_overflow_is_error);
    DTUNITTEST_RUN_TEST(test_dtiox_dummy_behavior_knobs);
}
