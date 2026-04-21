#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtheaper.h>
#include <dtcore/dtkvp.h>
#include <dtcore/dtparse.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtmc_base/dttimeseries.h>
#include <dtmc_base/dttimeseries_beat.h>

#define TAG "dttimeseries_beat"

typedef struct dttimeseries_beat_t
{
    int32_t model_number;

    double f1;
    double f2;
    double amplitude;
    double offset;
} dttimeseries_beat_t;

DTTIMESERIES_DECLARE_API(dttimeseries_beat);
DTTIMESERIES_INIT_VTABLE(dttimeseries_beat);

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_beat_create(dttimeseries_beat_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    DTERR_C(dttimeseries_set_vtable(DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_BEAT, &dttimeseries_beat_vt));

    DTERR_C(dtheaper_alloc_and_zero((int32_t)sizeof(dttimeseries_beat_t), "dttimeseries_beat_t", (void**)self_ptr));

    dttimeseries_beat_t* self = *self_ptr;

    self->model_number = DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_BEAT;
    self->f1 = 1.0;
    self->f2 = 1.1;
    self->amplitude = 1.0;
    self->offset = 0.0;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_beat_configure(dttimeseries_beat_t* self DTTIMESERIES_CONFIGURE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(kvp_list);

    const char* s = NULL;

    DTERR_C(dtkvp_list_get(kvp_list, "f1", &s));
    if (s != NULL)
    {
        double v = 0.0;
        if (dtparse_double(s, &v))
            self->f1 = v;
        s = NULL;
    }

    DTERR_C(dtkvp_list_get(kvp_list, "f2", &s));
    if (s != NULL)
    {
        double v = 0.0;
        if (dtparse_double(s, &v))
            self->f2 = v;
        s = NULL;
    }

    DTERR_C(dtkvp_list_get(kvp_list, "amplitude", &s));
    if (s != NULL)
    {
        double v = 0.0;
        if (dtparse_double(s, &v))
            self->amplitude = v;
        s = NULL;
    }

    DTERR_C(dtkvp_list_get(kvp_list, "offset", &s));
    if (s != NULL)
    {
        double v = 0.0;
        if (dtparse_double(s, &v))
            self->offset = v;
        s = NULL;
    }

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_beat_read(dttimeseries_beat_t* self DTTIMESERIES_READ_ARGS)
{
    dterr_t* dterr = NULL;

    double t = (double)microseconds * 1e-6;
    *value = self->offset + self->amplitude * (sin(2.0 * M_PI * self->f1 * t) + sin(2.0 * M_PI * self->f2 * t));

    goto cleanup;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_beat_to_string(dttimeseries_beat_t* self DTTIMESERIES_TO_STRING_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_string);
    snprintf(out_string,
      out_string_capacity,
      "beat f1=%f f2=%f amplitude=%f offset=%f",
      self->f1,
      self->f2,
      self->amplitude,
      self->offset);

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern void
dttimeseries_beat_dispose(dttimeseries_beat_t* self)
{
    if (self != NULL)
    {
        dtheaper_free(self);
    }
}
