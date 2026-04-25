/*
 * dtnetprofile_distributor -- Publishes and collects dtnetprofile records over a netportal.
 *
 * Subscribes to the network-profile topic on a dtnetportal and places
 * arriving serialized profiles into a dtbufferqueue. The distribute function
 * serializes a local dtnetprofile_t and publishes it on the same topic.
 * Separating serialization, queuing, and transport keeps each layer
 * independently testable.
 *
 * cdox v1.0.2
 */
#pragma once

#include <dtcore/dterr.h>

#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtnetportal.h>

#include <dtmc_base/dtnetprofile.h>

typedef struct dtnetprofile_distributor_config_t
{
    dtnetportal_handle netportal_handle;
    dtbufferqueue_handle bufferqueue_handle;
} dtnetprofile_distributor_config_t;

typedef struct dtnetprofile_distributor_t
{
    dtnetprofile_distributor_config_t config;

} dtnetprofile_distributor_t;

// -----------------------------------------------------------------------------

extern dterr_t*
dtnetprofile_distributor_init(dtnetprofile_distributor_t* self);

extern dterr_t*
dtnetprofile_distributor_configure( //
  dtnetprofile_distributor_t* self,
  dtnetprofile_distributor_config_t* config);

extern dterr_t*
dtnetprofile_distributor_activate(dtnetprofile_distributor_t* self);

extern dterr_t*
dtnetprofile_distributor_collect( //
  dtnetprofile_distributor_t* self,
  dtnetprofile_t* netprofile);

extern dterr_t*
dtnetprofile_distributor_distribute( //
  dtnetprofile_distributor_t* self,
  dtnetprofile_t* netprofile);

extern void
dtnetprofile_distributor_dispose(dtnetprofile_distributor_t* self);