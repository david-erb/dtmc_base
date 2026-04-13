/*
 * dtadc_scanlist -- Batched ADC scan collection with binary serialization.
 *
 * Groups a variable number of dtadc_scan_t records and their shared channel
 * count into a single transferable unit. Provides init, pack-length query,
 * pack, unpack, and dispose, following the same packx conventions used
 * throughout the library. Suited for batching ADC samples before
 * transmission or logging.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtadc_scan.h>

#define DTADC_SCANS_CHANNEL_READING_T int32_t

// a scan list possibly containing multiple scans
typedef struct dtadc_scanlist_t
{
    int32_t scan_count;
    int32_t channel_count;
    dtadc_scan_t* scans;
} dtadc_scanlist_t;

// --------------------------------------------------------------------------------------------
// initialize and allocate
extern dterr_t*
dtadc_scanlist_init(dtadc_scanlist_t* self);

// --------------------------------------------------------------------------------------------
// Returns packed length
extern dterr_t*
dtadc_scanlist_packx_length(dtadc_scanlist_t* self, int32_t* length);

// --------------------------------------------------------------------------------------------
// packs the scanlist into the output buffer
// start packing from *offset, and return the new offset there
// length is the total length of the output buffer, for bounds checking
extern dterr_t*
dtadc_scanlist_packx(dtadc_scanlist_t* self, uint8_t* output, int32_t* offset, int32_t length);

// --------------------------------------------------------------------------------------------
// unpacks the scanlist from the input buffer
// start unpacking from *offset, and return the new offset there
// length is the total length of the input buffer, for bounds checking
extern dterr_t*
dtadc_scanlist_unpackx(dtadc_scanlist_t* self, const uint8_t* input, int32_t* offset, int32_t length);

// --------------------------------------------------------------------------------------------
extern void
dtadc_scanlist_dispose(dtadc_scanlist_t* self);