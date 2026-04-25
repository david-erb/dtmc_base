/*
 * dtuart_helpers -- UART parameter types with validation and string conversion.
 *
 * Defines enumerations for parity, data bits, stop bits, and flow control,
 * each with an is_valid predicate and a to_string formatter. A unified
 * dtuart_helper_config_t bundles all parameters alongside a baud rate; a
 * validate function reports the first invalid field via dterr_t. Shared by
 * UART-backed dtiox implementations across ESP-IDF, Zephyr, PicoSDK, and
 * Linux backends.
 *
 * cdox v1.0.2
 */
#pragma once

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

// -----------------------------------------------------------------------------
// UART configuration enums
// Note: 0 is intentionally NOT a valid value for any enum.
// -----------------------------------------------------------------------------

typedef enum
{
    DTUART_PARITY_NONE = 1,
    DTUART_PARITY_EVEN,
    DTUART_PARITY_ODD,
} dtuart_parity_t;

const char*
dtuart_parity_to_string(dtuart_parity_t parity);

bool
dtuart_parity_is_valid(int32_t value);

// -----------------------------------------------------------------------------

typedef enum
{
    // Intentionally do NOT use 7/8 directly, so that "0" is not meaningful and
    // future expansion is easier without overlapping with other numeric fields.
    DTUART_DATA_BITS_7 = 1,
    DTUART_DATA_BITS_8,
} dtuart_data_bits_t;

const char*
dtuart_data_bits_to_string(dtuart_data_bits_t data_bits);

bool
dtuart_data_bits_is_valid(int32_t value);

int32_t
dtuart_data_bits_to_int(dtuart_data_bits_t data_bits);

// -----------------------------------------------------------------------------

typedef enum
{
    DTUART_STOPBITS_1 = 1,
    DTUART_STOPBITS_2,
} dtuart_stopbits_t;

const char*
dtuart_stopbits_to_string(dtuart_stopbits_t stop_bits);

bool
dtuart_stopbits_is_valid(int32_t value);

// -----------------------------------------------------------------------------

typedef enum
{
    DTUART_FLOW_NONE = 1,
    DTUART_FLOW_RTSCTS,
} dtuart_flow_t;

const char*
dtuart_flow_to_string(dtuart_flow_t flow);

bool
dtuart_flow_is_valid(int32_t value);

// -----------------------------------------------------------------------------
// Unified configuration bundle
// -----------------------------------------------------------------------------

typedef struct dtuart_helper_config_t
{
    int32_t baudrate; // must be > 0

    dtuart_parity_t parity;
    dtuart_data_bits_t data_bits;
    dtuart_stopbits_t stop_bits;
    dtuart_flow_t flow;
} dtuart_helper_config_t;

// Default config instance (exported).
extern const dtuart_helper_config_t dtuart_helper_default_config;

// -----------------------------------------------------------------------------
// Validate a config.
// Returns NULL if OK; otherwise returns dterr_t* describing what's invalid.
// -----------------------------------------------------------------------------
extern dterr_t*
dtuart_helper_validate(const dtuart_helper_config_t* cfg);

// -----------------------------------------------------------------------------
// Returns a heap-allocated string describing all parameters.
// Caller must free() the returned string.
// Returns NULL on allocation failure.
// -----------------------------------------------------------------------------
extern void
dtuart_helper_to_string(const dtuart_helper_config_t* cfg, char* out_str, int32_t out_str_size);
