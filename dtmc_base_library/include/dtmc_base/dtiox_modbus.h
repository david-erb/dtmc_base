/*
 * dtiox_modbus -- Wire-level Modbus holding-register protocol for blob exchange.
 *
 * Defines the register map, command codes, and status values for a
 * master/slave blob-transfer protocol layered on top of Modbus TCP/RTU
 * holding registers. Masters write blobs by issuing PUT_BLOB and poll for
 * slave-originated data with GIVE_ME_ANY_DATA. A helper macro converts
 * byte counts to the minimum register count needed to carry the payload.
 *
 * cdox v1.0.2
 */
#pragma once

#include <stdint.h>

//
// Wire-level "application" protocol carried over Modbus holding registers.
// This header is shared by master and slave implementations.
//

// Default Modbus/TCP port.
#define DTIOX_MODBUS_TCP_DEFAULT_PORT 502

// Conservative max blob size that fits in a single Read/Write Multiple Registers.
// 120 registers * 2 bytes = 240 bytes payload, plus a couple of header registers.
#define DTIOX_MODBUS_MAX_BLOB_BYTES 240
#define DTIOX_MODBUS_MAX_BLOB_REGS (DTIOX_MODBUS_MAX_BLOB_BYTES / 2)

// Register map (holding registers).
// Master -> Slave request area
#define DTIOX_MODBUS_REG_M2S_CMD 0  // u16 command
#define DTIOX_MODBUS_REG_M2S_LEN 1  // u16 payload length in bytes
#define DTIOX_MODBUS_REG_M2S_DATA 2 // data starts here (packed bytes)

// Slave -> Master response area
#define DTIOX_MODBUS_REG_S2M_STATUS 200 // u16 status
#define DTIOX_MODBUS_REG_S2M_LEN 201    // u16 payload length in bytes
#define DTIOX_MODBUS_REG_S2M_DATA 202   // data starts here (packed bytes)

// Commands written by master
typedef enum dtiox_modbus_cmd_e
{
    DTIOX_MODBUS_CMD_NONE = 0,
    DTIOX_MODBUS_CMD_PUT_BLOB = 1,         // master writes blob -> slave consumes, no app response
    DTIOX_MODBUS_CMD_GIVE_ME_ANY_DATA = 2, // poll: "do you have data for me?"
} dtiox_modbus_cmd_t;

// Status values written by slave
typedef enum dtiox_modbus_status_e
{
    DTIOX_MODBUS_STATUS_NO_DATA = 0,
    DTIOX_MODBUS_STATUS_HAS_DATA = 1,
} dtiox_modbus_status_t;

// Helper: how many holding registers needed to carry N bytes of blob (packed 2 bytes per reg).
#define DTIOX_MODBUS_BLOB_TO_REGS(byte_len) (((byte_len) + 1) / 2)
