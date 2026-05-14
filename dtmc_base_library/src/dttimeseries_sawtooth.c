#include <math.h>
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
#include <dtmc_base/dttimeseries_sawtooth.h>

#define TAG "dttimeseries_sawtooth"

typedef struct dttimeseries_sawtooth_t
{
    int32_t model_number;

    double min;
    double max;
    double rise_time;   // seconds; upslope duration
    double fall_time;   // seconds; downslope duration
} dttimeseries_sawtooth_t;

DTTIMESERIES_DECLARE_API(dttimeseries_sawtooth);
DTTIMESERIES_INIT_VTABLE(dttimeseries_sawtooth);

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sawtooth_create(dttimeseries_sawtooth_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    DTERR_C(dttimeseries_set_vtable(DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_SAWTOOTH, &dttimeseries_sawtooth_vt));

    DTERR_C(dtheaper_alloc_and_zero((int32_t)sizeof(dttimeseries_sawtooth_t), "dttimeseries_sawtooth_t", (void**)self_ptr));

    dttimeseries_sawtooth_t* self = (dttimeseries_sawtooth_t*)*self_ptr;

    self->model_number = DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_SAWTOOTH;
    self->min       = 0.0;
    self->max       = 1.0;
    self->rise_time = 0.9;
    self->fall_time = 0.1;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sawtooth_configure(dttimeseries_sawtooth_t* self DTTIMESERIES_CONFIGURE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(kvp_list);

    const char* s = NULL;
    double v = 0.0;

    DTERR_C(dtkvp_list_get(kvp_list, "min", &s));
    if (s != NULL && dtparse_double(s, &v))
        self->min = v;

    s = NULL;
    DTERR_C(dtkvp_list_get(kvp_list, "max", &s));
    if (s != NULL && dtparse_double(s, &v))
        self->max = v;

    s = NULL;
    DTERR_C(dtkvp_list_get(kvp_list, "rise_time", &s));
    if (s != NULL && dtparse_double(s, &v) && v > 0.0)
        self->rise_time = v;

    s = NULL;
    DTERR_C(dtkvp_list_get(kvp_list, "fall_time", &s));
    if (s != NULL && dtparse_double(s, &v) && v > 0.0)
        self->fall_time = v;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sawtooth_read(dttimeseries_sawtooth_t* self DTTIMESERIES_READ_ARGS)
{
    dterr_t* dterr = NULL;

    double period = self->rise_time + self->fall_time;
    double t = fmod((double)microseconds * 1e-6, period);

    if (t < self->rise_time)
        *value = self->min + (self->max - self->min) * (t / self->rise_time);
    else
        *value = self->max - (self->max - self->min) * ((t - self->rise_time) / self->fall_time);

    goto cleanup;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sawtooth_to_string(dttimeseries_sawtooth_t* self DTTIMESERIES_TO_STRING_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_string);
    snprintf(out_string,
      out_string_capacity,
      "sawtooth min=%f max=%f rise_time=%f fall_time=%f",
      self->min,
      self->max,
      self->rise_time,
      self->fall_time);

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern void
dttimeseries_sawtooth_dispose(dttimeseries_sawtooth_t* self)
{
    if (self != NULL)
    {
        dtheaper_free(self);
    }
}
