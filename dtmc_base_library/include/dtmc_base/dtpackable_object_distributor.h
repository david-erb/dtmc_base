/*
 * dtpackable_object_distributor -- Distributes packable objects over a netportal topic.
 *
 * Adapts the dtpackable serialization interface to topic-based network
 * transport via a dtnetportal and a dtbufferqueue. Large objects are
 * transparently chunked using dtchunker before transmission. The collect
 * operation blocks until a complete object arrives or a timeout expires,
 * decoupling the producer's serialization rate from the consumer's
 * processing rate.
 *
 * cdox v1.0.2
 */
#pragma once

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpackable.h>

#include <dtcore/dtchunker.h>
#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtnetportal.h>

typedef struct dtpackable_object_distributor_config_t
{
    const char* topic;
    dtnetportal_handle netportal_handle;
    dtbufferqueue_handle bufferqueue_handle;
} dtpackable_object_distributor_config_t;

typedef struct dtpackable_object_distributor_t
{
    dtpackable_object_distributor_config_t config;
    dtchunker_t chunker; // used for chunking large messages
} dtpackable_object_distributor_t;

// -----------------------------------------------------------------------------

extern dterr_t*
dtpackable_object_distributor_init(dtpackable_object_distributor_t* self);

extern dterr_t*
dtpackable_object_distributor_configure( //
  dtpackable_object_distributor_t* self,
  dtpackable_object_distributor_config_t* config);

extern dterr_t*
dtpackable_object_distributor_activate( //
  dtpackable_object_distributor_t* self);

extern dterr_t*
dtpackable_object_distributor_subscribe( //
  dtpackable_object_distributor_t* self);

extern dterr_t*
dtpackable_object_distributor_collect( //
  dtpackable_object_distributor_t* self,
  dtobject_handle* out_object_handle,
  int32_t timeout_ms);

extern dterr_t*
dtpackable_object_distributor_distribute( //
  dtpackable_object_distributor_t* self,
  dtobject_handle object_handle);

extern void
dtpackable_object_distributor_dispose(dtpackable_object_distributor_t* self);