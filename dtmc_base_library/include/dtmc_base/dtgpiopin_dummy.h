/*
 * dtgpiopin_dummy -- Deterministic GPIO pin stand-in for unit testing.
 *
 * Implements the full dtgpiopin vtable (attach, enable, read, write,
 * concat_format, dispose) in software. Configurable behavior flags control
 * whether writes trigger edge callbacks and whether enabling the pin emits
 * an initial edge, making it possible to exercise interrupt-driven logic
 * under a single-threaded test runner without real GPIO hardware or SDKs.
 *
 * cdox v1.0.2
 */
#pragma once
// See markdown documentation at the end of this file.

// Stand-in dtgpiopin provider for environments without real GPIO hardware.

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtmc_base/dtgpiopin.h>

// This struct exists to mirror tweakable test behavior while keeping the dummy light.
typedef struct dtgpiopin_dummy_behavior_t
{
    // Configuration knobs (read at init/configure time):
    bool start_level_high;    // default false; if true, init level = 1
    bool auto_edge_on_write;  // default true; if false, write() won’t emit callbacks
    bool emit_edge_on_enable; // default false; if true, enabling emits RISING if level=1 else FALLING

    // (optional) read-only mirrors you can populate for white-boxy tests:
    bool irq_enabled_snapshot; // set in enable()
    bool last_level_snapshot;  // updated in write()/configure/init
} dtgpiopin_dummy_behavior_t;

typedef struct
{
    int32_t pin_number;
    dtgpiopin_mode_t mode;
    dtgpiopin_pull_t pull;
    dtgpiopin_drive_t drive;
} dtgpiopin_dummy_config_t;

// forward declare the concrete dummy type
typedef struct dtgpiopin_dummy_t dtgpiopin_dummy_t;

// **Usage:** Use when you want the dummy to manage its own storage (pair with generated `dispose`).
extern dterr_t*
dtgpiopin_dummy_create(dtgpiopin_dummy_t** self_ptr);

// **Usage:** Use for stack/static lifetimes when you don't want heap allocation.
// Registers this model's vtable with the ::dtgpiopin registry so facade calls can dispatch here.
extern dterr_t*
dtgpiopin_dummy_init(dtgpiopin_dummy_t* self);

// **Notes:**
// - Call before first facade calls if you need non-default pin configuration.
// - Applies the supplied pin configuration to the dummy (mode/pull/drive/number).
extern dterr_t*
dtgpiopin_dummy_configure(dtgpiopin_dummy_t* self, dtgpiopin_dummy_config_t* config);

// Behavior accessors required by the unit tests
extern dterr_t*
dtgpiopin_dummy_get_behavior(dtgpiopin_dummy_t* self, dtgpiopin_dummy_behavior_t* out);
extern dterr_t*
dtgpiopin_dummy_set_behavior(dtgpiopin_dummy_t* self, const dtgpiopin_dummy_behavior_t* in);

// The dummy is a concrete implementation of the ::dtgpiopin vtable-based interface.
// The macro below declares the public facade entry points (`attach`, `enable`, `read`, `write`, `dispose`)
// that down-cast the opaque handle and dispatch via this model's registered vtable.

// --------------------------------------------------------------------------------------

DTGPIOPIN_DECLARE_API(dtgpiopin_dummy) // < Declare facade glue for the dummy model.

#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]
# dtgpiopin_dummy

A dummy GPIO-pin implementation that satisfies the **dtgpiopin** vtable interface for deterministic, single-threaded tests. It provides a lightweight stand-in when real GPIO hardware/SDKs are unavailable.

## Mini-guide

- Use `dtgpiopin_dummy_create` for heap lifetime; use `dtgpiopin_dummy_init` for stack/static lifetime.
- Apply pin configuration with `dtgpiopin_dummy_configure` before first use (sets pin number, mode, pull, drive).
- The dummy invokes the attached ISR callback only when you trigger it through facade operations that change level (e.g., `write` from low→high emits a RISING edge if IRQs are enabled and a callback is attached).
- Error contract: functions returning `dterr_t*` yield `NULL` on success; non-NULL on failure (caller owns the error).
- Implements the dtgpiopin vtable; dispatch is by `model_number` published during `init`.

## Example

```c
#include <dtmc_base/dtgpiopin_dummy.h>
#include <dtcore/dterr.h>

static void on_edge(dtgpiopin_edge_t edge, void* ctx) {
    (void)ctx;
    // test records, asserts, etc.
}

void example(void)
{
    dterr_t* err = NULL;

    // Create a heap-owned dummy pin
    dtgpiopin_dummy_t* pin = NULL;
    err = dtgpiopin_dummy_create(&pin);
    if (err != NULL) { /* handle error and return */ }

    // Configure as output (pin 5, no pull, default drive)
    dtgpiopin_dummy_config_t cfg = { .pin_number = 5,
                               .mode = DTGPIOPIN_MODE_OUTPUT,
                               .pull = DTGPIOPIN_PULL_NONE,
                               .drive = DTGPIOPIN_DRIVE_DEFAULT };
    err = dtgpiopin_dummy_configure(pin, &cfg);
    if (err != NULL) { /* handle error and dispose */ }

    // Attach a callback and enable "interrupts"
    err = dtgpiopin_dummy_attach(pin, on_edge, NULL);
    if (err == NULL) err = dtgpiopin_dummy_enable(pin, true);

    // Write high -> will invoke on_edge(RISING, …)
    if (err == NULL) err = dtgpiopin_dummy_write(pin, true);

    // Cleanup
    dtgpiopin_dummy_dispose(pin);
}
```

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
