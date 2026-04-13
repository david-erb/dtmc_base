/*
 * dtdebouncer -- Time-qualified debounce filter for binary signals.
 *
 * Accepts raw boolean samples and emits a stable state only after the
 * candidate has held steady for a configurable millisecond interval. An
 * optional callback fires on each accepted transition. The instance tracks
 * separate counts for raw edges, debounced edges, rising transitions, and
 * falling transitions, supporting both callback-driven and polled usage
 * patterns without GPIO or platform dependencies.
 *
 * cdox v1.0.2
 */
#pragma once
#include <sys/types.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

typedef uint32_t dtdebouncer_milliseconds_t;
#define DTDEBOUNCER_MILLISECONDS_PRI PRIu32

typedef void (*dtdebouncer_callback_fn)(bool new_state, void* callback_context);
typedef dtdebouncer_milliseconds_t (*dtdebouncer_time_fn)(void);

typedef struct dtdebouncer_t
{
    // Core debounce state
    bool stable_state;                            // last accepted (debounced) state
    bool candidate_state;                         // raw candidate waiting to be accepted
    dtdebouncer_milliseconds_t candidate_time_ms; // when candidate was first seen
    dtdebouncer_milliseconds_t debounce_ms;       // required stable time before accepting candidate
    dtdebouncer_callback_fn callback_fn;          // called on accepted state changes (optional)
    void* callback_context;                       // context for callback
    dtdebouncer_time_fn time_now_ms;              // function to get current time in ms

    // Diagnostics
    uint32_t raw_edge_count;       // every raw edge (any transition)
    uint32_t debounced_edge_count; // every accepted (debounced) transition
    uint32_t rise_count;           // accepted 0 -> 1 transitions
    uint32_t fall_count;           // accepted 1 -> 0 transitions
} dtdebouncer_t;

// --------------------------------------------------------------------------------------------
// Initialize a debouncer instance.
// - self must be a valid pointer to storage for dtdebouncer_t.
// - time_fn must be non-NULL and return a monotonically increasing millisecond counter
//   (wrap-around is handled using unsigned arithmetic).
// Returns NULL on success, or a dterr_t* on failure (e.g., failed assertions).
extern dterr_t*
dtdebouncer_init( //
  dtdebouncer_t* self,
  bool initial_state,
  dtdebouncer_milliseconds_t debounce_ms,
  dtdebouncer_callback_fn callback_fn,
  void* callback_context,
  dtdebouncer_time_fn time_fn);

// --------------------------------------------------------------------------------------------
// Feed a new raw state sample into the debouncer.
// This function is not thread-safe; call it from a single context (e.g., ISR or polling loop).
extern void
dtdebouncer_update(dtdebouncer_t* self, bool raw_state);

#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]

# dtdebouncer

Debounce binary signals with time-qualified state changes

This group of functions provides time-based debouncing for binary signals. 
It is intended for use in polling loops or interrupt-adjacent contexts where raw transitions may be noisy.
It is designed to accept raw samples and emit stable state changes via an optional callback.

## Mini-guide

- Call the initializer once before use to set the initial state and capture the current time in milliseconds.
- Pass each raw sample to the update function so the module can observe state changes over time.
- Set the debounce interval at initialization to define how long a candidate state must remain unchanged.
- Provide a callback if notifications are needed, which is invoked only after a state becomes stable.
- Read diagnostic counters from the instance to examine raw transitions and accepted edges.

## Example

```c
static dtdebouncer_milliseconds_t now_ms(void)
{
    return 0;
}

static void on_change(bool new_state, void* ctx)
{
    (void)new_state;
    (void)ctx;
}

void example(void)
{
    dtdebouncer_t d;
    dtdebouncer_init(&d, false, 10, on_change, NULL, now_ms);
    dtdebouncer_update(&d, true);
}
```

## Data structures

### dtdebouncer_callback_fn

Defines the signature for notification callbacks invoked on accepted state changes.

Members:

### dtdebouncer_milliseconds_t

Represents a millisecond time value used for debounce timing.

Members:

### dtdebouncer_t

Holds debounce state, configuration, callbacks, and diagnostic counters.

Members:

> `bool stable_state` Last accepted debounced state.  
> `bool candidate_state` Current raw candidate awaiting acceptance.  
> `dtdebouncer_milliseconds_t candidate_time_ms` Timestamp when the current candidate was first observed.  
> `dtdebouncer_milliseconds_t debounce_ms` Required stable duration before accepting a candidate.  
> `dtdebouncer_callback_fn callback_fn` Optional callback invoked on accepted state changes.  
> `void* callback_context` Caller-provided context passed to the callback.  
> `dtdebouncer_time_fn time_now_ms` Function used to obtain the current time in milliseconds.  
> `uint32_t raw_edge_count` Count of all observed raw transitions.  
> `uint32_t debounced_edge_count` Count of accepted debounced transitions.  
> `uint32_t rise_count` Count of accepted low-to-high transitions.  
> `uint32_t fall_count` Count of accepted high-to-low transitions.  

### dtdebouncer_time_fn

Defines the signature for functions that provide the current time in milliseconds.

Members:

## Macros

### DTDEBOUNCER_MILLISECONDS_PRI

Provides a printf format specifier for millisecond values.

## Functions

### dtdebouncer_init

Initializes a debouncer instance with timing, callback, and initial state.

Params:

> `dtdebouncer_t* self` Storage for the debouncer instance.  
> `bool initial_state` Initial accepted state.  
> `dtdebouncer_milliseconds_t debounce_ms` Required stable duration in milliseconds.  
> `dtdebouncer_callback_fn callback_fn` Optional callback for accepted transitions.  
> `void* callback_context` Caller-provided context for the callback.  
> `dtdebouncer_time_fn time_fn` Function that returns the current time in milliseconds.  

Return: `dterr_t*` NULL on success or an error pointer on failure.

### dtdebouncer_update

Processes a raw binary sample and advances debounce state.

Params:

> `dtdebouncer_t* self` Debouncer instance to update.  
> `bool raw_state` Newly observed raw state.  

Return: `void`  No return value.

<!-- FG_IDC: 42333d2e-7aa0-4c25-8920-d7857cb11ebf | FG_UTC: 2026-01-17T09:52:33Z | FG_MAN=yes -->

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
