// dtgpiopin.c — facade dispatch + vtable registry

#include <inttypes.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtgpiopin.h>

#define DTGPIOPIN_MAX_VTABLES 4

static int32_t dtgpiopin_model_numbers[DTGPIOPIN_MAX_VTABLES] = { 0 };
static void* dtgpiopin_vtables[DTGPIOPIN_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtgpiopin_registry = {
    .model_numbers = dtgpiopin_model_numbers,
    .vtables = dtgpiopin_vtables,
    .max_vtables = DTGPIOPIN_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtgpiopin_set_vtable(int32_t model_number, dtgpiopin_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtgpiopin_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtgpiopin vtable failed for model number %" PRId32, model_number);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtgpiopin_get_vtable(int32_t model_number, dtgpiopin_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtgpiopin_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtgpiopin vtable failed for model number %" PRId32, model_number);
    return dterr;
}

// --------------------------------------------------------------------------------------------
// Thin dispatchers — these read the model_number from the instance (first field)
// and forward to the registered backend vtable.

dterr_t*
dtgpiopin_attach(dtgpiopin_handle handle DTGPIOPIN_ATTACH_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtgpiopin_vt_t* vtable;
    dterr_t* dterr = dtgpiopin_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->attach((void*)handle DTGPIOPIN_ATTACH_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtgpiopin_enable(dtgpiopin_handle handle DTGPIOPIN_ENABLE_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtgpiopin_vt_t* vtable;
    dterr_t* dterr = dtgpiopin_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->enable((void*)handle DTGPIOPIN_ENABLE_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtgpiopin_read(dtgpiopin_handle handle DTGPIOPIN_READ_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtgpiopin_vt_t* vtable;
    dterr_t* dterr = dtgpiopin_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->read((void*)handle DTGPIOPIN_READ_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtgpiopin_write(dtgpiopin_handle handle DTGPIOPIN_WRITE_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtgpiopin_vt_t* vtable;
    dterr_t* dterr = dtgpiopin_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->write((void*)handle DTGPIOPIN_WRITE_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtgpiopin_concat_format(dtgpiopin_handle handle DTGPIOPIN_CONCAT_FORMAT_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtgpiopin_vt_t* vtable;
    dterr_t* dterr = dtgpiopin_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->concat_format((void*)handle DTGPIOPIN_CONCAT_FORMAT_PARAMS);
}
// --------------------------------------------------------------------------------------------
void
dtgpiopin_dispose(dtgpiopin_handle handle)
{
    // Same disposal rules used elsewhere: safe on NULL; ignore if model_number==0.
    if (handle == NULL)
        return;

    int32_t model_number = *((int32_t*)handle);

    // If model_number is 0, not bound to a real backend; don't dispose.
    if (model_number == 0)
        return;

    dtgpiopin_vt_t* vtable;
    if (dtgpiopin_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}
