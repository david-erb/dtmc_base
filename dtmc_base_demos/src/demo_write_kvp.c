#include <dtcore/dtcore_constants.h>

#include <dtcore/dterr.h>
#include <dtcore/dtguid.h>
#include <dtcore/dtkvp.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtparse.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtnvblob.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base_demos/demo_write_kvp.h>

#define TAG "dtmc_base_demo_write_kvp"

// the demo's privates
typedef struct dtmc_base_demo_write_kvp_t
{
    // demo configuration
    dtmc_base_demo_write_kvp_config_t config;

    // track number of writes
    int32_t write_counter;
} dtmc_base_demo_write_kvp_t;

// --------------------------------------------------------------------------------------
// Create a new instance, allocating memory as needed
dterr_t*
dtmc_base_demo_write_kvp_create(dtmc_base_demo_write_kvp_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    *self = (dtmc_base_demo_write_kvp_t*)malloc(sizeof(dtmc_base_demo_write_kvp_t));
    if (*self == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM,
          DTERR_LOC,
          NULL,
          "failed to allocate %zu bytes for dtmc_base_demo_write_kvp_t",
          sizeof(dtmc_base_demo_write_kvp_t));
        goto cleanup;
    }

    memset(*self, 0, sizeof(dtmc_base_demo_write_kvp_t));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
dtmc_base_demo_write_kvp_configure( //
  dtmc_base_demo_write_kvp_t* self,
  dtmc_base_demo_write_kvp_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);
    DTERR_ASSERT_NOT_NULL(config->kvp_list);
    DTERR_ASSERT_NOT_NULL(config->nvblob_handle);

    self->config = *config;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// read a kvp list from nvblob
dterr_t*
dtmc_base_demo_write_kvp__read_nvblob(dtmc_base_demo_write_kvp_t* self, dtkvp_list_t* kvp_list, dtnvblob_handle nvblob_handle)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // probe the nvblob size
    int32_t nvblob_size = 0;
    DTERR_C(dtnvblob_read(nvblob_handle, NULL, &nvblob_size));

    // buffer to hold the nvblob data
    DTERR_C(dtbuffer_create(&buffer, nvblob_size));

    // read the nvblob data into the buffer
    int32_t read_size = nvblob_size;
    DTERR_C(dtnvblob_read(nvblob_handle, buffer->payload, &read_size));

    if (read_size != nvblob_size)
    {
        dterr = dterr_new(
          DTERR_IO, DTERR_LOC, NULL, "only read %" PRId32 " of %" PRId32 " bytes from nvblob", read_size, nvblob_size);
        goto cleanup;
    }

    // unpack the buffer into the kvp list
    DTERR_C(dtkvp_list_unpackx(kvp_list, (const uint8_t*)buffer->payload, NULL, buffer->length));

    {
        const char* s;
        DTERR_C(dtkvp_list_get(kvp_list, DTCORE_CONSTANTS_KVP_KEY_WRITE_COUNTER, &s));
        dtparse_int32(s, &self->write_counter);
    }

cleanup:

    return dterr;
}

// -------------------------------------------------------------------------------
dterr_t*
dtmc_base_demo_write_kvp__write_nvblob(dtmc_base_demo_write_kvp_t* self, dtkvp_list_t* kvp_list, dtnvblob_handle nvblob_handle)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* buffer = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(nvblob_handle);

    // make up a new guid for this write
    {
        int32_t random = dtcpu_random_int32();
        dtguid_t guid;
        dtguid_generate_from_int32(&guid, random);
        char guid_str[DTGUID_STRING_SIZE] = { 0 };
        dtguid_to_string(&guid, guid_str, sizeof(guid_str));
        DTERR_C(dtkvp_list_set(kvp_list, DTCORE_CONSTANTS_KVP_KEY_WRITE_GUID, guid_str));
    }

    // bump the write counter
    {
        self->write_counter++;
        char s[16] = { 0 };
        snprintf(s, sizeof(s), "%" PRId32, self->write_counter);
        DTERR_C(dtkvp_list_set(kvp_list, DTCORE_CONSTANTS_KVP_KEY_WRITE_COUNTER, s));
    }

    // compute the packed length
    int32_t pack_length;
    DTERR_C(dtkvp_list_packx_length(kvp_list, &pack_length));

    // create a buffer to hold the packed data

    DTERR_C(dtbuffer_create(&buffer, pack_length));

    // pack the data into the buffer
    DTERR_C(dtkvp_list_packx(kvp_list, (uint8_t*)buffer->payload, NULL, buffer->length));

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
// --------------------------------------------------------------------------------------
// Run the demo logic forever
dterr_t*
dtmc_base_demo_write_kvp_start(dtmc_base_demo_write_kvp_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    const char* separator = "\n    ";

    const char* cpu_id = dtcpu_identify();
    dtlog_info(TAG, "dtcpu_identify: %s", cpu_id ? cpu_id : "(null)");

    {
        dtkvp_list_t old_kvp_list = { 0 };
        DTERR_C(dtkvp_list_init(&old_kvp_list));
        dterr = dtmc_base_demo_write_kvp__read_nvblob(self, &old_kvp_list, self->config.nvblob_handle);
        if (dterr == NULL)
        {
            char* s = dtstr_dup("");
            DTERR_C(dtkvp_list_compose_plain_text(&old_kvp_list, &s, separator));
            dtlog_info(TAG, "existed before: %s", strlen(s) > 0 ? s : "(empty)");
            dtstr_dispose(s);
            dtkvp_list_dispose(&old_kvp_list);
        }
        else
        {
            dtlog_error(TAG, "no existing kvp list found (%s)", dterr->message);
            dterr_dispose(dterr);
            dterr = NULL;
        }
    }

    DTERR_C(dtmc_base_demo_write_kvp__write_nvblob(self, self->config.kvp_list, self->config.nvblob_handle));

    {
        char* s = dtstr_dup("");
        DTERR_C(dtkvp_list_compose_plain_text(self->config.kvp_list, &s, separator));
        dtlog_info(TAG, "replaced with: %s", strlen(s) > 0 ? s : "(empty)");
        dtstr_dispose(s);
    }

cleanup:

    return dterr;
}

// --------------------------------------------------------------------------------------
// Stop, unregister and dispose of the demo instance resources
void
dtmc_base_demo_write_kvp_dispose(dtmc_base_demo_write_kvp_t* self)
{
    if (self == NULL)
        return;

    memset(self, 0, sizeof(dtmc_base_demo_write_kvp_t));

    free(self);
}