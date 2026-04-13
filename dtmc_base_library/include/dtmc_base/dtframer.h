/*
 * dtframer -- Message framing facade with model-numbered vtable dispatch.
 *
 * Defines a uniform interface for encoding a topic-and-payload pair into a
 * byte stream and for decoding an incoming byte stream back into topic and
 * payload, delivering complete messages via a callback. The opaque handle
 * and global vtable registry allow framing algorithms (simple, CRC,
 * compressed, encrypted) to be swapped at initialization without changing
 * surrounding transport or application code.
 *
 * cdox v1.0.2
 */
#pragma once

// dtframer
// Facade for turrning a message into a byte stream and vice versa.
// Mini-guide
// - The handle is opaque and must have been created by an implementation-specific method.
// - Implementations are typically pure software and not expected to interact with hardware or system resources directly.
// - `encode` encodes a topic+payload into a byte stream stored in a new or pre-allocated buffer.
// - 'decode' decodes serial chunks of a byte stream into a topic+payload which is then delivered via a callback.
// - Call `dispose` exactly once per handle to free its resources.
// - All functions return `NULL` on success, or a `dterr_t*` describing an error.

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>

// -----------------------------------------------------------------------------
// Opaque handle

struct dtframer_handle_t;
typedef struct dtframer_handle_t* dtframer_handle;

// callback with completed message
#define DTFRAMER_MESSAGE_CALLBACK_ARGS void *callback_context, const char *topic, dtbuffer_t *payload
#define DTFRAMER_MESSAGE_CALLBACK_PARAMS callback_context, topic, payload
typedef dterr_t* (*dtframer_message_callback_t)(DTFRAMER_MESSAGE_CALLBACK_ARGS);

// -----------------------------------------------------------------------------
// Argument helpers

#define DTFRAMER_SET_MESSAGE_CALLBACK_ARGS , dtframer_message_callback_t callback_function, void *callback_context
#define DTFRAMER_SET_MESSAGE_CALLBACK_PARAMS , callback_function, callback_context
#define DTFRAMER_ENCODE_ARGS , const char *topic, dtbuffer_t *payload, dtbuffer_t *encoded_buffer, int32_t *encoded_length
#define DTFRAMER_ENCODE_PARAMS , topic, payload, encoded_buffer, encoded_length
#define DTFRAMER_DECODE_ARGS , uint8_t *buffer, int32_t length
#define DTFRAMER_DECODE_PARAMS , buffer, length

// -----------------------------------------------------------------------------
// Delegate (vtable) signatures

typedef dterr_t* (*dtframer_set_message_callback_fn)(void* self DTFRAMER_SET_MESSAGE_CALLBACK_ARGS);
typedef dterr_t* (*dtframer_encode_fn)(void* self DTFRAMER_ENCODE_ARGS);
typedef dterr_t* (*dtframer_decode_fn)(void* self DTFRAMER_DECODE_ARGS);
typedef void (*dtframer_dispose_fn)(void* self);

// -----------------------------------------------------------------------------
// Virtual table type

typedef struct dtframer_vt_t
{
    dtframer_set_message_callback_fn set_message_callback;
    dtframer_encode_fn encode;
    dtframer_decode_fn decode;
    dtframer_dispose_fn dispose;
} dtframer_vt_t;

// -----------------------------------------------------------------------------
// Vtable registry

extern dterr_t*
dtframer_set_vtable(int32_t model_number, dtframer_vt_t* vtable);

extern dterr_t*
dtframer_get_vtable(int32_t model_number, dtframer_vt_t** vtable);

// -----------------------------------------------------------------------------
// Declaration helpers

#define DTFRAMER_DECLARE_API_EX(NAME, T)                                                                                       \
    extern dterr_t* NAME##_set_message_callback(NAME##T self DTFRAMER_SET_MESSAGE_CALLBACK_ARGS);                              \
    extern dterr_t* NAME##_encode(NAME##T self DTFRAMER_ENCODE_ARGS);                                                          \
    extern dterr_t* NAME##_decode(NAME##T self DTFRAMER_DECODE_ARGS);                                                          \
    extern void NAME##_dispose(NAME##T self);

DTFRAMER_DECLARE_API_EX(dtframer, _handle)
#define DTFRAMER_DECLARE_API(NAME) DTFRAMER_DECLARE_API_EX(NAME, _t*)

#define DTFRAMER_INIT_VTABLE(NAME)                                                                                             \
    static dtframer_vt_t NAME##_vt = {                                                                                         \
        .set_message_callback = (dtframer_set_message_callback_fn)NAME##_set_message_callback,                                 \
        .encode = (dtframer_encode_fn)NAME##_encode,                                                                           \
        .decode = (dtframer_decode_fn)NAME##_decode,                                                                           \
        .dispose = (dtframer_dispose_fn)NAME##_dispose,                                                                        \
    };

// -----------------------------------------------------------------------------
// Common struct prefix

#define DTFRAMER_COMMON_MEMBERS int32_t model_number;

// -----------------------------------------------------------------------------
#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
