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
#include <dtmc_base/dttimeseries_sine.h>

#define TAG "dttimeseries_sine"

typedef struct dttimeseries_sine_t
{
    int32_t model_number;

    double frequency;
    double amplitude;
    double offset;
} dttimeseries_sine_t;

DTTIMESERIES_DECLARE_API(dttimeseries_sine);
DTTIMESERIES_INIT_VTABLE(dttimeseries_sine);

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sine_create(dttimeseries_sine_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    DTERR_C(dttimeseries_set_vtable(DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_SINE, &dttimeseries_sine_vt));

    DTERR_C(dtheaper_alloc_and_zero((int32_t)sizeof(dttimeseries_sine_t), "dttimeseries_sine_t", (void**)self_ptr));

    dttimeseries_sine_t* self = (dttimeseries_sine_t*)*self_ptr;

    self->model_number = DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_SINE;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sine_configure(dttimeseries_sine_t* self DTTIMESERIES_CONFIGURE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(kvp_list);

    const char* frequency_string = NULL;
    DTERR_C(dtkvp_list_get(kvp_list, "frequency", &frequency_string));
    if (frequency_string != NULL)
    {
        double frequency_double = 0.0;
        bool parse_result = dtparse_double(frequency_string, &frequency_double);
        if (parse_result)
            self->frequency = frequency_double;
    }

    const char* amplitude_string = NULL;
    DTERR_C(dtkvp_list_get(kvp_list, "amplitude", &amplitude_string));
    if (amplitude_string != NULL)
    {
        double amplitude_double = 0.0;
        bool parse_result = dtparse_double(amplitude_string, &amplitude_double);
        if (parse_result)
            self->amplitude = amplitude_double;
    }

    const char* offset_string = NULL;
    DTERR_C(dtkvp_list_get(kvp_list, "offset", &offset_string));
    if (offset_string != NULL)
    {
        double offset_double = 0.0;
        bool parse_result = dtparse_double(offset_string, &offset_double);
        if (parse_result)
            self->offset = offset_double;
    }

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sine_read(dttimeseries_sine_t* self DTTIMESERIES_READ_ARGS)
{
    dterr_t* dterr = NULL;

    double t_seconds = (double)microseconds * 1e-6;
    *value = self->offset + self->amplitude * sin(2.0 * M_PI * self->frequency * t_seconds);

    goto cleanup;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_sine_to_string(dttimeseries_sine_t* self DTTIMESERIES_TO_STRING_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_string);
    snprintf(out_string, out_string_capacity, "sine frequency=%f amplitude=%f offset=%f", self->frequency, self->amplitude, self->offset);

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern void
dttimeseries_sine_dispose(dttimeseries_sine_t* self)
{
    if (self != NULL)
    {
        dtheaper_free(self);
    }
}
