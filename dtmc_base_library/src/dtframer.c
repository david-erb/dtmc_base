#include <dtcore/dterr.h>
#include <dtcore/dtvtable.h>
#include <dtmc_base/dtframer.h>
#include <inttypes.h>

#ifndef DTFRAMER_MAX_VTABLES
#define DTFRAMER_MAX_VTABLES 4
#endif

static int32_t dtframer_model_numbers[DTFRAMER_MAX_VTABLES] = { 0 };
static void* dtframer_vtables[DTFRAMER_MAX_VTABLES] = { 0 };

static dtvtable_registry_t dtframer_registry = {
    .model_numbers = dtframer_model_numbers,
    .vtables = dtframer_vtables,
    .max_vtables = DTFRAMER_MAX_VTABLES,
};

// ------------------------------------------------------------------------------
dterr_t*
dtframer_set_vtable(int32_t model_number, dtframer_vt_t* vt)
{
    dterr_t* dterr = dtvtable_set(&dtframer_registry, model_number, (void*)vt);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "setting dtframer vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------
dterr_t*
dtframer_get_vtable(int32_t model_number, dtframer_vt_t** out)
{
    dterr_t* dterr = dtvtable_get(&dtframer_registry, model_number, (void**)out);
    if (dterr)
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "getting dtframer vtable failed for model %" PRId32, model_number);
    return dterr;
}

// ------------------------------------------------------------------------------
// Thin dispatchers — mirrors dtgpiopin facade. :contentReference[oaicite:2]{index=2}
#define DISPATCH(CLASS, NAME, ARGS, PARAMS, RET)                                                                               \
    RET CLASS##_##NAME(CLASS##_handle h ARGS)                                                                                  \
    {                                                                                                                          \
        if (!h)                                                                                                                \
            return dterr_new(DTERR_FAIL, DTERR_LOC, NULL, #CLASS " handle is NULL");                                           \
        int32_t model = *((int32_t*)h);                                                                                        \
        dtframer_vt_t* vt = NULL;                                                                                              \
        dterr_t* dterr = CLASS##_get_vtable(model, &vt);                                                                       \
        if (dterr)                                                                                                             \
            return dterr;                                                                                                      \
        return vt->NAME((void*)h PARAMS);                                                                                      \
    }

DISPATCH(dtframer, set_message_callback, DTFRAMER_SET_MESSAGE_CALLBACK_ARGS, DTFRAMER_SET_MESSAGE_CALLBACK_PARAMS, dterr_t*)
DISPATCH(dtframer, encode, DTFRAMER_ENCODE_ARGS, DTFRAMER_ENCODE_PARAMS, dterr_t*)
DISPATCH(dtframer, decode, DTFRAMER_DECODE_ARGS, DTFRAMER_DECODE_PARAMS, dterr_t*)

void
dtframer_dispose(dtframer_handle h)
{
    if (!h)
        return;
    int32_t model = *((int32_t*)h);
    dtframer_vt_t* vt = NULL;
    if (!dtframer_get_vtable(model, &vt))
        vt->dispose((void*)h);
}
