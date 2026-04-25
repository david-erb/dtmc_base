#include <dtcore/dterr.h>
#include <dtcore/dtpackx.h>

#include <dtmc_base/dtadc_scan.h>

// --------------------------------------------------------------------------------------------
// Returns packed length
dterr_t*
dtadc_scan_packx_length(dtadc_scan_t* self, int32_t channel_count, int32_t* length)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(length);

    if (channel_count < 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel_count cannot be negative");
        goto cleanup;
    }

    *length = 0;

    *length += dtpackx_pack_int64_length();                   // timestamp_ns
    *length += dtpackx_pack_int64_length();                   // sequence_number
    *length += (channel_count * dtpackx_pack_int32_length()); // channels

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
// packs the scan into the output buffer
// start packing from *offset, and return the new offset there
// length is the total length of the output buffer, for bounds checking
dterr_t*
dtadc_scan_packx(dtadc_scan_t* self, int32_t channel_count, uint8_t* output, int32_t* offset, int32_t length)
{
    dterr_t* dterr = NULL;
    int32_t p = 0;
    int32_t i = 0;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->channels);
    DTERR_ASSERT_NOT_NULL(output);
    DTERR_ASSERT_NOT_NULL(offset);

    if (channel_count < 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel_count cannot be negative");
        goto cleanup;
    }

    p = *offset;

    DTPACKX_PACK(dtpackx_pack_int64((int64_t)self->timestamp_ns, output, p, length), p, length);
    DTPACKX_PACK(dtpackx_pack_int64((int64_t)self->sequence_number, output, p, length), p, length);

    for (i = 0; i < channel_count; i++)
    {
        DTPACKX_PACK(dtpackx_pack_int32((int32_t)self->channels[i], output, p, length), p, length);
    }

    *offset = p;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
// unpacks the scan from the input buffer
// start unpacking from *offset, and return the new offset there
// length is the total length of the input buffer, for bounds checking
//
// NOTE:
// self->channels must already point to storage for at least channel_count entries.
dterr_t*
dtadc_scan_unpackx(dtadc_scan_t* self, int32_t channel_count, const uint8_t* input, int32_t* offset, int32_t length)
{
    dterr_t* dterr = NULL;
    int32_t p = 0;
    int32_t i = 0;
    int64_t timestamp_ns = 0;
    int64_t sequence_number = 0;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->channels);
    DTERR_ASSERT_NOT_NULL(input);
    DTERR_ASSERT_NOT_NULL(offset);

    if (channel_count < 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel_count cannot be negative");
        goto cleanup;
    }

    p = *offset;

    DTPACKX_UNPACK(dtpackx_unpack_int64(input, p, length, &timestamp_ns), p, length);
    self->timestamp_ns = (uint64_t)timestamp_ns;

    DTPACKX_UNPACK(dtpackx_unpack_int64(input, p, length, &sequence_number), p, length);
    self->sequence_number = (uint64_t)sequence_number;

    for (i = 0; i < channel_count; i++)
    {
        int32_t value = 0;
        DTPACKX_UNPACK(dtpackx_unpack_int32(input, p, length, &value), p, length);
        self->channels[i] = (DTADC_SCAN_CHANNEL_READING_T)value;
    }

    *offset = p;

cleanup:
    return dterr;
}