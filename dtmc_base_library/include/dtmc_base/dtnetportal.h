/*
 * dtnetportal -- Publish/subscribe network portal facade with vtable dispatch.
 *
 * Defines a transport-agnostic interface for topic-based messaging: activate
 * the connection, subscribe to a named topic with a receive callback, publish
 * a dtbuffer payload, and query connection metadata. A global model-number
 * registry supports over a dozen backends (MQTT, CoAP, BLE, TCP, UDP,
 * Modbus, CAN, shared memory, stream) without changing the application layer.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdint.h>

#include <dtcore/dtbuffer.h>
#include <dtcore/dterr.h>
#include <dtmc_base/dttasker.h>

// opaque handle for dispatch calls
struct dtnetportal_handle_t;
typedef struct dtnetportal_handle_t* dtnetportal_handle;

typedef struct dtnetportal_info_t
{
    // origin has scheme+host+port, e.g. "coap://localhost:5683"
    // or "coaps://localhost:5684" if TLS is used
    char listening_origin[128];
    const char* flavor;
    const char* version;
    dterr_t* internal_task_dterr;
} dtnetportal_info_t;

#define DTNETPORTAL_IFACE_NAME "dtnetportal_iface"

#define DTNETPORTAL_MAX_TOPIC_LENGTH (64)

#define DTNETPORTAL_RECEIVE_CALLBACK_ARGS void *receiver_self, const char *topic, dtbuffer_t *buffer
#define DTNETPORTAL_RECEIVE_CALLBACK_PARAMS receiver_self, topic, buffer
typedef dterr_t* (*dtnetportal_receive_callback_t)(DTNETPORTAL_RECEIVE_CALLBACK_ARGS);

// arguments

#define DTNETPORTAL_ACTIVATE_ARGS
#define DTNETPORTAL_ACTIVATE_PARAMS
#define DTNETPORTAL_SUBSCRIBE_ARGS , const char *topic, void *recipient_self, dtnetportal_receive_callback_t receive_callback
#define DTNETPORTAL_SUBSCRIBE_PARAMS , topic, recipient_self, receive_callback
#define DTNETPORTAL_PUBLISH_ARGS , const char *topic, dtbuffer_t *buffer
#define DTNETPORTAL_PUBLISH_PARAMS , topic, buffer
#define DTNETPORTAL_GET_INFO_ARGS , dtnetportal_info_t* info
#define DTNETPORTAL_GET_INFO_PARAMS , info

// delegates
typedef dterr_t* (*dtnetportal_activate_fn)(void* self DTNETPORTAL_ACTIVATE_ARGS);
typedef dterr_t* (*dtnetportal_subscribe_fn)(void* self DTNETPORTAL_SUBSCRIBE_ARGS);
typedef dterr_t* (*dtnetportal_publish_fn)(void* self DTNETPORTAL_PUBLISH_ARGS);
typedef dterr_t* (*dtnetportal_get_info_fn)(void* self DTNETPORTAL_GET_INFO_ARGS);
typedef void (*dtnetportal_dispose_fn)(void* self);

// virtual table type
typedef struct dtnetportal_vt_t
{
    dtnetportal_activate_fn activate;
    dtnetportal_subscribe_fn subscribe;
    dtnetportal_publish_fn publish;
    dtnetportal_get_info_fn get_info;
    dtnetportal_dispose_fn dispose;
} dtnetportal_vt_t;

extern dterr_t*
dtnetportal_set_vtable(int32_t model_number, dtnetportal_vt_t* vtable);
extern dterr_t*
dtnetportal_get_vtable(int32_t model_number, dtnetportal_vt_t** vtable);

// declaration dispatcher or implementation
#define DTNETPORTAL_DECLARE_API_EX(NAME, T)                                                                                    \
    extern dterr_t* NAME##_activate(NAME##T self DTNETPORTAL_ACTIVATE_ARGS);                                                   \
    extern dterr_t* NAME##_subscribe(NAME##T self DTNETPORTAL_SUBSCRIBE_ARGS);                                                 \
    extern dterr_t* NAME##_publish(NAME##T self DTNETPORTAL_PUBLISH_ARGS);                                                     \
    extern dterr_t* NAME##_get_info(NAME##T self DTNETPORTAL_GET_INFO_ARGS);                                                   \
    extern void NAME##_dispose(NAME##T self);

// declare dispatcher
DTNETPORTAL_DECLARE_API_EX(dtnetportal, _handle)

// declare implementation (put this in its .h file)
#define DTNETPORTAL_DECLARE_API(NAME) DTNETPORTAL_DECLARE_API_EX(NAME, _t*)

// initialize implementation vtable  (put this in its .c file)
#define DTNETPORTAL_INIT_VTABLE(NAME)                                                                                          \
    static dtnetportal_vt_t NAME##_vt = {                                                                                      \
        .activate = (dtnetportal_activate_fn)NAME##_activate,                                                                  \
        .subscribe = (dtnetportal_subscribe_fn)NAME##_subscribe,                                                               \
        .publish = (dtnetportal_publish_fn)NAME##_publish,                                                                     \
        .get_info = (dtnetportal_get_info_fn)NAME##_get_info,                                                                  \
        .dispose = (dtnetportal_dispose_fn)NAME##_dispose,                                                                     \
    };

// common members expected at the start of all implementation structures
#define DTNETPORTAL_COMMON_MEMBERS int32_t model_number;

#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]

# dtnetportal

Coming soon...

// --8<-- [end:markdown-documentation]
// clang-format on
#endif
