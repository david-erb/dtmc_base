/*
 * dtadc_scan -- Single ADC acquisition record with binary serialization.
 *
 * Represents one ADC sampling event: a nanosecond timestamp, a monotonic
 * sequence number, and a variable-length array of int32 channel readings.
 * The packx functions serialize and deserialize the scan to and from a
 * compact binary format, enabling transmission over serial links or network
 * portals with explicit offset tracking and bounds checking.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#define DTADC_SCAN_CHANNEL_READING_T int32_t

// a single scan possibly containing multiple channels
typedef struct dtadc_scan_t
{
    uint64_t timestamp_ns;
    uint64_t sequence_number;
    // make sure this is always last
    DTADC_SCAN_CHANNEL_READING_T* channels;
} dtadc_scan_t;

// --------------------------------------------------------------------------------------------
// Returns packed length
dterr_t*
dtadc_scan_packx_length(dtadc_scan_t* self, int32_t channel_count, int32_t* length);

// --------------------------------------------------------------------------------------------
// packs the scan into the output buffer
// start packing from *offset, and return the new offset there
// length is the total length of the output buffer, for bounds checking
dterr_t*
dtadc_scan_packx(dtadc_scan_t* self, int32_t channel_count, uint8_t* output, int32_t* offset, int32_t length);

// --------------------------------------------------------------------------------------------
// unpacks the scan from the input buffer
// start unpacking from *offset, and return the new offset there
// length is the total length of the input buffer, for bounds checking
//
// NOTE:
// self->channels must already point to storage for at least channel_count entries.
dterr_t*
dtadc_scan_unpackx(dtadc_scan_t* self, int32_t channel_count, const uint8_t* input, int32_t* offset, int32_t length);
