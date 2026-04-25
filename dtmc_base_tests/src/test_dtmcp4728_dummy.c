#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtunittest.h>

#include <dtmc_base/dtmcp4728.h>
#include <dtmc_base/dtmcp4728_dummy.h>

// --------------------------------------------------------------------------------------------
// Helpers

static dterr_t*
make_attached(dtmcp4728_dummy_t** out)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(dtmcp4728_dummy_create(&obj));

    dtmcp4728_dummy_config_t cfg = { .device_address_7bit = 0x60 };
    DTERR_C(dtmcp4728_dummy_configure(obj, &cfg));
    DTERR_C(dtmcp4728_dummy_attach(obj));

    *out = obj;

cleanup:
    if (dterr && obj)
        dtmcp4728_dummy_dispose(obj);
    return dterr;
}

static dtmcp4728_channel_config_t
make_channel(dtmcp4728_channel_t ch, uint16_t val)
{
    dtmcp4728_channel_config_t c = { 0 };
    c.channel = ch;
    c.value_12bit = val;
    c.vref = DTMCP4728_VREF_VDD;
    c.power_down = DTMCP4728_POWER_DOWN_NORMAL;
    c.gain = DTMCP4728_GAIN_X1;
    c.udac = false;
    return c;
}

// --------------------------------------------------------------------------------------------
// 1) Lifecycle: create -> configure -> attach -> detach -> dispose

static dterr_t*
test_dtmcp4728_dummy_lifecycle(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(dtmcp4728_dummy_create(&obj));
    DTUNITTEST_ASSERT_TRUE(!obj->is_configured);
    DTUNITTEST_ASSERT_TRUE(!obj->is_attached);

    dtmcp4728_dummy_config_t cfg = { .device_address_7bit = 0x61 };
    DTERR_C(dtmcp4728_dummy_configure(obj, &cfg));
    DTUNITTEST_ASSERT_TRUE(obj->is_configured);
    DTUNITTEST_ASSERT_INT(obj->config.device_address_7bit, ==, 0x61);

    DTERR_C(dtmcp4728_dummy_attach(obj));
    DTUNITTEST_ASSERT_TRUE(obj->is_attached);
    DTUNITTEST_ASSERT_INT(obj->attach_call_count, ==, 1);

    DTERR_C(dtmcp4728_dummy_detach(obj));
    DTUNITTEST_ASSERT_TRUE(!obj->is_attached);
    DTUNITTEST_ASSERT_INT(obj->detach_call_count, ==, 1);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 2) Facade vtable dispatch via opaque handle

static dterr_t*
test_dtmcp4728_dummy_facade_dispatch(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;
    dtmcp4728_handle h = NULL;

    DTERR_C(make_attached(&obj));

    h = (dtmcp4728_handle)obj;

    dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT];
    for (int i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        channels[i] = make_channel((dtmcp4728_channel_t)i, (uint16_t)(100 * (i + 1)));

    DTERR_C(dtmcp4728_fast_write(h, channels));
    DTUNITTEST_ASSERT_INT(obj->fast_write_call_count, ==, 1);

cleanup:
    dtmcp4728_dispose(h);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 3) fast_write: call counter and all-channel capture

static dterr_t*
test_dtmcp4728_dummy_fast_write(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT];
    channels[0] = make_channel(DTMCP4728_CHANNEL_A, 0x100);
    channels[1] = make_channel(DTMCP4728_CHANNEL_B, 0x200);
    channels[2] = make_channel(DTMCP4728_CHANNEL_C, 0x300);
    channels[3] = make_channel(DTMCP4728_CHANNEL_D, 0x400);

    DTERR_C(dtmcp4728_dummy_fast_write(obj, channels));

    DTUNITTEST_ASSERT_INT(obj->fast_write_call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(obj->last_fast_write[0].value_12bit, ==, 0x100);
    DTUNITTEST_ASSERT_INT(obj->last_fast_write[1].value_12bit, ==, 0x200);
    DTUNITTEST_ASSERT_INT(obj->last_fast_write[2].value_12bit, ==, 0x300);
    DTUNITTEST_ASSERT_INT(obj->last_fast_write[3].value_12bit, ==, 0x400);

    // second call updates capture
    channels[0].value_12bit = 0xFFF;
    DTERR_C(dtmcp4728_dummy_fast_write(obj, channels));
    DTUNITTEST_ASSERT_INT(obj->fast_write_call_count, ==, 2);
    DTUNITTEST_ASSERT_INT(obj->last_fast_write[0].value_12bit, ==, 0xFFF);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 4) multi_write: call counter and single-channel capture

static dterr_t*
test_dtmcp4728_dummy_multi_write(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dtmcp4728_channel_config_t cfg = make_channel(DTMCP4728_CHANNEL_C, 0x7FF);
    cfg.vref = DTMCP4728_VREF_INTERNAL;
    cfg.gain = DTMCP4728_GAIN_X2;

    DTERR_C(dtmcp4728_dummy_multi_write(obj, &cfg));

    DTUNITTEST_ASSERT_INT(obj->multi_write_call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(obj->last_multi_write.channel, ==, DTMCP4728_CHANNEL_C);
    DTUNITTEST_ASSERT_INT(obj->last_multi_write.value_12bit, ==, 0x7FF);
    DTUNITTEST_ASSERT_INT(obj->last_multi_write.vref, ==, DTMCP4728_VREF_INTERNAL);
    DTUNITTEST_ASSERT_INT(obj->last_multi_write.gain, ==, DTMCP4728_GAIN_X2);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 5) sequential_write: partial and full sequences, capture count

static dterr_t*
test_dtmcp4728_dummy_sequential_write(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    // Write channels B and C (2 channels starting from B)
    dtmcp4728_channel_config_t cfgs[2];
    cfgs[0] = make_channel(DTMCP4728_CHANNEL_B, 0x111);
    cfgs[1] = make_channel(DTMCP4728_CHANNEL_C, 0x222);

    DTERR_C(dtmcp4728_dummy_sequential_write(obj, DTMCP4728_CHANNEL_B, cfgs, 2));

    DTUNITTEST_ASSERT_INT(obj->sequential_write_call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(obj->last_sequential_write_count, ==, 2);
    DTUNITTEST_ASSERT_INT(obj->last_sequential_write[0].value_12bit, ==, 0x111);
    DTUNITTEST_ASSERT_INT(obj->last_sequential_write[1].value_12bit, ==, 0x222);

    // Full 4-channel write from A
    dtmcp4728_channel_config_t all[DTMCP4728_CHANNEL_COUNT];
    for (int i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        all[i] = make_channel((dtmcp4728_channel_t)i, (uint16_t)(0x010 * (i + 1)));

    DTERR_C(dtmcp4728_dummy_sequential_write(obj, DTMCP4728_CHANNEL_A, all, DTMCP4728_CHANNEL_COUNT));
    DTUNITTEST_ASSERT_INT(obj->sequential_write_call_count, ==, 2);
    DTUNITTEST_ASSERT_INT(obj->last_sequential_write_count, ==, DTMCP4728_CHANNEL_COUNT);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 6) single_write_eeprom: call counter and capture

static dterr_t*
test_dtmcp4728_dummy_single_write_eeprom(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dtmcp4728_channel_config_t cfg = make_channel(DTMCP4728_CHANNEL_D, 0xABC);
    cfg.power_down = DTMCP4728_POWER_DOWN_100K;

    DTERR_C(dtmcp4728_dummy_single_write_eeprom(obj, &cfg));

    DTUNITTEST_ASSERT_INT(obj->single_write_eeprom_call_count, ==, 1);
    DTUNITTEST_ASSERT_INT(obj->last_single_write_eeprom.channel, ==, DTMCP4728_CHANNEL_D);
    DTUNITTEST_ASSERT_INT(obj->last_single_write_eeprom.value_12bit, ==, 0xABC);
    DTUNITTEST_ASSERT_INT(obj->last_single_write_eeprom.power_down, ==, DTMCP4728_POWER_DOWN_100K);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 7) General calls: counters increment

static dterr_t*
test_dtmcp4728_dummy_general_calls(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    DTERR_C(dtmcp4728_dummy_general_call_reset(obj));
    DTUNITTEST_ASSERT_INT(obj->general_call_reset_call_count, ==, 1);

    DTERR_C(dtmcp4728_dummy_general_call_wakeup(obj));
    DTUNITTEST_ASSERT_INT(obj->general_call_wakeup_call_count, ==, 1);

    DTERR_C(dtmcp4728_dummy_general_call_software_update(obj));
    DTUNITTEST_ASSERT_INT(obj->general_call_software_update_call_count, ==, 1);

    // second calls still counted
    DTERR_C(dtmcp4728_dummy_general_call_reset(obj));
    DTUNITTEST_ASSERT_INT(obj->general_call_reset_call_count, ==, 2);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 8) read_all: returns last fast_write snapshot

static dterr_t*
test_dtmcp4728_dummy_read_all(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dtmcp4728_channel_config_t written[DTMCP4728_CHANNEL_COUNT];
    for (int i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        written[i] = make_channel((dtmcp4728_channel_t)i, (uint16_t)(0x100 + i));

    DTERR_C(dtmcp4728_dummy_fast_write(obj, written));

    dtmcp4728_channel_config_t readback[DTMCP4728_CHANNEL_COUNT];
    memset(readback, 0, sizeof(readback));
    DTERR_C(dtmcp4728_dummy_read_all(obj, readback));

    DTUNITTEST_ASSERT_INT(obj->read_all_call_count, ==, 1);
    for (int i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        DTUNITTEST_ASSERT_INT(readback[i].value_12bit, ==, (int)(0x100 + i));

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 9) to_string: produces non-empty, null-terminated output

static dterr_t*
test_dtmcp4728_dummy_to_string(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    char buf[128];
    DTERR_C(dtmcp4728_dummy_to_string(obj, buf, sizeof(buf)));
    DTUNITTEST_ASSERT_TRUE(buf[0] != '\0');
    DTUNITTEST_ASSERT_TRUE(buf[sizeof(buf) - 1] == '\0');

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 10) Guard: operations fail when not attached

static dterr_t*
test_dtmcp4728_dummy_not_attached_guard(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(dtmcp4728_dummy_create(&obj));

    dtmcp4728_dummy_config_t cfg = { .device_address_7bit = 0x60 };
    DTERR_C(dtmcp4728_dummy_configure(obj, &cfg));
    // deliberately not attaching

    dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT];
    for (int i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        channels[i] = make_channel((dtmcp4728_channel_t)i, 0);

    dterr_t* err = dtmcp4728_dummy_fast_write(obj, channels);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADCONFIG);
    dterr_dispose(err);

    err = dtmcp4728_dummy_general_call_reset(obj);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADCONFIG);
    dterr_dispose(err);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 11) Guard: double-attach fails

static dterr_t*
test_dtmcp4728_dummy_double_attach_fails(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dterr_t* err = dtmcp4728_dummy_attach(obj);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADCONFIG);
    dterr_dispose(err);

    DTUNITTEST_ASSERT_INT(obj->attach_call_count, ==, 2);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 12) Guard: configure while attached fails

static dterr_t*
test_dtmcp4728_dummy_configure_while_attached_fails(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dtmcp4728_dummy_config_t cfg2 = { .device_address_7bit = 0x62 };
    dterr_t* err = dtmcp4728_dummy_configure(obj, &cfg2);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADCONFIG);
    dterr_dispose(err);

    // original address unchanged
    DTUNITTEST_ASSERT_INT(obj->config.device_address_7bit, ==, 0x60);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 13) Validation: value_12bit out of range

static dterr_t*
test_dtmcp4728_dummy_invalid_value(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT];
    for (int i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        channels[i] = make_channel((dtmcp4728_channel_t)i, 0);
    channels[2].value_12bit = 0x1000; // one over the 12-bit max

    dterr_t* err = dtmcp4728_dummy_fast_write(obj, channels);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADARG);
    dterr_dispose(err);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 14) Validation: sequential_write channel order mismatch

static dterr_t*
test_dtmcp4728_dummy_sequential_write_order_mismatch(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    dtmcp4728_channel_config_t cfgs[2];
    cfgs[0] = make_channel(DTMCP4728_CHANNEL_C, 0x100); // wrong: should be A
    cfgs[1] = make_channel(DTMCP4728_CHANNEL_B, 0x200);

    dterr_t* err = dtmcp4728_dummy_sequential_write(obj, DTMCP4728_CHANNEL_A, cfgs, 2);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_BADARG);
    dterr_dispose(err);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 15) Error injection: inject_attach_error

static dterr_t*
test_dtmcp4728_dummy_inject_attach_error(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(dtmcp4728_dummy_create(&obj));

    dtmcp4728_dummy_config_t cfg = { .device_address_7bit = 0x60 };
    DTERR_C(dtmcp4728_dummy_configure(obj, &cfg));

    obj->inject_attach_error = dterr_new(DTERR_STATE, DTERR_LOC, NULL, "injected attach failure");

    dterr_t* err = dtmcp4728_dummy_attach(obj);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_STATE);
    dterr_dispose(err);

    DTUNITTEST_ASSERT_TRUE(!obj->is_attached);
    DTUNITTEST_ASSERT_INT(obj->attach_call_count, ==, 1);

    // inject is consumed; second attach succeeds
    DTERR_C(dtmcp4728_dummy_attach(obj));
    DTUNITTEST_ASSERT_TRUE(obj->is_attached);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// 16) Error injection: inject_fast_write_error

static dterr_t*
test_dtmcp4728_dummy_inject_fast_write_error(void)
{
    dterr_t* dterr = NULL;
    dtmcp4728_dummy_t* obj = NULL;

    DTERR_C(make_attached(&obj));

    obj->inject_fast_write_error = dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "injected write failure");

    dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT];
    for (int i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        channels[i] = make_channel((dtmcp4728_channel_t)i, (uint16_t)(i * 10));

    dterr_t* err = dtmcp4728_dummy_fast_write(obj, channels);
    DTUNITTEST_ASSERT_DTERR(err, DTERR_FAIL);
    dterr_dispose(err);

    DTUNITTEST_ASSERT_INT(obj->fast_write_call_count, ==, 1);

    // inject is consumed; next call succeeds
    DTERR_C(dtmcp4728_dummy_fast_write(obj, channels));
    DTUNITTEST_ASSERT_INT(obj->fast_write_call_count, ==, 2);

cleanup:
    dtmcp4728_dummy_dispose(obj);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Suite entry point

void
test_dtmc_base_dtmcp4728_dummy(DTUNITTEST_SUITE_ARGS)
{
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_lifecycle);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_facade_dispatch);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_fast_write);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_multi_write);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_sequential_write);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_single_write_eeprom);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_general_calls);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_read_all);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_to_string);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_not_attached_guard);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_double_attach_fails);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_configure_while_attached_fails);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_invalid_value);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_sequential_write_order_mismatch);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_inject_attach_error);
    DTUNITTEST_RUN_TEST(test_dtmcp4728_dummy_inject_fast_write_error);
}
