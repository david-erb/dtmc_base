#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtpackx.h>

#include <dtmc_base/dtadc_scan.h>
#include <dtmc_base/dtadc_scanlist.h>

#define TAG "dtadc_scanlist"

// --------------------------------------------------------------------------------------------
dterr_t*
dtadc_scanlist_init(dtadc_scanlist_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    if (self->scan_count <= 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "scan_count must be positive");
        goto cleanup;
    }

    if (self->channel_count <= 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel_count must be positive");
        goto cleanup;
    }

    int32_t scans_bytes = self->scan_count * (int32_t)sizeof(dtadc_scan_t);
    int32_t channels_bytes = self->scan_count * self->channel_count * (int32_t)sizeof(DTADC_SCANS_CHANNEL_READING_T);
    int32_t total_bytes = scans_bytes + channels_bytes;

    // single allocation for all scan and channel data
    DTERR_C(dtheaper_alloc(total_bytes, "dtadc_scanlist", (void**)&self->scans));

    // channels data starts after the contiguous block of scan structs
    DTADC_SCANS_CHANNEL_READING_T* channels_base = (DTADC_SCANS_CHANNEL_READING_T*)(self->scans + self->scan_count);

    for (int32_t i = 0; i < self->scan_count; i++)
    {
        self->scans[i].channels = channels_base + (i * self->channel_count);
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Returns packed length
dterr_t*
dtadc_scanlist_packx_length(dtadc_scanlist_t* self, int32_t* length)
{
    dterr_t* dterr = NULL;
    int32_t result = 0;
    int32_t i = 0;
    int32_t scan_length = 0;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(length);

    if (self->scan_count < 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "scan_count cannot be negative");
        goto cleanup;
    }

    if (self->channel_count < 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel_count cannot be negative");
        goto cleanup;
    }

    result += dtpackx_pack_int32_length(); // scan_count
    result += dtpackx_pack_int32_length(); // channel_count

    DTERR_C(dtadc_scan_packx_length(&self->scans[i], self->channel_count, &scan_length));
    result += scan_length * self->scan_count;

    *length = result;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
// packs the scanlist into the output buffer
// start packing from *offset, and return the new offset there
// length is the total length of the output buffer, for bounds checking
dterr_t*
dtadc_scanlist_packx(dtadc_scanlist_t* self, uint8_t* output, int32_t* offset, int32_t length)
{
    dterr_t* dterr = NULL;
    int32_t p = 0;
    int32_t i = 0;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(output);
    DTERR_ASSERT_NOT_NULL(offset);

    if (self->scan_count < 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "scan_count cannot be negative");
        goto cleanup;
    }

    if (self->channel_count < 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel_count cannot be negative");
        goto cleanup;
    }

    if (self->scan_count > 0 && self->scans == NULL)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "scans is NULL with nonzero scan_count");
        goto cleanup;
    }

    p = *offset;

    DTPACKX_PACK(dtpackx_pack_int32(self->scan_count, output, p, length), p, length);
    DTPACKX_PACK(dtpackx_pack_int32(self->channel_count, output, p, length), p, length);

    for (i = 0; i < self->scan_count; i++)
    {
        DTERR_C(dtadc_scan_packx(&self->scans[i], self->channel_count, output, &p, length));
    }

    *offset = p;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
// unpacks the scanlist from the input buffer
// start unpacking from *offset, and return the new offset there
// length is the total length of the input buffer, for bounds checking
dterr_t*
dtadc_scanlist_unpackx(dtadc_scanlist_t* self, const uint8_t* input, int32_t* offset, int32_t length)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(input);
    DTERR_ASSERT_NOT_NULL(offset);

    int32_t p = 0;
    int32_t i = 0;

    memset(self, 0, sizeof(*self));

    p = *offset;
    DTPACKX_UNPACK(dtpackx_unpack_int32(input, p, length, &self->scan_count), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_int32(input, p, length, &self->channel_count), p, length);
    dtadc_scanlist_init(self);

    for (i = 0; i < self->scan_count; i++)
    {
        DTERR_C(dtadc_scan_unpackx(&self->scans[i], self->channel_count, input, &p, length));
    }

    *offset = p;

cleanup:
    if (dterr != NULL)
    {
        dtadc_scanlist_dispose(self);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------------
void
dtadc_scanlist_dispose(dtadc_scanlist_t* self)
{
    if (self == NULL)
        return;

    dtheaper_free(self->scans);

    memset(self, 0, sizeof(*self));
}