#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtcore/dtlog.h>
#include <dtcore/dtobject.h>
#include <dtcore/dtpackable.h>

#include <dtcore/dtchunker.h>

#include <dtmc_base/dtnetportal.h>

#include <dtmc_base/dtbuffer_distributor.h>

#define TAG "dtbuffer_distributor"

#define dtlog_debug(TAG, ...)

// forward the declaration to the method which netportal calls when it gets a new message
static dterr_t* netportal_receive_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS);

// -----------------------------------------------------------------------------

dterr_t*
dtbuffer_distributor_init(dtbuffer_distributor_t* self)
{
    dterr_t* dterr = NULL;

    if (self == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "self is NULL");
        goto cleanup;
    }

    memset(self, 0, sizeof(*self));

cleanup:

    if (dterr)
        dtbuffer_distributor_dispose(self);

    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtbuffer_distributor_configure( //
  dtbuffer_distributor_t* self,
  dtbuffer_distributor_config_t* config)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(config);
    DTERR_ASSERT_NOT_NULL(config->netportal_handle);
    DTERR_ASSERT_NOT_NULL(config->topic);

    self->config = *config;

    if (self->config.netportal_payload_chunk_size == 0)
    {
        self->config.netportal_payload_chunk_size = 240; // default chunk size
    }

    DTERR_C(dtchunker_init(&self->chunker, self->config.netportal_payload_chunk_size));

    goto cleanup;

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code,
          DTERR_LOC,
          dterr,
          "unable to configure dtbuffer_distributor for topic \"%s\"",
          config && config->topic ? config->topic : "unknown");
    }

    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtbuffer_distributor_subscribe(dtbuffer_distributor_t* self)
{
    dterr_t* dterr = NULL;
    DTERR_ASSERT_NOT_NULL(self);
    DTERR_ASSERT_NOT_NULL(self->config.bufferqueue_handle);

    // self-subscribe to simple handler which puts into our bufferqueue
    DTERR_C(dtnetportal_subscribe(self->config.netportal_handle, self->config.topic, self, netportal_receive_callback));

    dtlog_debug(TAG, "subscribed for topic \"%s\"", self->config.topic);

cleanup:
    if (dterr != NULL)
    {
        dterr =
          dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "unable to subscribe to topic \"%s\"", self ? self->config.topic : "unknown");
    }
    return dterr;
}

// -----------------------------------------------------------------------------
dterr_t*
dtbuffer_distributor_collect( //
  dtbuffer_distributor_t* self,
  dtbuffer_t** out_buffer,
  int32_t timeout_ms)
{
    dterr_t* dterr = NULL;
    dtbuffer_t* chunk_buffer = NULL;
    dtbuffer_t* network_buffer = NULL;

    if (self == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "self is NULL");
        goto cleanup;
    }

    if (self->config.bufferqueue_handle == NULL)
    {
        dterr = dterr_new(DTERR_ARGUMENT_NULL, DTERR_LOC, NULL, "bufferqueue_handle was not provided in config");
        goto cleanup;
    }

    // -----------------------------------------------------------------
    // 1. Reassemble chunks into a final buffer

    DTERR_C(dtchunker_reset(&self->chunker));
    int chunk_count = 0;
    while (network_buffer == NULL)
    {
        // Receive (blocking) one chunk into a newly allocated buffer.
        DTERR_C(dtbufferqueue_get(self->config.bufferqueue_handle, &chunk_buffer, DTTIMEOUT_FOREVER, NULL));

        // let chunker import into reassembled full buffer
        DTERR_C(dtchunker_import(&self->chunker, chunk_buffer, &network_buffer));

        // always free the chunk buffer allocated by netportal
        dtbuffer_dispose(chunk_buffer);
        chunk_buffer = NULL;
        chunk_count++;
    }

    dtlog_debug(TAG,
      "received buffer of %" PRId32 " bytes that came in %d chunks from topic \"%s\"",
      network_buffer->length,
      chunk_count,
      self->config.topic);

cleanup:

    if (dterr != NULL)
    {
        dtbuffer_dispose(network_buffer);
        network_buffer = NULL;
        dterr = dterr_new(DTERR_FAIL, DTERR_LOC, dterr, "failed to collect buffer");
    }

    // caller takes ownership of the returned buffer
    *out_buffer = network_buffer;

    return dterr;
}

// -----------------------------------------------------------------------------

dterr_t*
dtbuffer_distributor_distribute( //
  dtbuffer_distributor_t* self,
  dtbuffer_t* in_buffer)
{
    dterr_t* dterr = NULL;

    dtlog_debug(TAG,
      "distributing buffer total size %" PRId32 " bytes in chunks of %d bytes on topic \"%s\"",
      in_buffer->length,
      self->chunker.max_chunk_size,
      self->config.topic);

    // reset chunker before commencing the sequence
    DTERR_C(dtchunker_reset(&self->chunker));
    while (true)
    {
        dtbuffer_t* chunk_buffer = NULL;

        // get a buffer that contains the next chunk
        DTERR_C(dtchunker_export(&self->chunker, in_buffer, &chunk_buffer));

        // no more chunks to send?
        if (chunk_buffer == NULL)
            break;

        DTERR_C(dtnetportal_publish(self->config.netportal_handle, self->config.topic, chunk_buffer));
    }

cleanup:
    if (dterr != NULL)
    {
        dterr = dterr_new(dterr->error_code, DTERR_LOC, dterr, "unable to distribute buffer");
    }
    return dterr;
}

// -----------------------------------------------------------------------------
void
dtbuffer_distributor_dispose(dtbuffer_distributor_t* self)
{
    if (self == NULL)
        return;

    dtchunker_dispose(&self->chunker);

    memset(self, 0, sizeof(*self));
}

// -----------------------------------------------------------------------------
// callback by netportal when a new message arrives on our topic
static dterr_t*
netportal_receive_callback(DTNETPORTAL_RECEIVE_CALLBACK_ARGS)
{
    dtbuffer_distributor_t* self = (dtbuffer_distributor_t*)receiver_self;
    dterr_t* dterr = NULL;

    DTERR_C(dtbufferqueue_put(self->config.bufferqueue_handle, buffer, DTTIMEOUT_NOWAIT, NULL));

cleanup:

    return dterr;
}