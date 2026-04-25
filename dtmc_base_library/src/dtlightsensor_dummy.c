#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>

#include <dtmc_base/dtlightsensor.h>
#include <dtmc_base/dtlightsensor_dummy.h>

DTLIGHTSENSOR_INIT_VTABLE(dtlightsensor_dummy);

// --------------------------------------------------------------------------------------
extern dterr_t*
dtlightsensor_dummy_create(dtlightsensor_dummy_t** self_ptr)
{
    dterr_t* dterr = NULL;

    *self_ptr = (dtlightsensor_dummy_t*)malloc(sizeof(dtlightsensor_dummy_t));
    if (*self_ptr == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM,
          DTERR_LOC,
          NULL,
          "failed to allocate %zu bytes for dtlightsensor_dummy_t",
          sizeof(dtlightsensor_dummy_t));
        goto cleanup;
    }

    DTERR_C(dtlightsensor_dummy_init(*self_ptr));

    (*self_ptr)->_is_malloced = true;

    (*self_ptr)->_create_call_count = 1;

cleanup:

    if (dterr != NULL)
    {
        if (*self_ptr != NULL)
        {
            free(*self_ptr);
        }

        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "dtlightsensor_dummy_create failed");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_dummy_init(dtlightsensor_dummy_t* self)
{
    dterr_t* dterr = NULL;

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_SENSOR_MODEL_DUMMY;

    // set the vtable for this model number
    DTERR_C(dtlightsensor_set_vtable(self->model_number, &_sensor_vt));

    self->_init_call_count++;

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "dtlightsensor_dummy_init failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_dummy_configure(dtlightsensor_dummy_t* self, dtlightsensor_dummy_config_t* config)
{
    self->config = *config;

    self->_configure_call_count++;
    return NULL;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_dummy_activate(dtlightsensor_dummy_t* self DTLIGHTSENSOR_ACTIVATE_ARGS)
{
    dterr_t* dterr = NULL;
    self->_activate_call_count++;

    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_dummy_sample(dtlightsensor_dummy_t* self DTLIGHTSENSOR_SAMPLE_ARGS)
{
    dterr_t* dterr = NULL;
    self->_sample_call_count++;

    *sample = self->_sample_call_count;

    return dterr;
}

// --------------------------------------------------------------------------------------
void
dtlightsensor_dummy_dispose(dtlightsensor_dummy_t* self)
{
    if (self == NULL)
        return;

    if (self->_is_malloced)
    {
        free(self);
    }
    else
    {
        memset(self, 0, sizeof(*self));
    }
}
