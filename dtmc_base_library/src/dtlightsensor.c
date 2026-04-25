#include <inttypes.h>

#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtlightsensor.h>

#define DT_SENSOR_MAX_VTABLES 16

static int32_t dtlightsensor_model_numbers[DT_SENSOR_MAX_VTABLES] = { 0 };
static void* dtlightsensor_vtables[DT_SENSOR_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtlightsensor_registry = {
    .model_numbers = dtlightsensor_model_numbers,
    .vtables = dtlightsensor_vtables,
    .max_vtables = DT_SENSOR_MAX_VTABLES,
};

// --------------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_set_vtable(int32_t model_number, dtlightsensor_vt_t* vtable)
{
    dterr_t* dterr = dtvtable_set(&dtlightsensor_registry, model_number, (void*)vtable);
    if (dterr != NULL)
        dterr = dterr_new(
          DTERR_FAIL, DTERR_LOC, dterr, "setting dtlightsensor vtable failed for model number %" PRId32, model_number);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_get_vtable(int32_t model_number, dtlightsensor_vt_t** vtable)
{
    dterr_t* dterr = dtvtable_get(&dtlightsensor_registry, model_number, (void**)vtable);
    if (dterr != NULL)
        dterr = dterr_new(
          DTERR_FAIL, DTERR_LOC, dterr, "getting dtlightsensor vtable failed for model number %" PRId32, model_number);
    return dterr;
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_activate(dtlightsensor_handle handle DTLIGHTSENSOR_ACTIVATE_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtlightsensor_vt_t* vtable;
    dterr_t* dterr = dtlightsensor_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->activate((void*)handle DTLIGHTSENSOR_ACTIVATE_PARAMS);
}

// --------------------------------------------------------------------------------------------
dterr_t*
dtlightsensor_sample(dtlightsensor_handle handle DTLIGHTSENSOR_SAMPLE_ARGS)
{
    int32_t model_number = *((int32_t*)handle);
    dtlightsensor_vt_t* vtable;
    dterr_t* dterr = dtlightsensor_get_vtable(model_number, &vtable);
    if (dterr != NULL)
        return dterr;
    return vtable->sample((void*)handle DTLIGHTSENSOR_SAMPLE_PARAMS);
}

// --------------------------------------------------------------------------------------------
void
dtlightsensor_dispose(dtlightsensor_handle handle)
{
    if (handle == NULL)
        return;
    int32_t model_number = *((int32_t*)handle);
    dtlightsensor_vt_t* vtable;
    if (dtlightsensor_get_vtable(model_number, &vtable) == NULL)
        vtable->dispose((void*)handle);
}
