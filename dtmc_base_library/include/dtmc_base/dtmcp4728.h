/*
 * dtmcp4728 -- Facade for the MCP4728 quad 12-bit DAC over I2C.
 *
 * Defines a vtable-dispatched interface for a Microchip MCP4728 four-channel
 * 12-bit DAC. Each channel is independently configured with an output value, voltage reference and gain.
 *
 * A global model-number registry allows platform
 * implementations to register and be resolved at runtime without recompiling
 * call sites.
 *
 * cdox v1.0.2.1
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#define DTMCP4728_CHANNEL_COUNT (4)

// -----------------------------------------------------------------------------
// Opaque handle

struct dtmcp4728_handle_t;
typedef struct dtmcp4728_handle_t* dtmcp4728_handle;

// -----------------------------------------------------------------------------
// Shared types

typedef enum dtmcp4728_channel_t
{
    DTMCP4728_CHANNEL_A = 0,
    DTMCP4728_CHANNEL_B = 1,
    DTMCP4728_CHANNEL_C = 2,
    DTMCP4728_CHANNEL_D = 3
} dtmcp4728_channel_t;

typedef enum dtmcp4728_vref_t
{
    DTMCP4728_VREF_VDD = 0,
    DTMCP4728_VREF_INTERNAL = 1
} dtmcp4728_vref_t;

typedef enum dtmcp4728_power_down_t
{
    DTMCP4728_POWER_DOWN_NORMAL = 0,
    DTMCP4728_POWER_DOWN_1K = 1,
    DTMCP4728_POWER_DOWN_100K = 2,
    DTMCP4728_POWER_DOWN_500K = 3
} dtmcp4728_power_down_t;

typedef enum dtmcp4728_gain_t
{
    DTMCP4728_GAIN_X1 = 0,
    DTMCP4728_GAIN_X2 = 1
} dtmcp4728_gain_t;

typedef struct dtmcp4728_channel_config_t
{
    dtmcp4728_channel_t channel;
    uint16_t value_12bit;
    dtmcp4728_vref_t vref;
    dtmcp4728_power_down_t power_down;
    dtmcp4728_gain_t gain;
    bool udac;
} dtmcp4728_channel_config_t;

// -----------------------------------------------------------------------------
// Argument helpers

#define DTMCP4728_ATTACH_ARGS
#define DTMCP4728_ATTACH_PARAMS

#define DTMCP4728_DETACH_ARGS
#define DTMCP4728_DETACH_PARAMS

#define DTMCP4728_FAST_WRITE_ARGS , const dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT]
#define DTMCP4728_FAST_WRITE_PARAMS , channels

#define DTMCP4728_MULTI_WRITE_ARGS , const dtmcp4728_channel_config_t* channel_config
#define DTMCP4728_MULTI_WRITE_PARAMS , channel_config

#define DTMCP4728_SEQUENTIAL_WRITE_ARGS                                                                                        \
    , dtmcp4728_channel_t start_channel, const dtmcp4728_channel_config_t *channel_configs, int32_t channel_count
#define DTMCP4728_SEQUENTIAL_WRITE_PARAMS , start_channel, channel_configs, channel_count

#define DTMCP4728_SINGLE_WRITE_EEPROM_ARGS , const dtmcp4728_channel_config_t* channel_config
#define DTMCP4728_SINGLE_WRITE_EEPROM_PARAMS , channel_config

#define DTMCP4728_GENERAL_CALL_RESET_ARGS
#define DTMCP4728_GENERAL_CALL_RESET_PARAMS

#define DTMCP4728_GENERAL_CALL_WAKEUP_ARGS
#define DTMCP4728_GENERAL_CALL_WAKEUP_PARAMS

#define DTMCP4728_GENERAL_CALL_SOFTWARE_UPDATE_ARGS
#define DTMCP4728_GENERAL_CALL_SOFTWARE_UPDATE_PARAMS

#define DTMCP4728_READ_ALL_ARGS , dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT]
#define DTMCP4728_READ_ALL_PARAMS , channels

#define DTMCP4728_TO_STRING_ARGS , char *buffer, int32_t buffer_size
#define DTMCP4728_TO_STRING_PARAMS , buffer, buffer_size

// -----------------------------------------------------------------------------
// Delegate (vtable) signatures

typedef dterr_t* (*dtmcp4728_attach_fn)(void* self DTMCP4728_ATTACH_ARGS);
typedef dterr_t* (*dtmcp4728_detach_fn)(void* self DTMCP4728_DETACH_ARGS);
typedef void (*dtmcp4728_dispose_fn)(void* self);
typedef dterr_t* (*dtmcp4728_fast_write_fn)(void* self DTMCP4728_FAST_WRITE_ARGS);
typedef dterr_t* (*dtmcp4728_multi_write_fn)(void* self DTMCP4728_MULTI_WRITE_ARGS);
typedef dterr_t* (*dtmcp4728_sequential_write_fn)(void* self DTMCP4728_SEQUENTIAL_WRITE_ARGS);
typedef dterr_t* (*dtmcp4728_single_write_eeprom_fn)(void* self DTMCP4728_SINGLE_WRITE_EEPROM_ARGS);
typedef dterr_t* (*dtmcp4728_general_call_reset_fn)(void* self DTMCP4728_GENERAL_CALL_RESET_ARGS);
typedef dterr_t* (*dtmcp4728_general_call_wakeup_fn)(void* self DTMCP4728_GENERAL_CALL_WAKEUP_ARGS);
typedef dterr_t* (*dtmcp4728_general_call_software_update_fn)(void* self DTMCP4728_GENERAL_CALL_SOFTWARE_UPDATE_ARGS);
typedef dterr_t* (*dtmcp4728_read_all_fn)(void* self DTMCP4728_READ_ALL_ARGS);
typedef dterr_t* (*dtmcp4728_to_string_fn)(void* self DTMCP4728_TO_STRING_ARGS);

// -----------------------------------------------------------------------------
// Virtual table type

typedef struct dtmcp4728_vt_t
{
    dtmcp4728_attach_fn attach;
    dtmcp4728_detach_fn detach;
    dtmcp4728_dispose_fn dispose;
    dtmcp4728_fast_write_fn fast_write;
    dtmcp4728_multi_write_fn multi_write;
    dtmcp4728_sequential_write_fn sequential_write;
    dtmcp4728_single_write_eeprom_fn single_write_eeprom;
    dtmcp4728_general_call_reset_fn general_call_reset;
    dtmcp4728_general_call_wakeup_fn general_call_wakeup;
    dtmcp4728_general_call_software_update_fn general_call_software_update;
    dtmcp4728_read_all_fn read_all;
    dtmcp4728_to_string_fn to_string;
} dtmcp4728_vt_t;

// -----------------------------------------------------------------------------
// Vtable registry

extern dterr_t*
dtmcp4728_set_vtable(int32_t model_number, dtmcp4728_vt_t* vtable);

extern dterr_t*
dtmcp4728_get_vtable(int32_t model_number, dtmcp4728_vt_t** vtable);

// -----------------------------------------------------------------------------
// Declaration helpers

#define DTMCP4728_DECLARE_API_EX(NAME, T)                                                                                      \
    extern dterr_t* NAME##_attach(NAME##T self DTMCP4728_ATTACH_ARGS);                                                         \
    extern dterr_t* NAME##_detach(NAME##T self DTMCP4728_DETACH_ARGS);                                                         \
    extern void NAME##_dispose(NAME##T self);                                                                                  \
    extern dterr_t* NAME##_fast_write(NAME##T self DTMCP4728_FAST_WRITE_ARGS);                                                 \
    extern dterr_t* NAME##_multi_write(NAME##T self DTMCP4728_MULTI_WRITE_ARGS);                                               \
    extern dterr_t* NAME##_sequential_write(NAME##T self DTMCP4728_SEQUENTIAL_WRITE_ARGS);                                     \
    extern dterr_t* NAME##_single_write_eeprom(NAME##T self DTMCP4728_SINGLE_WRITE_EEPROM_ARGS);                               \
    extern dterr_t* NAME##_general_call_reset(NAME##T self DTMCP4728_GENERAL_CALL_RESET_ARGS);                                 \
    extern dterr_t* NAME##_general_call_wakeup(NAME##T self DTMCP4728_GENERAL_CALL_WAKEUP_ARGS);                               \
    extern dterr_t* NAME##_general_call_software_update(NAME##T self DTMCP4728_GENERAL_CALL_SOFTWARE_UPDATE_ARGS);             \
    extern dterr_t* NAME##_read_all(NAME##T self DTMCP4728_READ_ALL_ARGS);                                                     \
    extern dterr_t* NAME##_to_string(NAME##T self DTMCP4728_TO_STRING_ARGS);

// declare dispatcher
DTMCP4728_DECLARE_API_EX(dtmcp4728, _handle)

// declare implementation (put this in its .h file)
#define DTMCP4728_DECLARE_API(NAME) DTMCP4728_DECLARE_API_EX(NAME, _t*)

// -----------------------------------------------------------------------------
// Common struct prefix

#define DTMCP4728_COMMON_MEMBERS int32_t model_number;

// -----------------------------------------------------------------------------
// Vtable initializer helper

#define DTMCP4728_INIT_VTABLE(NAME)                                                                                            \
    static dtmcp4728_vt_t NAME##_vt = {                                                                                        \
        .attach = (dtmcp4728_attach_fn)NAME##_attach,                                                                          \
        .detach = (dtmcp4728_detach_fn)NAME##_detach,                                                                          \
        .dispose = (dtmcp4728_dispose_fn)NAME##_dispose,                                                                       \
        .fast_write = (dtmcp4728_fast_write_fn)NAME##_fast_write,                                                              \
        .multi_write = (dtmcp4728_multi_write_fn)NAME##_multi_write,                                                           \
        .sequential_write = (dtmcp4728_sequential_write_fn)NAME##_sequential_write,                                            \
        .single_write_eeprom = (dtmcp4728_single_write_eeprom_fn)NAME##_single_write_eeprom,                                   \
        .general_call_reset = (dtmcp4728_general_call_reset_fn)NAME##_general_call_reset,                                      \
        .general_call_wakeup = (dtmcp4728_general_call_wakeup_fn)NAME##_general_call_wakeup,                                   \
        .general_call_software_update = (dtmcp4728_general_call_software_update_fn)NAME##_general_call_software_update,        \
        .read_all = (dtmcp4728_read_all_fn)NAME##_read_all,                                                                    \
        .to_string = (dtmcp4728_to_string_fn)NAME##_to_string,                                                                 \
    }
