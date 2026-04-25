#include <stdlib.h>
#include <string.h>

#include <dtcore/dterr.h>
#include <dtcore/dtstr.h>
#include <dtmc_base/dtmc_base_constants.h>

#include <dtmc_base/dtgpiopin.h>
#include <dtmc_base/dtgpiopin_dummy.h> // declares dtgpiopin_dummy_t (opaque), behavior API

/*
    Intent:
      - Single-threaded, no real ISR or HW
      - Returns dterr_t* on error, NULL on success
      - Publish vtable during init via dtgpiopin_set_vtable()
      - Provide create()/init()/configure() like dtsemaphore_dummy
*/

// Declare global vtable instance for this class
DTGPIOPIN_INIT_VTABLE(dtgpiopin_dummy)

/*------------------------------------------------------------------------*/
// Concrete instance layout (private to this TU)
typedef struct dtgpiopin_dummy_t
{
    DTGPIOPIN_COMMON_MEMBERS; // int32_t model_number;
    bool _is_malloced;        // mirrors dtsemaphore_dummy pattern

    dtgpiopin_dummy_config_t config; // input/pull/drive settings

    // Runtime state
    bool level;       // current logical level
    bool irq_enabled; // whether "interrupts" are enabled

    // ISR hookup
    dtgpiopin_isr_fn cb;  // attached callback (if any)
    void* caller_context; // user context pointer

    // Test behavior knobs (white-box insights live here)
    dtgpiopin_dummy_behavior_t behavior;
} dtgpiopin_dummy_t;

/*------------------------------------------------------------------------*/
dterr_t*
dtgpiopin_dummy_create(dtgpiopin_dummy_t** self_ptr)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self_ptr);

    *self_ptr = (dtgpiopin_dummy_t*)malloc(sizeof(dtgpiopin_dummy_t));
    if (*self_ptr == NULL)
    {
        dterr = dterr_new(
          DTERR_NOMEM, DTERR_LOC, NULL, "failed to allocate %zu bytes for dtgpiopin_dummy_t", sizeof(dtgpiopin_dummy_t));
        goto cleanup;
    }

    DTERR_C(dtgpiopin_dummy_init(*self_ptr));
    (*self_ptr)->_is_malloced = true;

cleanup:
    if (dterr != NULL)
    {
        if (self_ptr != NULL && *self_ptr != NULL)
        {
            free(*self_ptr);
            *self_ptr = NULL;
        }
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "dtgpiopin_dummy_create failed");
    }
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtgpiopin_dummy_init(dtgpiopin_dummy_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    memset(self, 0, sizeof(*self));
    self->model_number = DTMC_BASE_CONSTANTS_GPIOPIN_MODEL_DUMMY; // matches your registry constants

    // Default config is safe INPUT with no pull.
    self->config.pin_number = 0;
    self->config.mode = DTGPIOPIN_MODE_INPUT;
    self->config.pull = DTGPIOPIN_PULL_NONE;
    self->config.drive = DTGPIOPIN_DRIVE_DEFAULT;

    // Runtime defaults
    self->level = false;
    self->irq_enabled = false;
    self->cb = NULL;
    self->caller_context = NULL;

    // Behavior defaults
    self->behavior.start_level_high = false;
    self->behavior.auto_edge_on_write = true;
    self->behavior.emit_edge_on_enable = false;
    self->behavior.irq_enabled_snapshot = false;
    self->behavior.last_level_snapshot = false;

    // Publish vtable for this model (idempotent in single-threaded tests).
    DTERR_C(dtgpiopin_set_vtable(self->model_number, &dtgpiopin_dummy_vt));

cleanup:
    if (dterr != NULL)
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "dtgpiopin_dummy_init failed");
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtgpiopin_dummy_configure(dtgpiopin_dummy_t* self, dtgpiopin_dummy_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);

    self->config = *config;

    // Apply behavior-driven initial level after (re)configuration.
    if (self->behavior.start_level_high)
    {
        self->level = true;
        self->behavior.last_level_snapshot = true;
    }
    else
    {
        self->level = false;
        self->behavior.last_level_snapshot = false;
    }

cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/
// Behavior accessors required by the unit tests

dterr_t*
dtgpiopin_dummy_get_behavior(dtgpiopin_dummy_t* self, dtgpiopin_dummy_behavior_t* out)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out);
    *out = self->behavior;
cleanup:
    return dterr;
}

dterr_t*
dtgpiopin_dummy_set_behavior(dtgpiopin_dummy_t* self, const dtgpiopin_dummy_behavior_t* in)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(in);
    self->behavior = *in;
// If test sets behavior pre-configure, level will be applied in configure().
// If they set it post-configure and need immediate effect, they can call configure() again.
cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/
// Vtable methods (backend)

dterr_t*
dtgpiopin_dummy_attach(dtgpiopin_dummy_t* self DTGPIOPIN_ATTACH_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    self->cb = cb;
    self->caller_context = caller_context;
cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtgpiopin_dummy_enable(dtgpiopin_dummy_t* self DTGPIOPIN_ENABLE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    self->irq_enabled = enable;
    self->behavior.irq_enabled_snapshot = enable;

    // If enabling and requested by behavior, emit a level-derived edge immediately.
    // This matches tests that expect a notification on enable based on current level.
    if (enable && self->cb != NULL && self->behavior.emit_edge_on_enable)
    {
        dtgpiopin_edge_t e = self->level ? DTGPIOPIN_IRQ_RISING : DTGPIOPIN_IRQ_FALLING;
        self->cb(e, self->caller_context);
    }

cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtgpiopin_dummy_read(dtgpiopin_dummy_t* self DTGPIOPIN_READ_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_level);

    *out_level = self->level;
cleanup:
    return dterr;
}

/*------------------------------------------------------------------------*/
dterr_t*
dtgpiopin_dummy_write(dtgpiopin_dummy_t* self DTGPIOPIN_WRITE_ARGS)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);

    // Policy: writing is only valid for OUTPUT or INOUT
    if (!(self->config.mode == DTGPIOPIN_MODE_OUTPUT || self->config.mode == DTGPIOPIN_MODE_INOUT))
    {
        dterr = dterr_new(DTERR_FAIL,
          DTERR_LOC,
          NULL,
          "write invalid in current mode (pin=%u, mode=%d)",
          self->config.pin_number,
          (int)self->config.mode);
        return dterr;
    }

    bool prev = self->level;
    self->level = level;
    self->behavior.last_level_snapshot = level;

    // If IRQs enabled and callback present, decide whether to emit.
    if (self->irq_enabled && self->cb != NULL)
    {
        // Gate by behavior flag; also require an actual edge (prev != level)
        if (self->behavior.auto_edge_on_write && (prev != level))
        {
            dtgpiopin_edge_t edge = level ? DTGPIOPIN_IRQ_RISING : DTGPIOPIN_IRQ_FALLING;
            self->cb(edge, self->caller_context);
        }
    }
cleanup:
    return dterr;
}

// -----------------------------------------------------------------------------
// Human-readable status string

dterr_t*
dtgpiopin_dummy_concat_format(dtgpiopin_dummy_t* self DTGPIOPIN_CONCAT_FORMAT_ARGS)
{
    dterr_t* dterr = NULL;
    char* s = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(out_str);

    s = dtstr_concat_format(s, NULL, "gpiopin_dummy (%" PRId32 ") pin=%" PRIu32, self->model_number, self->config.pin_number);

cleanup:
    *out_str = s;
    return dterr;
}

/*------------------------------------------------------------------------*/
void
dtgpiopin_dummy_dispose(dtgpiopin_dummy_t* self)
{
    if (self == NULL)
        return;

    // Disable and drop callback first to avoid late calls.
    self->irq_enabled = false;
    self->cb = NULL;
    self->caller_context = NULL;

    if (self->_is_malloced)
    {
        free(self);
    }
    else
    {
        memset(self, 0, sizeof(*self));
    }
}
