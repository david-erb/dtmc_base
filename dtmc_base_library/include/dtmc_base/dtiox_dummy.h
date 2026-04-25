/*
 * dtiox_dummy -- In-memory test double for the dtiox byte-stream I/O interface.
 *
 * Implements the full dtiox vtable (attach, detach, enable, read, write,
 * set_rx_semaphore, concat_format, dispose) using internal ring buffers.
 * An optional loopback flag causes writes to appear immediately as received
 * data. The push_rx_bytes helper injects arbitrary byte sequences to exercise
 * receive-path logic without physical UART or socket hardware.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtiox.h>
#include <dtmc_base/dtuart_helpers.h>

typedef struct dtiox_dummy_t dtiox_dummy_t;

typedef struct
{
    // Backend interprets these fields; keep generic where possible.
    dtuart_helper_config_t uart_config;

    // Optional pin/port hints (backend-specific use)
    const char* name; // e.g., "uart0"
    uint8_t pin_tx;   // optional: global or SoC pin number
    uint8_t pin_rx;   // optional
    uint8_t pin_rts;  // optional (if flow == RTSCTS)
    uint8_t pin_cts;  // optional
} dtiox_dummy_config_t;

/* Create/Init/Configure — same shape you’ve used elsewhere */
dterr_t*
dtiox_dummy_create(dtiox_dummy_t** self_ptr);
dterr_t*
dtiox_dummy_init(dtiox_dummy_t* self);
dterr_t*
dtiox_dummy_configure(dtiox_dummy_t* self, const dtiox_dummy_config_t* cfg);

/* Testing helpers */
typedef struct
{
    bool loopback;       /* if true, write() immediately becomes RX */
    int32_t rx_capacity; /* default 1024 */
    int32_t tx_capacity; /* default 1024 */
} dtiox_dummy_behavior_t;

dterr_t*
dtiox_dummy_set_behavior(dtiox_dummy_t* self, const dtiox_dummy_behavior_t* b);
dterr_t*
dtiox_dummy_push_rx_bytes(dtiox_dummy_t* self, const uint8_t* data, int32_t len);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTIOX_DECLARE_API(dtiox_dummy);
