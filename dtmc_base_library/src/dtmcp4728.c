#include <inttypes.h>

#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>

#include <dtmc_base/dtmcp4728.h>

#ifndef DTMCP4728_MAX_VTABLES
#define DTMCP4728_MAX_VTABLES 4
#endif

static int32_t dtmcp4728_model_numbers[DTMCP4728_MAX_VTABLES] = { 0 };
static void* dtmcp4728_vtables[DTMCP4728_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtmcp4728_registry = {
    .model_numbers = dtmcp4728_model_numbers,
    .vtables = dtmcp4728_vtables,
    .max_vtables = DTMCP4728_MAX_VTABLES,
};

// ------------------------------------------------------------------------------
dterr_t*
dtmcp4728_set_vtable(int32_t model_number, dtmcp4728_vt_t* vt)
{
    dterr_t* dterr = dtvtable_set(&dtmcp4728_registry, model_number, (void*)vt);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtmcp4728 vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------
dterr_t*
dtmcp4728_get_vtable(int32_t model_number, dtmcp4728_vt_t** out)
{
    dterr_t* dterr = dtvtable_get(&dtmcp4728_registry, model_number, (void**)out);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtmcp4728 vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------

DTVTABLE_DISPATCH(dtmcp4728, attach, DTMCP4728_ATTACH_ARGS, DTMCP4728_ATTACH_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, detach, DTMCP4728_DETACH_ARGS, DTMCP4728_DETACH_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, fast_write, DTMCP4728_FAST_WRITE_ARGS, DTMCP4728_FAST_WRITE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, multi_write, DTMCP4728_MULTI_WRITE_ARGS, DTMCP4728_MULTI_WRITE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, sequential_write, DTMCP4728_SEQUENTIAL_WRITE_ARGS, DTMCP4728_SEQUENTIAL_WRITE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, single_write_eeprom, DTMCP4728_SINGLE_WRITE_EEPROM_ARGS, DTMCP4728_SINGLE_WRITE_EEPROM_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, general_call_reset, DTMCP4728_GENERAL_CALL_RESET_ARGS, DTMCP4728_GENERAL_CALL_RESET_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, general_call_wakeup, DTMCP4728_GENERAL_CALL_WAKEUP_ARGS, DTMCP4728_GENERAL_CALL_WAKEUP_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, general_call_software_update, DTMCP4728_GENERAL_CALL_SOFTWARE_UPDATE_ARGS, DTMCP4728_GENERAL_CALL_SOFTWARE_UPDATE_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, read_all, DTMCP4728_READ_ALL_ARGS, DTMCP4728_READ_ALL_PARAMS, dterr_t*)
DTVTABLE_DISPATCH(dtmcp4728, to_string, DTMCP4728_TO_STRING_ARGS, DTMCP4728_TO_STRING_PARAMS, dterr_t*)

void
dtmcp4728_dispose(dtmcp4728_handle h)
{
    if (!h)
        return;
    int32_t model = *((int32_t*)h);
    dtmcp4728_vt_t* vt = NULL;
    if (!dtmcp4728_get_vtable(model, &vt))
        vt->dispose((void*)h);
}
