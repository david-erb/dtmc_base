#include <inttypes.h>

#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dthttpd.h>

#ifndef DTHTTPD_MAX_VTABLES
#define DTHTTPD_MAX_VTABLES 4
#endif

static int32_t dthttpd_model_numbers[DTHTTPD_MAX_VTABLES] = { 0 };
static void* dthttpd_vtables[DTHTTPD_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dthttpd_registry = {
    .model_numbers = dthttpd_model_numbers,
    .vtables = dthttpd_vtables,
    .max_vtables = DTHTTPD_MAX_VTABLES,
};

// ------------------------------------------------------------------------------
dterr_t*
dthttpd_set_vtable(int32_t model_number, dthttpd_vt_t* vt)
{
    dterr_t* dterr = dtvtable_set(&dthttpd_registry, model_number, (void*)vt);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dthttpd vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------
dterr_t*
dthttpd_get_vtable(int32_t model_number, dthttpd_vt_t** out)
{
    dterr_t* dterr = dtvtable_get(&dthttpd_registry, model_number, (void**)out);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dthttpd vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------

DTVTABLE_DISPATCH(dthttpd, loop, DTHTTPD_LOOP_ARGS, DTHTTPD_LOOP_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dthttpd, stop, DTHTTPD_STOP_ARGS, DTHTTPD_STOP_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dthttpd, join, DTHTTPD_JOIN_ARGS, DTHTTPD_JOIN_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dthttpd, set_callback, DTHTTPD_SET_CALLBACK_ARGS, DTHTTPD_SET_CALLBACK_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dthttpd, concat_format, DTHTTPD_CONCAT_FORMAT_ARGS, DTHTTPD_CONCAT_FORMAT_PARAMS, dterr_t*)

void
dthttpd_dispose(dthttpd_handle h)
{
    if (!h)
        return;
    int32_t model = *((int32_t*)h);
    dthttpd_vt_t* vt = NULL;
    if (!dthttpd_get_vtable(model, &vt))
        vt->dispose((void*)h);
}
