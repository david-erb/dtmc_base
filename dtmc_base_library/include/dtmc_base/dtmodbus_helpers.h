/*
 * dtmodbus_helpers -- Byte-to-register packing utilities for Modbus payloads.
 *
 * Provides two functions that convert between a byte array and an array of
 * 16-bit Modbus holding registers, packing two bytes per register in network
 * byte order. Used by both master and slave implementations to prepare and
 * extract blob payloads exchanged over the dtiox_modbus wire protocol.
 *
 * cdox v1.0.2
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
void
dtmodbus_helpers_pack_bytes_to_regs(const uint8_t* bytes, int32_t len, uint16_t* regs_out);

void
dtmodbus_helpers_unpack_regs_to_bytes(const uint16_t* regs, int32_t byte_len, uint8_t* bytes_out);