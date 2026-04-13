/*
 * dtiox -- Byte-stream I/O exchange facade for UART, CAN, and socket backends.
 *
 * Defines a vtable interface for non-blocking asynchronous byte I/O: attach
 * and detach hardware resources, enable or disable the channel, read received
 * bytes from an internal buffer, write bytes for asynchronous transmission,
 * and bind a semaphore for RX-ready notification. A global model-number
 * registry supports a wide range of backends (ESP-IDF UART/CAN, Zephyr,
 * PicoSDK, Linux TTY/CAN/TCP, Modbus RTU and TCP) without changing
 * surrounding transport code.
 *
 * cdox v1.0.2
 */
#pragma once
// UART facade

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <dtcore/dterr.h>

#include <dtmc_base/dtsemaphore.h>

#define DTIOX_IFACE_NAME "dtiox_iface"
// ------------------------------------------------------------------------------
// The dtiox facade.
// An IOX (input/output exchange) device provides a generic interface for
// reading and writing data streams, such as serial ports, network sockets,
// or other byte-oriented I/O channels. The dtiox interface abstracts the
// underlying implementation details, allowing users to interact with various
// I/O devices in a consistent manner.

// dtiox_handle: Opaque handle representing an IOX device instance.
// dtiox_attach: Attaches the IOX device to its underlying hardware or resource.
// dtiox_detach: Detaches the IOX device from its underlying hardware or resource.
// dtiox_enable: Enables or disables the IOX device for reading and writing operations.  Clears internal buffers and counters.
//     Stops interrupts when disabled.
// dtiox_read: Non-blocking. Copies previously received data from the IOX device into a provided buffer.  Actual data reception
//     of bytes off the wire happens asynchronously.  When incoming bytes have overflowed internal buffers, an overflow error is
//     reported on the next read().  Posts the semaphore when new data is available.
// dtiox_write: Non-blocking. Absorbs as much as possible to internal buffer. Caller can reuse the buffer upon return. Actual
//     placing of bytes on the wire happens "soon" asynchronously. Not an error if the entirety of the caller's buffer cannot be
//     accepted.
// dtiox_set_rx_semaphore: Sets a semaphore that will be signaled when new data is available to read.
// dtiox_concat_format: Concatenates a formatted string representation of the IOX device's configuration and status information.
// dtiox_dispose: Disposes of the IOX device instance, releasing any allocated resources.

// -----------------------------------------------------------------------------
// Opaque handle
struct dtiox_handle_t;
typedef struct dtiox_handle_t* dtiox_handle;

// -----------------------------------------------------------------------------

#define DTIOX_ATTACH_ARGS
#define DTIOX_ATTACH_PARAMS
#define DTIOX_DETACH_ARGS
#define DTIOX_DETACH_PARAMS
#define DTIOX_ENABLE_ARGS , bool enabled
#define DTIOX_ENABLE_PARAMS , enabled
#define DTIOX_READ_ARGS , uint8_t *buf, int32_t buf_len, int32_t *out_read
#define DTIOX_READ_PARAMS , buf, buf_len, out_read
#define DTIOX_WRITE_ARGS , const uint8_t *buf, int32_t len, int32_t *out_written
#define DTIOX_WRITE_PARAMS , buf, len, out_written
#define DTIOX_SET_RX_SEMAPHORE_ARGS , dtsemaphore_handle rx_semaphore
#define DTIOX_SET_RX_SEMAPHORE_PARAMS , rx_semaphore
#define DTIOX_CONCAT_FORMAT_ARGS , char *in_str, char *separator, char **out_str
#define DTIOX_CONCAT_FORMAT_PARAMS , in_str, separator, out_str

// -----------------------------------------------------------------------------
// Vtable

typedef dterr_t* (*dtiox_attach_fn)(void* self DTIOX_ATTACH_ARGS);
typedef dterr_t* (*dtiox_detach_fn)(void* self DTIOX_DETACH_ARGS);
typedef dterr_t* (*dtiox_enable_fn)(void* self DTIOX_ENABLE_ARGS);
typedef dterr_t* (*dtiox_read_fn)(void* self DTIOX_READ_ARGS);
typedef dterr_t* (*dtiox_write_fn)(void* self DTIOX_WRITE_ARGS);
typedef dterr_t* (*dtiox_set_rx_semaphore_fn)(void* self DTIOX_SET_RX_SEMAPHORE_ARGS);
typedef dterr_t* (*dtiox_concat_format_fn)(void* self DTIOX_CONCAT_FORMAT_ARGS);
typedef void (*dtiox_dispose_fn)(void* self);

typedef struct dtiox_vt_t
{
    dtiox_attach_fn attach;
    dtiox_detach_fn detach;
    dtiox_enable_fn enable;
    dtiox_read_fn read;
    dtiox_write_fn write;
    dtiox_set_rx_semaphore_fn set_rx_semaphore;
    dtiox_concat_format_fn concat_format;
    dtiox_dispose_fn dispose;
} dtiox_vt_t;

// Registry (same model-numbered registry pattern as dtgpiopin)
extern dterr_t*
dtiox_set_vtable(int32_t model_number, dtiox_vt_t* vtable);
extern dterr_t*
dtiox_get_vtable(int32_t model_number, dtiox_vt_t** vtable);

// declaration dispatcher or implementation
#define DTIOX_DECLARE_API_EX(NAME, T)                                                                                          \
    extern dterr_t* NAME##_attach(NAME##T self DTIOX_ATTACH_ARGS);                                                             \
    extern dterr_t* NAME##_detach(NAME##T self DTIOX_DETACH_ARGS);                                                             \
    extern dterr_t* NAME##_enable(NAME##T self DTIOX_ENABLE_ARGS);                                                             \
    extern dterr_t* NAME##_read(NAME##T self DTIOX_READ_ARGS);                                                                 \
    extern dterr_t* NAME##_write(NAME##T self DTIOX_WRITE_ARGS);                                                               \
    extern dterr_t* NAME##_set_rx_semaphore(NAME##T self DTIOX_SET_RX_SEMAPHORE_ARGS);                                         \
    extern dterr_t* NAME##_concat_format(NAME##T self DTIOX_CONCAT_FORMAT_ARGS);                                               \
    extern void NAME##_dispose(NAME##T self);

// declare dispatcher
DTIOX_DECLARE_API_EX(dtiox, _handle)

// declare implementation (put this in its .h file)
#define DTIOX_DECLARE_API(NAME) DTIOX_DECLARE_API_EX(NAME, _t*)

// Helper for concrete objects to embed as first field
#define DTIOX_COMMON_MEMBERS int32_t model_number;

// Helper to build a vtable variable from backend symbol prefix
#define DTIOX_INIT_VTABLE(NAME)                                                                                                \
    static dtiox_vt_t NAME##_vt = {                                                                                            \
        .attach = (dtiox_attach_fn)NAME##_attach,                                                                              \
        .detach = (dtiox_detach_fn)NAME##_detach,                                                                              \
        .enable = (dtiox_enable_fn)NAME##_enable,                                                                              \
        .read = (dtiox_read_fn)NAME##_read,                                                                                    \
        .write = (dtiox_write_fn)NAME##_write,                                                                                 \
        .set_rx_semaphore = (dtiox_set_rx_semaphore_fn)NAME##_set_rx_semaphore,                                                \
        .concat_format = (dtiox_concat_format_fn)NAME##_concat_format,                                                         \
        .dispose = (dtiox_dispose_fn)NAME##_dispose,                                                                           \
    }
