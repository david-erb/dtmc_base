#include <stdlib.h>
#include <string.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtdotstar.h>
#include <dtmc_base/dtdotstar_dummy.h>

DTDOTSTAR_INIT_VTABLE(dtdotstar_dummy);

// --------------------------------------------------------------------------------------
extern dterr_t*
dtdotstar_dummy_create(dtdotstar_dummy_t** self_ptr)
{
    dterr_t* dterr = NULL;

    *self_ptr = (dtdotstar_dummy_t*)malloc(sizeof(dtdotstar_dummy_t));
    if (*self_ptr == NULL)
    {
        dterr = dterr_new(
          DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for dtdotstar_dummy_t", sizeof(dtdotstar_dummy_t));
        goto cleanup;
    }

    DTERR_C(dtdotstar_dummy_init(*self_ptr));

    (*self_ptr)->_is_malloced = true;

cleanup:

    if (dterr != NULL)
    {
        if (*self_ptr != NULL)
        {
            free(*self_ptr);
        }

        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "dtdotstar_dummy_create failed");
    }
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dummy_init(dtdotstar_dummy_t* self)
{
    dterr_t* dterr = NULL;

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_DOTSTAR_MODEL_DUMMY;

    // set the vtable for this model number
    DTERR_C(dtdotstar_set_vtable(self->model_number, &dtdotstar_dummy_vt));

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "dtdotstar_dummy_init failed");
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dummy_configure(dtdotstar_dummy_t* self, dtdotstar_dummy_config_t* config)
{
    self->config = *config;
    return NULL; // success
}

// --------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dummy_set_post_cb(dtdotstar_dummy_t* self, dtdotstar_post_cb_fn post_cb, void* user_context)
{
    self->_post_cb = post_cb;
    self->_post_cb_user_context = user_context;
    return NULL;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dummy_connect(dtdotstar_dummy_t* self)
{
    dterr_t* dterr = NULL;
    self->_connect_call_count++;
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dummy_dither(dtdotstar_dummy_t* self DTDOTSTAR_DITHER_ARGS)
{
    dterr_t* dterr = NULL;
    self->_dither_call_count++;
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dummy_transmit(dtdotstar_dummy_t* self DTDOTSTAR_TRANSMIT_ARGS)
{
    dterr_t* dterr = NULL;
    self->_transmit_call_count++;
    return dterr;
}

// --------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dummy_enqueue(dtdotstar_dummy_t* self DTDOTSTAR_ENQUEUE_ARGS)
{
    dterr_t* dterr = NULL;
    self->_enqueue_call_count++;

    // we call the post callback as if we are finished with the SPI transaction
    if (self->_post_cb != NULL)
    {
        self->_post_cb(self->_post_cb_user_context);
    }

    return dterr;
}

// --------------------------------------------------------------------------------------
void
dtdotstar_dummy_dispose(dtdotstar_dummy_t* self)
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
