/*
 * dtnetportal_shm -- Shared-memory netportal implementation.
 *
 * Implements the dtnetportal interface using POSIX shared memory as the
 * transport, enabling low-latency inter-process publish/subscribe on Linux
 * without network overhead. Follows the standard create/init/configure
 * lifecycle and registers its vtable for model-number dispatch via the
 * dtnetportal registry.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtcore/dtobject.h>

#include <dtmc_base/dtnetportal.h>

typedef struct dtnetportal_shm_config_t
{
    int placeholder;
} dtnetportal_shm_config_t;

typedef struct dtnetportal_shm_t dtnetportal_shm_t;

extern dterr_t*
dtnetportal_shm_create(dtnetportal_shm_t** self_ptr);

extern dterr_t*
dtnetportal_shm_init(dtnetportal_shm_t* self);

extern dterr_t*
dtnetportal_shm_configure(dtnetportal_shm_t* self, dtnetportal_shm_config_t* config);

// --------------------------------------------------------------------------------------
// Interface plumbing.

DTNETPORTAL_DECLARE_API(dtnetportal_shm);
DTOBJECT_DECLARE_API(dtnetportal_shm);