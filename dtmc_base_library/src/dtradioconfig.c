#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtpackable.h>

#include <dtcore/dtpackx.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtnvblob.h>
#include <dtmc_base/dtradioconfig.h>

#define TAG "dtradioconfig"

// -------------------------------------------------------------------------------
dterr_t*
dtradioconfig_init(dtradioconfig_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    memset(self, 0, sizeof(*self));
cleanup:
    return dterr;
}

// -------------------------------------------------------------------------------
dterr_t*
dtradioconfig_write_nvblob(dtradioconfig_t* self, dtnvblob_handle nvblob_handle)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(nvblob_handle);

    // compute the packed length
    int32_t pack_length;
    DTERR_C(dtradioconfig_packx_length(self, &pack_length));

    // create a buffer to hold the packed data

    DTERR_C(dtbuffer_create(&buffer, pack_length));

    // pack the data into the buffer

    DTERR_C(dtradioconfig_packx(self, (uint8_t*)buffer->payload, NULL, buffer->length));

    // write the buffer to the nvblob

    int32_t written_size = pack_length;
    DTERR_C(dtnvblob_write(nvblob_handle, buffer->payload, &written_size));

    if (written_size != pack_length)
    {
        dterr = dterr_new(
          DTERR_IO, DTERR_LOC, NULL, "only wrote %" PRId32 " of %" PRId32 " bytes to nvblob", written_size, pack_length);
        goto cleanup;
    }

cleanup:

    dtbuffer_dispose(buffer);

    return dterr;
}

// -------------------------------------------------------------------------------
dterr_t*
dtradioconfig_read_nvblob(dtradioconfig_t* self, dtnvblob_handle nvblob_handle)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(nvblob_handle);

    // estimate the maximum packed length
    int32_t pack_length = 1024;

    // create a buffer to hold the packed data
    DTERR_C(dtbuffer_create(&buffer, pack_length));

    // read the data from the nvblob
    DTERR_C(dtnvblob_read(nvblob_handle, buffer->payload, &pack_length));

    // unpack the data from the buffer
    DTERR_C(dtradioconfig_unpackx(self, (uint8_t*)buffer->payload, NULL, buffer->length));

cleanup:
    dtbuffer_dispose(buffer);

    return dterr;
}

// --------------------------------------------------------------------------------------------
// dtpackable implementation
// --------------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------------
dterr_t*
dtradioconfig_packx_length(dtradioconfig_t* self DTPACKABLE_PACKX_LENGTH_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(length);
    int32_t len = 0;

    len += dtpackx_pack_string_length(self->signature);
    len += dtpackx_pack_string_length(self->self_node_name);
    len += dtpackx_pack_string_length(self->wifi_ssid);
    len += dtpackx_pack_string_length(self->wifi_password);

    len += dtpackx_pack_string_length(self->mqtt_host);
    len += dtpackx_pack_string_length(self->mqtt_user);
    len += dtpackx_pack_string_length(self->mqtt_password);
    len += dtpackx_pack_int32_length();
    len += dtpackx_pack_int32_length();

    *length = len;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtradioconfig_packx(dtradioconfig_t* self DTPACKABLE_PACKX_ARGS)
{
    dterr_t* dterr = NULL;
    int32_t p = offset ? *offset : 0;

    p += dtpackx_pack_string(self->signature, output, p, length);
    p += dtpackx_pack_string(self->self_node_name, output, p, length);
    p += dtpackx_pack_string(self->wifi_ssid, output, p, length);
    p += dtpackx_pack_string(self->wifi_password, output, p, length);

    p += dtpackx_pack_string(self->mqtt_host, output, p, length);
    p += dtpackx_pack_string(self->mqtt_user, output, p, length);
    p += dtpackx_pack_string(self->mqtt_password, output, p, length);
    p += dtpackx_pack_int32(self->mqtt_port, output, p, length);
    p += dtpackx_pack_int32(self->mqtt_wsport, output, p, length);

    if (offset)
        *offset = p;

    return dterr;
}

// --------------------------------------------------------------------------------------------
// Unpacks the position values from the input buffer
dterr_t*
dtradioconfig_unpackx(dtradioconfig_t* self DTPACKABLE_UNPACKX_ARGS)
{
    dterr_t* dterr = NULL;
    int32_t p = offset ? *offset : 0;

    DTPACKX_UNPACK(dtpackx_unpack_string(input, p, length, &self->signature), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_string(input, p, length, &self->self_node_name), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_string(input, p, length, &self->wifi_ssid), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_string(input, p, length, &self->wifi_password), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_string(input, p, length, &self->mqtt_host), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_string(input, p, length, &self->mqtt_user), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_string(input, p, length, &self->mqtt_password), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_int32(input, p, length, &self->mqtt_port), p, length);
    DTPACKX_UNPACK(dtpackx_unpack_int32(input, p, length, &self->mqtt_wsport), p, length);

    if (offset)
        *offset = p;
cleanup:
    return dterr;
}

// -------------------------------------------------------------------------------
void
dtradioconfig_dispose(dtradioconfig_t* self)
{
    if (self == NULL)
        return;

    dtstr_dispose(self->mqtt_password);
    dtstr_dispose(self->mqtt_user);
    dtstr_dispose(self->mqtt_host);
    dtstr_dispose(self->wifi_password);
    dtstr_dispose(self->wifi_ssid);
    dtstr_dispose(self->self_node_name);
    dtstr_dispose(self->signature);
}

// -------------------------------------------------------------------------------
void
dtradioconfig_to_string(dtradioconfig_t* self, char* buffer, size_t length)
{
    snprintf(buffer,
      length,
      "\"%s\", node: \"%s\", "
      "wifi: \"%s\", mqtt: \"%s\" "
      "%" PRId32 "/%" PRId32 " user \"%s\"",
      self->signature ? self->signature : "NULL",
      self->self_node_name ? self->self_node_name : "NULL",
      self->wifi_ssid ? self->wifi_ssid : "NULL",
      self->mqtt_host ? self->mqtt_host : "NULL",
      self->mqtt_port,
      self->mqtt_wsport,
      self->mqtt_user ? self->mqtt_user : "NULL");
}
