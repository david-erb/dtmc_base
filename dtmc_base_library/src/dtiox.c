#include <inttypes.h>

#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtiox.h>

#ifndef DTIOX_MAX_VTABLES
#define DTIOX_MAX_VTABLES 4
#endif

static int32_t dtiox_model_numbers[DTIOX_MAX_VTABLES] = { 0 };
static void* dtiox_vtables[DTIOX_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtiox_registry = {
    .model_numbers = dtiox_model_numbers,
    .vtables = dtiox_vtables,
    .max_vtables = DTIOX_MAX_VTABLES,
};

// ------------------------------------------------------------------------------
dterr_t*
dtiox_set_vtable(int32_t model_number, dtiox_vt_t* vt)
{
    dterr_t* dterr = dtvtable_set(&dtiox_registry, model_number, (void*)vt);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtiox vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------
dterr_t*
dtiox_get_vtable(int32_t model_number, dtiox_vt_t** out)
{
    dterr_t* dterr = dtvtable_get(&dtiox_registry, model_number, (void**)out);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtiox vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------

DTVTABLE_DISPATCH(dtiox, attach, DTIOX_ATTACH_ARGS, DTIOX_ATTACH_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtiox, detach, DTIOX_DETACH_ARGS, DTIOX_DETACH_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtiox, enable, DTIOX_ENABLE_ARGS, DTIOX_ENABLE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtiox, read, DTIOX_READ_ARGS, DTIOX_READ_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtiox, write, DTIOX_WRITE_ARGS, DTIOX_WRITE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtiox, set_rx_semaphore, DTIOX_SET_RX_SEMAPHORE_ARGS, DTIOX_SET_RX_SEMAPHORE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtiox, concat_format, DTIOX_CONCAT_FORMAT_ARGS, DTIOX_CONCAT_FORMAT_PARAMS, dterr_t*)

void
dtiox_dispose(dtiox_handle h)
{
    if (!h)
        return;
    int32_t model = *((int32_t*)h);
    dtiox_vt_t* vt = NULL;
    if (!dtiox_get_vtable(model, &vt))
        vt->dispose((void*)h);
}
