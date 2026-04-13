/*
 * dtframer_simple -- Lightweight topic-version-payload framer without CRC or compression.
 *
 * A concrete dtframer implementation that encodes topic, protocol version,
 * and payload into a framed byte stream and decodes it back. A framing mode
 * hint (STREAM vs. BUS) tunes maximum topic and payload sizes for UART-style
 * streams or CAN-style short messages. Suited for environments where the
 * transport layer already provides error detection.
 *
 * cdox v1.0.2
 */
#pragma once
// See markdown documentation at the end of this file.

// Simple implementation of dtframer interface.
// Encodes topic, version and payload into a framed byte stream and decodes it back.
// This variant deliberately omits CRC and compression.

#include <stdbool.h>
#include <stdint.h>

#include <dtcore/dterr.h>
#include <dtmc_base/dtframer.h>

// Framing mode hint — currently both modes use the same basic wire format,
// but you can tune maximum sizes differently for typical UART vs CAN usage.
typedef enum
{
    DTFRAMER_SIMPLE_FRAMING_MODE_STREAM = 0, // e.g., UART-style long streams
    DTFRAMER_SIMPLE_FRAMING_MODE_BUS = 1,    // e.g., CAN-style smaller messages
} dtframer_simple_framing_mode_t;

typedef struct
{
    // Version byte to place into each frame, and to expect when decoding.
    // If zero, encoding uses version 1 and decoding accepts any version.
    uint8_t protocol_version;

    // Maximum accepted topic length in bytes (without terminator).
    // If zero, a default will be chosen in init (e.g., 64).
    int32_t maximum_topic_length;

    // Maximum accepted payload length in bytes.
    // If zero, a default will be chosen in init (e.g., 1024 or similar).
    int32_t maximum_payload_length;

    // High-level “profile” for tuning (UART style vs CAN style).
    dtframer_simple_framing_mode_t framing_mode;
} dtframer_simple_config_t;

// forward declare the concrete type
typedef struct dtframer_simple_t dtframer_simple_t;

// Create a heap-allocated instance of the object.
extern dterr_t*
dtframer_simple_create(dtframer_simple_t** self_ptr);

// Initialize a stack- or statically-allocated instance of the object.
extern dterr_t*
dtframer_simple_init(dtframer_simple_t* self);

// Supply configuration settings before first use.
extern dterr_t*
dtframer_simple_configure(dtframer_simple_t* self, dtframer_simple_config_t* config);

// --------------------------------------------------------------------------------------

// Expose the dtframer API for this concrete type
DTFRAMER_DECLARE_API(dtframer_simple)

#if MARKDOWN_DOCUMENTATION
// clang-format off
// --8<-- [start:markdown-documentation]


// --8<-- [end:markdown-documentation]
// clang-format on
#endif
