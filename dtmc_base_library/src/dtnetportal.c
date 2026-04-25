#include <inttypes.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtnetportal.h>

#define DTNETPORTAL_MAX_VTABLES 16

static int32_t dtnetportal_model_numbers[DTNETPORTAL_MAX_VTABLES] = { 0 };
static void* dtnetportal_vtables[DTNETPORTAL_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtnetportal_registry = {
    .model_numbers = dtnetportal_model_numbers,
    .vtables = dtnetportal_vtables,
    .max_vtables = DTNETPORTAL_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetportal_set_vtable(int32_t model_number, dtnetportal_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtnetportal_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtnetportal vtable failed for model number %" PRId32, model_number);

    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetportal_get_vtable(int32_t model_number, dtnetportal_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtnetportal_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtnetportal vtable failed for model number %" PRId32, model_number);

    return dterr;
}

DTVTABLE_DISPATCH(dtnetportal, activate, DTNETPORTAL_ACTIVATE_ARGS, DTNETPORTAL_ACTIVATE_PARAMS, dterr_t*)

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetportal_subscribe(dtnetportal_handle handle DTNETPORTAL_SUBSCRIBE_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtnetportal_vt_t* vtable;
    dterr_t* dterr = dtnetportal_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->subscribe((void*)handle DTNETPORTAL_SUBSCRIBE_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetportal_publish(dtnetportal_handle handle DTNETPORTAL_PUBLISH_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtnetportal_vt_t* vtable;
    dterr_t* dterr = dtnetportal_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->publish((void*)handle DTNETPORTAL_PUBLISH_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtnetportal_get_info(dtnetportal_handle handle DTNETPORTAL_GET_INFO_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtnetportal_vt_t* vtable;
    dterr_t* dterr = dtnetportal_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->get_info((void*)handle DTNETPORTAL_GET_INFO_PARAMS);
}

// --------------------------------------------------------------------------------------------

void
dtnetportal_dispose(dtnetportal_handle handle)
{
    // TODO: Add logic to check handle and model number in dispose to all classes.
    if (handle == NULL)
        return;

    int32_t model_number = *((int32_t*)handle);

    // if model_number is 0, we are not activateed to a real device, so don't
    // dispose of it
    if (model_number == 0)
        return;

    dtnetportal_vt_t* vtable;
    if (dtnetportal_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}