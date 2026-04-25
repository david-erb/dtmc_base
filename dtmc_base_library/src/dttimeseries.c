#include "stdint.h"
#include <inttypes.h>

#include <dtcore/dterr.h>

#include <dtcore/dtvtable.h>

#include <dtmc_base/dttimeseries.h>

#define DTTIMESERIES_MAX_VTABLES 16

static int32_t dttimeseries_model_numbers[DTTIMESERIES_MAX_VTABLES] = { 0 };
static void* dttimeseries_vtables[DTTIMESERIES_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dttimeseries_registry = {
    .model_numbers = dttimeseries_model_numbers,
    .vtables = dttimeseries_vtables,
    .max_vtables = DTTIMESERIES_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dttimeseries_set_vtable(int32_t model_number, dttimeseries_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dttimeseries_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dttimeseries vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dttimeseries_get_vtable(int32_t model_number, dttimeseries_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dttimeseries_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dttimeseries vtable failed for model number %" PRId32, model_number);

    return dterr;
}

DTVTABLE_DISPATCH(dttimeseries, configure, DTTIMESERIES_CONFIGURE_ARGS, DTTIMESERIES_CONFIGURE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dttimeseries, read, DTTIMESERIES_READ_ARGS, DTTIMESERIES_READ_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dttimeseries, to_string, DTTIMESERIES_TO_STRING_ARGS, DTTIMESERIES_TO_STRING_PARAMS, dterr_t*)

// --------------------------------------------------------------------------------------------
void
dttimeseries_dispose(dttimeseries_handle handle)
{
    if (handle == NULL)
        return;
    int32_t model_number = *((int32_t*)handle);
    dttimeseries_vt_t* vtable;
    if (dttimeseries_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}
