#include "stdint.h"
#include <inttypes.h>

#include <dtcore/dterr.h>

#include <dtcore/dtvtable.h>

#include <dtcore/dtraster.h>

#include <dtmc_base/dtdisplay.h>

#define DTDISPLAY_MAX_VTABLES 8

static int32_t dtdisplay_model_numbers[DTDISPLAY_MAX_VTABLES] = { 0 };
static void* dtdisplay_vtables[DTDISPLAY_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtdisplay_registry = {
    .model_numbers = dtdisplay_model_numbers,
    .vtables = dtdisplay_vtables,
    .max_vtables = DTDISPLAY_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtdisplay_set_vtable(int32_t model_number, dtdisplay_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtdisplay_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtdisplay vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtdisplay_get_vtable(int32_t model_number, dtdisplay_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtdisplay_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtdisplay vtable failed for model number %" PRId32, model_number);

    return dterr;
}

DTVTABLE_DISPATCH(dtdisplay, blit, DTDISPLAY_BLIT_ARGS, DTDISPLAY_BLIT_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtdisplay, attach, DTDISPLAY_ATTACH_ARGS, DTDISPLAY_ATTACH_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtdisplay, detach, DTDISPLAY_DETACH_ARGS, DTDISPLAY_DETACH_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtdisplay,
  create_compatible_raster,
  DTDISPLAY_CREATE_COMPATIBLE_RASTER_ARGS,
  DTDISPLAY_CREATE_COMPATIBLE_RASTER_PARAMS,
  dterr_t*)

// --------------------------------------------------------------------------------------------
void
dtdisplay_dispose(dtdisplay_handle handle)
{
    int32_t model_number = *((int32_t*)handle);
    dtdisplay_vt_t* vtable;
    if (dtdisplay_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}
