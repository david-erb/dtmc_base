#include <math.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dtbytes.h>
#include <dtcore/dterr.h>
#include <dtcore/dteventlogger.h>
#include <dtcore/dtglyph.h>
#include <dtcore/dtglyph_dos.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpicosdk_helper.h>
#include <dtcore/dtraster.h>
#include <dtcore/dtraster_rgb565.h>
#include <dtcore/dtraster_rgba8888.h>
#include <dtcore/dtrgba8888.h>
#include <dtcore/dtstr.h>

#include <dtmc_base/dtcpu.h>
#include <dtmc_base/dtruntime.h>

#include <dtmc_base/dtdisplay.h>

#include <dtmc_base_demos/demo_display_hello.h>
#include <dtmc_base_demos/demo_helpers.h>

#define TAG demo_name

// the demo's privates
typedef struct demo_t
{
    demo_config_t config;

    int32_t cycle_number;

    bool got_first_read;
} demo_t;

// demo helpers

// --------------------------------------------------------------------------------------------
static dterr_t*
demo_helper_draw_rectangles_primary(demo_t* self);
static dterr_t*
demo_helper_draw_rectangles_debug(demo_t* self);
static dterr_t*
demo_helper_create_raster(demo_t* self, dtraster_handle* out_raster_handle, int32_t w, int32_t h);
static dterr_t*
demo_helper_fill_raster(dtraster_handle handle, dtrgba8888_t color);
static dterr_t*
demo_helper_draw_string(demo_t* self,
  const char* text,
  dtrgba8888_t background_color,
  dtrgba8888_t foreground_color,
  int32_t x,
  int32_t y);
// --------------------------------------------------------------------------------------
// return a string description of the demo (the returned string is heap allocated)
dterr_t*
demo_describe(demo_t* self, char** out_description)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_description);

    char* d = *out_description;
    char* s = "\n    ";
    d = dtstr_concat_format(d, s, "This demo shows hello in a window.");
    *out_description = d;

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Create a new instance, allocating memory as needed
dterr_t*
demo_create(demo_t** self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    *self = (demo_t*)malloc(sizeof(demo_t));
    if (*self == NULL)
    {
        dterr = dterr_new(DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for demo_t", sizeof(demo_t));
        goto cleanup;
    }

    memset(*self, 0, sizeof(demo_t));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Configure the demo instance with handles to implementations and settings
dterr_t*
demo_configure( //
  demo_t* self,
  demo_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    self->config = *config;
cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Run the demo logic (loops forever, blocking)
dterr_t*
demo_start(demo_t* self)
{
    dterr_t* dterr = NULL;

    DTERR_ASSERT_NOT_NULL(self);

    // print the description of the demo
    {
        char* description = NULL;
        DTERR_C(dtmc_base_demo_helpers_decorate_description((void*)self, (demo_describe_fn)demo_describe, &description));
        dtlog_info(TAG, "%s", description);
    }

    DTERR_C(dtdisplay_attach(self->config.display_handle));

    demo_helper_draw_rectangles_debug(self);
    demo_helper_draw_rectangles_primary(self);

    int x = 20;
    int y = 40;
    DTERR_C(demo_helper_draw_string(self, "hello\nworld", DTRGBA8888_BLACK, DTRGBA8888_WHITE, x, y += 20));

    x = 120;
    y = 40;
    DTERR_C(demo_helper_draw_string(self, "red", DTRGBA8888_BLACK, DTRGBA8888_RED, x, y += 0));

    DTERR_C(demo_helper_draw_string(self, "green", DTRGBA8888_BLACK, DTRGBA8888_GREEN, x, y += 20));

    DTERR_C(demo_helper_draw_string(self, "blue", DTRGBA8888_BLACK, DTRGBA8888_BLUE, x, y += 20));

    DTERR_C(demo_helper_draw_string(self, "cyan", DTRGBA8888_BLACK, DTRGBA8888_CYAN, x, y += 20));

    DTERR_C(demo_helper_draw_string(self, "magenta", DTRGBA8888_BLACK, DTRGBA8888_MAGENTA, x, y += 20));

    DTERR_C(demo_helper_draw_string(self, "yellow", DTRGBA8888_BLACK, DTRGBA8888_YELLOW, x, y += 20));

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------
// Stop, unregister and dispose of the demo instance resources
void
demo_dispose(demo_t* self)
{
    if (self == NULL)
        return;

    free(self);
}

// --------------------------------------------------------------------------------------------
static dterr_t*
demo_helper_create_raster(demo_t* self, dtraster_handle* out_raster_handle, int32_t w, int32_t h)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_raster_handle);
    DTERR_ASSERT_NOT_NULL(self->config.display_handle);

    DTERR_C(dtdisplay_create_compatible_raster(self->config.display_handle, out_raster_handle, w, h));

    DTERR_C(dtraster_new_buffer(*out_raster_handle));

cleanup:

    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
demo_helper_fill_raster(dtraster_handle handle, dtrgba8888_t color)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(handle);

    dtraster_shape_t shape;
    DTERR_C(dtraster_get_shape(handle, &shape));

    for (int y = 0; y < shape.h; y++)
    {
        for (int x = 0; x < shape.w; x++)
        {
            DTERR_C(dtraster_store_pixel(handle, x, y, color));
        }
    }

cleanup:
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
demo_helper_draw_string(demo_t* self,
  const char* text,
  dtrgba8888_t background_color,
  dtrgba8888_t foreground_color,
  int32_t x,
  int32_t y)
{
    dterr_t* dterr = NULL;
    dtglyph_dos_t* glyph_object = NULL;
    dtglyph_handle glyph_handle = NULL;
    dtraster_handle glyph_raster = NULL;

    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(text);
    DTERR_ASSERT_NOT_NULL(self->config.display_handle);

    DTERR_C(dtglyph_dos_create(&glyph_object));

    glyph_handle = (dtglyph_handle)glyph_object;

    int32_t w, h;

    DTERR_C(dtglyph_calculate_box(glyph_handle, text, &w, &h));

    DTERR_C(demo_helper_create_raster(self, &glyph_raster, w, h));

    DTERR_C(demo_helper_fill_raster(glyph_raster, background_color));

    DTERR_C(dtglyph_blit(glyph_handle, text, glyph_raster, 0, 0, foreground_color));

    DTERR_C(dtdisplay_blit(self->config.display_handle, glyph_raster, x, y));

cleanup:
    dtraster_dispose(glyph_raster);
    dtglyph_dispose(glyph_handle);
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
demo_helper_draw_rectangles_primary(demo_t* self)
{
    dterr_t* dterr = NULL;
    dtraster_handle drawing_raster = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->config.display_handle);

    int count = 8;
    int w = 20;
    int h = 20;
    int gutter = 2;
    bool vertically = false;

    DTERR_C(demo_helper_create_raster(self, &drawing_raster, count * (w + gutter) - gutter, h));

    DTERR_C(dtraster_draw_rectangles_primary(drawing_raster, 0, 0, w, h, gutter, vertically, DTRGBA8888_WHITE));

    DTERR_C(dtdisplay_blit(self->config.display_handle, drawing_raster, 0, 10));

cleanup:
    dtraster_dispose(drawing_raster);
    return dterr;
}

// --------------------------------------------------------------------------------------------
static dterr_t*
demo_helper_draw_rectangles_debug(demo_t* self)
{
    dterr_t* dterr = NULL;
    dtraster_handle drawing_raster = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->config.display_handle);

    int count = 8;
    int w = 4;
    int h = 4;
    int gutter = 0;
    bool vertically = false;

    DTERR_C(demo_helper_create_raster(self, &drawing_raster, count * (w + gutter) - gutter, h));

    {
        char s[256];
        dtobject_to_string((dtobject_handle)drawing_raster, s, sizeof(s));
        dtlog_debug(TAG, "created drawing raster %s", s);
    }

    DTERR_C(dtraster_draw_rectangles_primary(drawing_raster, 0, 0, w, h, gutter, vertically, DTRGBA8888_TRANSPARENT));

    {
        dtbuffer_t* buffer = ((dtbuffer_t**)drawing_raster)[1];
        char s[256];
        dtbytes_compose_hex(buffer->payload, 32, s, sizeof(s));
        dtlog_debug(TAG, "%s are the primary rectangles", s);
    }

    DTERR_C(dtdisplay_blit(self->config.display_handle, drawing_raster, 0, 0));

cleanup:
    dtraster_dispose(drawing_raster);
    return dterr;
}
