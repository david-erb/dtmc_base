/*
 * dttimeseries_beat -- Beat-frequency signal source implementing the dttimeseries facade.
 *
 * Evaluates offset + amplitude * (sin(2*pi*f1*t) + sin(2*pi*f2*t)) for a
 * microsecond timestamp t converted to seconds.
 *
 * Produces a continuously cycling waveform whose amplitude envelope pulses at
 * the beat rate |f1-f2| Hz while the carrier oscillates at (f1+f2)/2 Hz.
 * Selecting different f1/f2 pairs per channel gives each channel a distinct
 * visual rhythm with no decay. Follows the standard create/configure/read
 * lifecycle and registers its vtable for polymorphic dispatch alongside other
 * dttimeseries backends.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dttimeseries.h>

typedef struct dttimeseries_beat_t dttimeseries_beat_t;

extern dterr_t*
dttimeseries_beat_create(dttimeseries_beat_t** self_ptr);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTTIMESERIES_DECLARE_API(dttimeseries_beat);
