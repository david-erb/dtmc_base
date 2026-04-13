#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtkvp.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtparse.h>
#include <dtcore/dtstr.h>

#include <dtcore/dtvtable.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtmc_base/dttimeseries.h>
#include <dtmc_base/dttimeseries_steady.h>

#define TAG "dttimeseries_steady"

typedef struct dttimeseries_steady_t
{
    int32_t model_number;

    double value;
} dttimeseries_steady_t;

DTTIMESERIES_DECLARE_API(dttimeseries_steady);
DTTIMESERIES_INIT_VTABLE(dttimeseries_steady);

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_steady_create(dttimeseries_steady_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    DTERR_C(dttimeseries_set_vtable(DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_STEADY, &dttimeseries_steady_vt));

    DTERR_C(dtheaper_alloc_and_zero((int32_t)sizeof(dttimeseries_steady_t), "dttimeseries_steady_t", (void**)self_ptr));

    dttimeseries_steady_t* self = (dttimeseries_steady_t*)*self_ptr;

    self->model_number = DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_STEADY;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_steady_configure(dttimeseries_steady_t* self DTTIMESERIES_CONFIGURE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(kvp_list);

    const char* value_string = NULL;
    DTERR_C(dtkvp_list_get(kvp_list, "value", &value_string));
    if (value_string == NULL)
        goto cleanup;

    double value_double = 0.0;
    bool parse_result = dtparse_double(value_string, &value_double);
    if (!parse_result)
        goto cleanup;

    self->value = value_double;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_steady_read(dttimeseries_steady_t* self DTTIMESERIES_READ_ARGS)
{
    dterr_t* dterr = NULL;

    *value = self->value;

    goto cleanup;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_steady_to_string(dttimeseries_steady_t* self DTTIMESERIES_TO_STRING_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_string);
    snprintf(out_string, out_string_capacity, "steady %f", self->value);

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern void
dttimeseries_steady_dispose(dttimeseries_steady_t* self)
{
    if (self != NULL)
    {
        dtheaper_free(self);
    }
}
