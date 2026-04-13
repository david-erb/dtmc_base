#include <inttypes.h>

#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtinterval.h>

#define DTINTERVAL_MAX_VTABLES 16

static int32_t dtinterval_model_numbers[DTINTERVAL_MAX_VTABLES] = { 0 };
static void* dtinterval_vtables[DTINTERVAL_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtinterval_registry = {
    .model_numbers = dtinterval_model_numbers,
    .vtables = dtinterval_vtables,
    .max_vtables = DTINTERVAL_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtinterval_set_vtable(int32_t model_number, dtinterval_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtinterval_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtinterval vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtinterval_get_vtable(int32_t model_number, dtinterval_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtinterval_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtinterval vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------

DTVTABLE_DISPATCH(dtinterval, set_callback, DTINTERVAL_SET_CALLBACK_ARGS, DTINTERVAL_SET_CALLBACK_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtinterval, start, DTINTERVAL_START_ARGS, DTINTERVAL_START_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtinterval, pause, DTINTERVAL_PAUSE_ARGS, DTINTERVAL_PAUSE_PARAMS, dterr_t*)

// --------------------------------------------------------------------------------------------

void
dtinterval_dispose(dtinterval_handle handle)
{
    // TODO: Add logic to check handle and model number in dispose to all classes.
    if (handle == NULL)
        return;

    int32_t model_number = *((int32_t*)handle);

    // if model_number is 0, we are not connected to a real device, so don't dispose of it
    if (model_number == 0)
        return;

    dtinterval_vt_t* vtable;
    if (dtinterval_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}