/*
 * dtbuffer_distributor -- Bidirectional dtbuffer relay over a netportal topic.
 *
 * Connects a dtbufferqueue to a dtnetportal subscription on a named topic.
 * Incoming messages are chunked for transmission via dtchunker and placed
 * into the queue; outbound calls pull from the queue and publish. Separating
 * queuing and transport concerns lets producers and consumers run on
 * different tasks without coupling them to a specific network backend.
 *
 * cdox v1.0.2
 */
#pragma once

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>

#include <dtcore/dtchunker.h>
#include <dtmc_base/dtbufferqueue.h>
#include <dtmc_base/dtnetportal.h>

typedef struct dtbuffer_distributor_config_t
{
    const char* topic;
    dtnetportal_handle netportal_handle;
    int netportal_payload_chunk_size; // size of chunks to use when sending messages
    dtbufferqueue_handle bufferqueue_handle;
} dtbuffer_distributor_config_t;

typedef struct dtbuffer_distributor_t
{
    dtbuffer_distributor_config_t config;
    dtchunker_t chunker; // used for chunking large messages
} dtbuffer_distributor_t;

// -----------------------------------------------------------------------------

extern dterr_t*
dtbuffer_distributor_init(dtbuffer_distributor_t* self);

extern dterr_t*
dtbuffer_distributor_configure( //
  dtbuffer_distributor_t* self,
  dtbuffer_distributor_config_t* config);

extern dterr_t*
dtbuffer_distributor_subscribe(dtbuffer_distributor_t* self);

extern dterr_t*
dtbuffer_distributor_subscribe(dtbuffer_distributor_t* self);

extern dterr_t*
dtbuffer_distributor_collect( //
  dtbuffer_distributor_t* self,
  dtbuffer_t** out_buffer,
  int32_t timeout_ms);

extern dterr_t*
dtbuffer_distributor_distribute( //
  dtbuffer_distributor_t* self,
  dtbuffer_t* in_buffer);

extern void
dtbuffer_distributor_dispose(dtbuffer_distributor_t* self);