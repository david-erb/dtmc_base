/*
 * dtmcp4728_dummy -- Null-hardware test double for the dtmcp4728 DAC interface.
 *
 * Implements the full dtmcp4728 vtable (attach, detach, fast_write, multi_write,
 * sequential_write, single_write_eeprom, general_call_reset, general_call_wakeup,
 * general_call_software_update, read_all, to_string, dispose) without any I2C
 * or hardware dependency. Call counters and captured channel state let unit tests
 * verify the correct sequence of operations without physical MCP4728 hardware.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtmcp4728.h>

// -----------------------------------------------------------------------------
// Config

typedef struct dtmcp4728_dummy_config_t
{
    uint8_t device_address_7bit;
} dtmcp4728_dummy_config_t;

// -----------------------------------------------------------------------------
// Concrete type

typedef struct dtmcp4728_dummy_t
{
    DTMCP4728_COMMON_MEMBERS

    dtmcp4728_dummy_config_t config;

    bool is_configured;
    bool is_attached;

    // inject errors: set to a non-NULL dterr_t* to have the next call return it
    dterr_t* inject_attach_error;
    dterr_t* inject_fast_write_error;
    dterr_t* inject_multi_write_error;
    dterr_t* inject_sequential_write_error;
    dterr_t* inject_single_write_eeprom_error;
    dterr_t* inject_read_all_error;

    // call counters
    int32_t attach_call_count;
    int32_t detach_call_count;
    int32_t fast_write_call_count;
    int32_t multi_write_call_count;
    int32_t sequential_write_call_count;
    int32_t single_write_eeprom_call_count;
    int32_t general_call_reset_call_count;
    int32_t general_call_wakeup_call_count;
    int32_t general_call_software_update_call_count;
    int32_t read_all_call_count;
    int32_t dispose_call_count;

    // last channel state captured per write command
    dtmcp4728_channel_config_t last_fast_write[DTMCP4728_CHANNEL_COUNT];
    dtmcp4728_channel_config_t last_multi_write;
    dtmcp4728_channel_config_t last_sequential_write[DTMCP4728_CHANNEL_COUNT];
    int32_t last_sequential_write_count;
    dtmcp4728_channel_config_t last_single_write_eeprom;

    bool _is_malloced;
} dtmcp4728_dummy_t;

// -----------------------------------------------------------------------------
// Lifecycle

extern dterr_t*
dtmcp4728_dummy_create(dtmcp4728_dummy_t** self_ptr);

extern dterr_t*
dtmcp4728_dummy_init(dtmcp4728_dummy_t* self);

extern dterr_t*
dtmcp4728_dummy_configure(dtmcp4728_dummy_t* self, const dtmcp4728_dummy_config_t* config);

// -----------------------------------------------------------------------------
// Interface plumbing

DTMCP4728_DECLARE_API(dtmcp4728_dummy)
