#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtuart_helpers.h>

// -----------------------------------------------------------------------------
// Defaults
// -----------------------------------------------------------------------------

const dtuart_helper_config_t dtuart_helper_default_config = {
    .baudrate = 115200,
    .data_bits = DTUART_DATA_BITS_8,
    .parity = DTUART_PARITY_NONE,
    .stop_bits = DTUART_STOPBITS_2,
    .flow = DTUART_FLOW_NONE,
};

// -----------------------------------------------------------------------------
// Enum helpers
// -----------------------------------------------------------------------------

const char*
dtuart_parity_to_string(dtuart_parity_t parity)
{
    switch (parity)
    {
        case DTUART_PARITY_NONE:
            return "N";
        case DTUART_PARITY_EVEN:
            return "E";
        case DTUART_PARITY_ODD:
            return "O";
        default:
            return "Unknown";
    }
}

bool
dtuart_parity_is_valid(int32_t value)
{
    return value == DTUART_PARITY_NONE || value == DTUART_PARITY_EVEN || value == DTUART_PARITY_ODD;
}

// -----------------------------------------------------------------------------

const char*
dtuart_data_bits_to_string(dtuart_data_bits_t data_bits)
{
    switch (data_bits)
    {
        case DTUART_DATA_BITS_7:
            return "7";
        case DTUART_DATA_BITS_8:
            return "8";
        default:
            return "Unknown";
    }
}

bool
dtuart_data_bits_is_valid(int32_t value)
{
    return value == DTUART_DATA_BITS_7 || value == DTUART_DATA_BITS_8;
}

int32_t
dtuart_data_bits_to_int(dtuart_data_bits_t data_bits)
{
    switch (data_bits)
    {
        case DTUART_DATA_BITS_7:
            return 7;
        case DTUART_DATA_BITS_8:
            return 8;
        default:
            return 0;
    }
}

// -----------------------------------------------------------------------------

const char*
dtuart_stopbits_to_string(dtuart_stopbits_t stop_bits)
{
    switch (stop_bits)
    {
        case DTUART_STOPBITS_1:
            return "1";
        case DTUART_STOPBITS_2:
            return "2";
        default:
            return "Unknown";
    }
}

bool
dtuart_stopbits_is_valid(int32_t value)
{
    return value == DTUART_STOPBITS_1 || value == DTUART_STOPBITS_2;
}

// -----------------------------------------------------------------------------

const char*
dtuart_flow_to_string(dtuart_flow_t flow)
{
    switch (flow)
    {
        case DTUART_FLOW_NONE:
            return "None";
        case DTUART_FLOW_RTSCTS:
            return "RTSCTS";
        default:
            return "Unknown";
    }
}

bool
dtuart_flow_is_valid(int32_t value)
{
    return value == DTUART_FLOW_NONE || value == DTUART_FLOW_RTSCTS;
}

// -----------------------------------------------------------------------------
// Validation
// -----------------------------------------------------------------------------

dterr_t*
dtuart_helper_validate(const dtuart_helper_config_t* cfg)
{
    if (!cfg)
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "UART config pointer is NULL");

    if (cfg->baudrate <= 0)
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "UART baudrate must be > 0 (got %" PRId32 ")", cfg->baudrate);

    if (!dtuart_parity_is_valid((int32_t)cfg->parity))
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "UART parity is invalid (got %" PRId32 ")", (int32_t)cfg->parity);

    if (!dtuart_data_bits_is_valid((int32_t)cfg->data_bits))
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "UART data_bits is invalid (got %" PRId32 ")", (int32_t)cfg->data_bits);

    if (!dtuart_stopbits_is_valid((int32_t)cfg->stop_bits))
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "UART stop_bits is invalid (got %" PRId32 ")", (int32_t)cfg->stop_bits);

    if (!dtuart_flow_is_valid((int32_t)cfg->flow))
        return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, "UART flow is invalid (got %" PRId32 ")", (int32_t)cfg->flow);

    return NULL;
}

// -----------------------------------------------------------------------------
// Format to string
// -----------------------------------------------------------------------------

void
dtuart_helper_to_string(const dtuart_helper_config_t* cfg, char* out_str, int32_t out_str_size)
{
    if (!cfg)
        return;

    const char* parity = dtuart_parity_to_string(cfg->parity);
    const char* data_bits = dtuart_data_bits_to_string(cfg->data_bits);
    const char* stop_bits = dtuart_stopbits_to_string(cfg->stop_bits);
    const char* flow = dtuart_flow_to_string(cfg->flow);

    char* format;
    if (cfg->flow == DTUART_FLOW_NONE)
        format = "%" PRId32 ",%s,%s,%s";
    else
        format = "%" PRId32 ",%s,%s,%s,%s";

    snprintf(out_str, out_str_size, format, cfg->baudrate, data_bits, parity, stop_bits, flow);
}
