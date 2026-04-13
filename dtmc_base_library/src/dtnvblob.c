// dtnvblob.c — facade dispatch + vtable registry

#include <inttypes.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtnvblob.h>

#define DTNVBLOB_MAX_VTABLES 4

static int32_t dtnvblob_model_numbers[DTNVBLOB_MAX_VTABLES] = { 0 };
static void* dtnvblob_vtables[DTNVBLOB_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtnvblob_registry = {
    .model_numbers = dtnvblob_model_numbers,
    .vtables = dtnvblob_vtables,
    .max_vtables = DTNVBLOB_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtnvblob_set_vtable(int32_t model_number, dtnvblob_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtnvblob_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtnvblob vtable failed for model number %" PRId32, model_number);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnvblob_get_vtable(int32_t model_number, dtnvblob_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtnvblob_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtnvblob vtable failed for model number %" PRId32, model_number);
    return dterr;
}

DTVTABLE_DISPATCH(dtnvblob, read, DTNVBLOB_READ_ARGS, DTNVBLOB_READ_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtnvblob, write, DTNVBLOB_WRITE_ARGS, DTNVBLOB_WRITE_PARAMS, dterr_t*)

// --------------------------------------------------------------------------------------------
void
dtnvblob_dispose(dtnvblob_handle handle)
{
    // Same disposal rules used elsewhere: safe on NULL; ignore if model_number==0.
    if (handle == NULL)
        return;

    int32_t model_number = *((int32_t*)handle);

    // If model_number is 0, not bound to a real backend; don't dispose.
    if (model_number == 0)
        return;

    dtnvblob_vt_t* vtable;
    if (dtnvblob_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}
