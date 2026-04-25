// dtmcp4728_dummy.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtmc_base_constants.h>
#include <dtmc_base/dtmcp4728.h>
#include <dtmc_base/dtmcp4728_dummy.h>

DTMCP4728_INIT_VTABLE(dtmcp4728_dummy);

// --------------------------------------------------------------------------------------------
// Internal helpers

static dterr_t*
dtmcp4728_dummy__validate_attached(dtmcp4728_dummy_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    if (!self->is_configured)
    {
        dterr = dterr_new(DTERR_BADCONFIG, DTERR_LOC, NULL, "dtmcp4728_dummy is not configured");
        goto cleanup;
    }

    if (!self->is_attached)
    {
        dterr = dterr_new(DTERR_BADCONFIG, DTERR_LOC, NULL, "dtmcp4728_dummy is not attached");
        goto cleanup;
    }

cleanup:
    return dterr;
}

static dterr_t*
dtmcp4728_dummy__validate_channel(dtmcp4728_channel_t channel)
{
    dterr_t* dterr = NULL;

    if ((int32_t)channel < (int32_t)DTMCP4728_CHANNEL_A || (int32_t)channel > (int32_t)DTMCP4728_CHANNEL_D)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel must be between A and D");
        goto cleanup;
    }

cleanup:
    return dterr;
}

static dterr_t*
dtmcp4728_dummy__validate_channel_config(const dtmcp4728_channel_config_t* channel_config)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(channel_config);

    DTERR_C(dtmcp4728_dummy__validate_channel(channel_config->channel));

    if (channel_config->value_12bit > 0x0FFF)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "value_12bit must be <= 0x0FFF");
        goto cleanup;
    }

    if ((int32_t)channel_config->vref < 0 || (int32_t)channel_config->vref > 1)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "vref is out of range");
        goto cleanup;
    }

    if ((int32_t)channel_config->power_down < 0 || (int32_t)channel_config->power_down > 3)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "power_down is out of range");
        goto cleanup;
    }

    if ((int32_t)channel_config->gain < 0 || (int32_t)channel_config->gain > 1)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "gain is out of range");
        goto cleanup;
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Lifecycle

dterr_t*
dtmcp4728_dummy_create(dtmcp4728_dummy_t** self_ptr)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self_ptr);

    *self_ptr = (dtmcp4728_dummy_t*)malloc(sizeof(dtmcp4728_dummy_t));
    if (*self_ptr == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for dtmcp4728_dummy_t", sizeof(dtmcp4728_dummy_t));
        goto cleanup;
    }

    DTERR_C(dtmcp4728_dummy_init(*self_ptr));

    (*self_ptr)->_is_malloced = true;

cleanup:
    if (dterr)
    {
        if (*self_ptr)
        {
            free(*self_ptr);
            *self_ptr = NULL;
        }
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "dtmcp4728_dummy_create failed");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_init(dtmcp4728_dummy_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_MCP4728_MODEL_DUMMY;

    DTERR_C(dtmcp4728_set_vtable(self->model_number, &dtmcp4728_dummy_vt));

cleanup:
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "dtmcp4728_dummy_init failed");
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_configure(dtmcp4728_dummy_t* self, const dtmcp4728_dummy_config_t* config)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    if (self->is_attached)
    {
        dterr = dterr_new(DTERR_BADCONFIG, DTERR_LOC, NULL, "cannot configure while attached");
        goto cleanup;
    }

    self->config = *config;
    self->is_configured = true;

cleanup:
    if (dterr)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "dtmcp4728_dummy_configure failed");
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Facade entry points

dterr_t*
dtmcp4728_dummy_attach(dtmcp4728_dummy_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    self->attach_call_count++;

    if (self->inject_attach_error)
    {
        dterr = self->inject_attach_error;
        self->inject_attach_error = NULL;
        goto cleanup;
    }

    if (!self->is_configured)
    {
        dterr = dterr_new(DTERR_BADCONFIG, DTERR_LOC, NULL, "dtmcp4728_dummy must be configured before attach");
        goto cleanup;
    }

    if (self->is_attached)
    {
        dterr = dterr_new(DTERR_BADCONFIG, DTERR_LOC, NULL, "dtmcp4728_dummy already attached");
        goto cleanup;
    }

    self->is_attached = true;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_detach(dtmcp4728_dummy_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    self->detach_call_count++;

    if (!self->is_attached)
        goto cleanup;

    self->is_attached = false;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

void
dtmcp4728_dummy_dispose(dtmcp4728_dummy_t* self)
{
    if (!self)
        return;

    self->dispose_call_count++;

    if (self->is_attached)
        dtmcp4728_dummy_detach(self);

    if (self->_is_malloced)
    {
        free(self);
    }
    else
    {
        int32_t dispose_count = self->dispose_call_count;
        memset(self, 0, sizeof(*self));
        self->dispose_call_count = dispose_count;
    }
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_fast_write(dtmcp4728_dummy_t* self, const dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT])
{
    dterr_t* dterr = NULL;
    int32_t i;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(channels);

    self->fast_write_call_count++;

    if (self->inject_fast_write_error)
    {
        dterr = self->inject_fast_write_error;
        self->inject_fast_write_error = NULL;
        goto cleanup;
    }

    DTERR_C(dtmcp4728_dummy__validate_attached(self));

    for (i = 0; i < DTMCP4728_CHANNEL_COUNT; i++)
        DTERR_C(dtmcp4728_dummy__validate_channel_config(&channels[i]));

    memcpy(self->last_fast_write, channels, sizeof(self->last_fast_write));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_multi_write(dtmcp4728_dummy_t* self, const dtmcp4728_channel_config_t* channel_config)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(channel_config);

    self->multi_write_call_count++;

    if (self->inject_multi_write_error)
    {
        dterr = self->inject_multi_write_error;
        self->inject_multi_write_error = NULL;
        goto cleanup;
    }

    DTERR_C(dtmcp4728_dummy__validate_attached(self));
    DTERR_C(dtmcp4728_dummy__validate_channel_config(channel_config));

    self->last_multi_write = *channel_config;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_sequential_write(dtmcp4728_dummy_t* self,
  dtmcp4728_channel_t start_channel,
  const dtmcp4728_channel_config_t* channel_configs,
  int32_t channel_count)
{
    dterr_t* dterr = NULL;
    int32_t i;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(channel_configs);

    self->sequential_write_call_count++;

    if (self->inject_sequential_write_error)
    {
        dterr = self->inject_sequential_write_error;
        self->inject_sequential_write_error = NULL;
        goto cleanup;
    }

    DTERR_C(dtmcp4728_dummy__validate_attached(self));
    DTERR_C(dtmcp4728_dummy__validate_channel(start_channel));

    if (channel_count <= 0 || channel_count > DTMCP4728_CHANNEL_COUNT)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "channel_count must be between 1 and %d", DTMCP4728_CHANNEL_COUNT);
        goto cleanup;
    }

    if (((int32_t)start_channel + channel_count) > DTMCP4728_CHANNEL_COUNT)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "start_channel + channel_count exceeds channel D");
        goto cleanup;
    }

    for (i = 0; i < channel_count; i++)
    {
        DTERR_C(dtmcp4728_dummy__validate_channel_config(&channel_configs[i]));

        if ((int32_t)channel_configs[i].channel != ((int32_t)start_channel + i))
        {
            dterr = dterr_new(DTERR_BADARG,
              DTERR_LOC,
              NULL,
              "channel_configs[%d].channel must match sequential channel order starting from start_channel",
              i);
            goto cleanup;
        }
    }

    memcpy(self->last_sequential_write, channel_configs, (size_t)channel_count * sizeof(dtmcp4728_channel_config_t));
    self->last_sequential_write_count = channel_count;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_single_write_eeprom(dtmcp4728_dummy_t* self, const dtmcp4728_channel_config_t* channel_config)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(channel_config);

    self->single_write_eeprom_call_count++;

    if (self->inject_single_write_eeprom_error)
    {
        dterr = self->inject_single_write_eeprom_error;
        self->inject_single_write_eeprom_error = NULL;
        goto cleanup;
    }

    DTERR_C(dtmcp4728_dummy__validate_attached(self));
    DTERR_C(dtmcp4728_dummy__validate_channel_config(channel_config));

    self->last_single_write_eeprom = *channel_config;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_general_call_reset(dtmcp4728_dummy_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    self->general_call_reset_call_count++;

    DTERR_C(dtmcp4728_dummy__validate_attached(self));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_general_call_wakeup(dtmcp4728_dummy_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    self->general_call_wakeup_call_count++;

    DTERR_C(dtmcp4728_dummy__validate_attached(self));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_general_call_software_update(dtmcp4728_dummy_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    self->general_call_software_update_call_count++;

    DTERR_C(dtmcp4728_dummy__validate_attached(self));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_read_all(dtmcp4728_dummy_t* self, dtmcp4728_channel_config_t channels[DTMCP4728_CHANNEL_COUNT])
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(channels);

    self->read_all_call_count++;

    if (self->inject_read_all_error)
    {
        dterr = self->inject_read_all_error;
        self->inject_read_all_error = NULL;
        goto cleanup;
    }

    DTERR_C(dtmcp4728_dummy__validate_attached(self));

    memcpy(channels, self->last_fast_write, sizeof(self->last_fast_write));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------

dterr_t*
dtmcp4728_dummy_to_string(dtmcp4728_dummy_t* self, char* buffer, int32_t buffer_size)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(buffer);

    if (buffer_size <= 0)
    {
        dterr = dterr_new(DTERR_BADARG, DTERR_LOC, NULL, "buffer_size must be > 0");
        goto cleanup;
    }

    snprintf(buffer,
      (size_t)buffer_size,
      "dtmcp4728_dummy addr=0x%02x configured=%d attached=%d",
      (unsigned int)self->config.device_address_7bit,
      self->is_configured ? 1 : 0,
      self->is_attached ? 1 : 0);

    buffer[buffer_size - 1] = '\0';

cleanup:
    return dterr;
}
