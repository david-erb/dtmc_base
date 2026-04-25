#include "stdint.h"
#include <inttypes.h>

#include <dtcore/dterr.h>

#include <dtcore/dtvtable.h>

#include <dtmc_base/dtadc.h>
#include <dtmc_base/dtadc_scan.h>

#define DTADC_MAX_VTABLES 8

static int32_t dtadc_model_numbers[DTADC_MAX_VTABLES] = { 0 };
static void* dtadc_vtables[DTADC_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtadc_registry = {
    .model_numbers = dtadc_model_numbers,
    .vtables = dtadc_vtables,
    .max_vtables = DTADC_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtadc_set_vtable(int32_t model_number, dtadc_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtadc_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtadc vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtadc_get_vtable(int32_t model_number, dtadc_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtadc_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtadc vtable failed for model number %" PRId32, model_number);

    return dterr;
}

DTVTABLE_DISPATCH(dtadc, activate, DTADC_ACTIVATE_ARGS, DTADC_ACTIVATE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtadc, deactivate, DTADC_DEACTIVATE_ARGS, DTADC_DEACTIVATE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtadc, get_status, DTADC_GET_STATUS_ARGS, DTADC_GET_STATUS_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtadc, to_string, DTADC_TO_STRING_ARGS, DTADC_TO_STRING_PARAMS, dterr_t*)

// --------------------------------------------------------------------------------------------
void
dtadc_dispose(dtadc_handle handle)
{
    if (handle == NULL)
        return;
    int32_t model_number = *((int32_t*)handle);
    dtadc_vt_t* vtable;
    if (dtadc_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}
