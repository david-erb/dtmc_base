#include <inttypes.h>
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

#include <dtcore/dtrandomizer_browngrav.h>

#include <dtmc_base/dtmc_base_constants.h>

#include <dtmc_base/dttimeseries.h>
#include <dtmc_base/dttimeseries_browngrav.h>

#define TAG "dttimeseries_browngrav"

typedef struct dttimeseries_browngrav_t
{
    int32_t model_number;

    dtrandomizer_browngrav_t randomizer;

    double offset;
    double scale;
} dttimeseries_browngrav_t;

DTTIMESERIES_DECLARE_API(dttimeseries_browngrav);
DTTIMESERIES_INIT_VTABLE(dttimeseries_browngrav);

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_browngrav_create(dttimeseries_browngrav_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    DTERR_C(dttimeseries_set_vtable(DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_BROWNGRAV, &dttimeseries_browngrav_vt));

    DTERR_C(dtheaper_alloc_and_zero((int32_t)sizeof(dttimeseries_browngrav_t), "dttimeseries_browngrav_t", (void**)self_ptr));

    dttimeseries_browngrav_t* self = (dttimeseries_browngrav_t*)*self_ptr;

    self->model_number = DTMC_BASE_CONSTANTS_TIMESERIES_MODEL_BROWNGRAV;
    self->scale = 1.0;

    DTERR_C(dtrandomizer_browngrav_init(&self->randomizer));

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_browngrav_configure(dttimeseries_browngrav_t* self DTTIMESERIES_CONFIGURE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(kvp_list);

    dtrandomizer_browngrav_config_t cfg = self->randomizer.config;

    const char* s = NULL;

    DTERR_C(dtkvp_list_get(kvp_list, "attraction_point", &s));
    if (s != NULL)
    {
        dtparse_int32(s, &cfg.attraction_point);
        s = NULL;
    }

    DTERR_C(dtkvp_list_get(kvp_list, "attraction_strength", &s));
    if (s != NULL)
    {
        dtparse_int32(s, &cfg.attraction_strength);
        s = NULL;
    }

    DTERR_C(dtkvp_list_get(kvp_list, "noise_intensity", &s));
    if (s != NULL)
    {
        dtparse_int32(s, &cfg.noise_intensity);
        s = NULL;
    }

    DTERR_C(dtkvp_list_get(kvp_list, "seed", &s));
    if (s != NULL)
    {
        dtparse_int32(s, &cfg.seed);
        s = NULL;
    }

    DTERR_C(dtrandomizer_browngrav_config(&self->randomizer, &cfg));

    DTERR_C(dtkvp_list_get(kvp_list, "offset", &s));
    if (s != NULL)
    {
        double v = 0.0;
        if (dtparse_double(s, &v))
            self->offset = v;
        s = NULL;
    }

    DTERR_C(dtkvp_list_get(kvp_list, "scale", &s));
    if (s != NULL)
    {
        double v = 0.0;
        if (dtparse_double(s, &v))
            self->scale = v;
        s = NULL;
    }

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_browngrav_read(dttimeseries_browngrav_t* self DTTIMESERIES_READ_ARGS)
{
    dterr_t* dterr = NULL;

    (void)microseconds;

    int32_t raw = 0;
    DTERR_C(dtrandomizer_browngrav_next(&self->randomizer, &raw));
    *value = self->offset + self->scale * (double)raw;

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern dterr_t*
dttimeseries_browngrav_to_string(dttimeseries_browngrav_t* self DTTIMESERIES_TO_STRING_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_string);
    snprintf(out_string, out_string_capacity,
             "browngrav attraction_point=%" PRId32 " attraction_strength=%" PRId32
             " noise_intensity=%" PRId32 " seed=%" PRId32 " offset=%f scale=%f",
             self->randomizer.config.attraction_point,
             self->randomizer.config.attraction_strength,
             self->randomizer.config.noise_intensity,
             self->randomizer.config.seed,
             self->offset,
             self->scale);

cleanup:
    return dterr;
}

// ------------------------------------------------------------------------
extern void
dttimeseries_browngrav_dispose(dttimeseries_browngrav_t* self)
{
    if (self != NULL)
    {
        dtrandomizer_browngrav_dispose(&self->randomizer);
        dtheaper_free(self);
    }
}
