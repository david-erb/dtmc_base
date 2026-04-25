/*
 * dtcpu -- Platform-specific CPU timing, busy-wait, and identification.
 *
 * Provides a lightweight abstraction over hardware timing: a mark-and-elapsed
 * pair for microsecond-resolution interval measurement, a busy-wait primitive,
 * a random integer generator, and a permanent unique-identifier string for
 * the running CPU or platform instance. Implemented separately for each
 * supported target (ESP-IDF, Zephyr, Linux, RP2040, etc.).
 *
 * cdox v1.0.2
 */
#pragma once

#include <inttypes.h>

typedef uint64_t dtcpu_microseconds_t;
#define DTCPU_MICROSECONDS_PRI PRIu64

typedef struct dtcpu_t
{
    uint32_t old, new;
} dtcpu_t;

extern dterr_t*
dtcpu_sysinit(void);

extern void
dtcpu_mark(dtcpu_t* m);

extern dtcpu_microseconds_t
dtcpu_elapsed_microseconds(const dtcpu_t* m);

// platform-specific busy-wait for the specified number of microseconds
extern void
dtcpu_busywait_microseconds(dtcpu_microseconds_t microseconds);

// return a random integer in the full int32_t range
extern int32_t
dtcpu_random_int32(void);

// return permanent unique identifier string for this particular CPU/platform
extern const char*
dtcpu_identify(void);