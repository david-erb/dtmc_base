#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// -----------------------------------------------------------------------------
void
dtmodbus_helpers_pack_bytes_to_regs(const uint8_t* bytes, int32_t len, uint16_t* regs_out)
{
    // Big-endian within register: [byte0]<<8 | [byte1]
    int32_t r = 0;
    for (int32_t i = 0; i < len; i += 2, r++)
    {
        uint16_t hi = bytes[i];
        uint16_t lo = (i + 1 < len) ? bytes[i + 1] : 0;
        regs_out[r] = (uint16_t)((hi << 8) | lo);
    }
}

void
dtmodbus_helpers_unpack_regs_to_bytes(const uint16_t* regs, int32_t byte_len, uint8_t* bytes_out)
{
    int32_t r = 0;
    for (int32_t i = 0; i < byte_len; i += 2, r++)
    {
        uint16_t v = regs[r];
        bytes_out[i] = (uint8_t)((v >> 8) & 0xFF);
        if (i + 1 < byte_len)
            bytes_out[i + 1] = (uint8_t)(v & 0xFF);
    }
}
