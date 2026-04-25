#include <inttypes.h>

#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtdotstar.h>

#define DT_DOTSTAR_MAX_VTABLES 16

static int32_t dtdotstar_model_numbers[DT_DOTSTAR_MAX_VTABLES] = { 0 };
static void* dtdotstar_vtables[DT_DOTSTAR_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtdotstar_registry = {
    .model_numbers = dtdotstar_model_numbers,
    .vtables = dtdotstar_vtables,
    .max_vtables = DT_DOTSTAR_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtdotstar_set_vtable(int32_t model_number, dtdotstar_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtdotstar_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtdotstar vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtdotstar_get_vtable(int32_t model_number, dtdotstar_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtdotstar_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtdotstar vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtdotstar_set_post_cb(dtdotstar_handle handle DTDOTSTAR_SET_POST_CB_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtdotstar_vt_t* vtable;
    dterr_t* dterr = dtdotstar_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->set_post_cb((void*)handle DTDOTSTAR_SET_POST_CB_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtdotstar_connect(dtdotstar_handle handle DTDOTSTAR_CONNECT_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtdotstar_vt_t* vtable;
    dterr_t* dterr = dtdotstar_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->connect((void*)handle DTDOTSTAR_CONNECT_ARGS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtdotstar_dither(dtdotstar_handle handle DTDOTSTAR_DITHER_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtdotstar_vt_t* vtable;
    dterr_t* dterr = dtdotstar_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->dither((void*)handle DTDOTSTAR_DITHER_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtdotstar_transmit(dtdotstar_handle handle DTDOTSTAR_TRANSMIT_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtdotstar_vt_t* vtable;
    dterr_t* dterr = dtdotstar_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->transmit((void*)handle DTDOTSTAR_TRANSMIT_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtdotstar_enqueue(dtdotstar_handle handle DTDOTSTAR_ENQUEUE_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtdotstar_vt_t* vtable;
    dterr_t* dterr = dtdotstar_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->enqueue((void*)handle DTDOTSTAR_ENQUEUE_PARAMS);
}

// --------------------------------------------------------------------------------------------

void
dtdotstar_dispose(dtdotstar_handle handle)
{
    // TODO: Add logic to check handle and model number in dispose to all classes.
    if (handle == NULL)
        return;

    int32_t model_number = *((int32_t*)handle);

    // if model_number is 0, we are not connected to a real device, so don't dispose of it
    if (model_number == 0)
        return;

    dtdotstar_vt_t* vtable;
    if (dtdotstar_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}