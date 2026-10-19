/* SPDX-License-Identifier: GPL-2.0-or-later
 * Protocol implementation adapted from ADTPro CommsThread.java,
 * Copyright (C) 2006-2020 David Schmidt.
 * MacADTPro C implementation, 2026. See LICENSE.
 */
#include "wide.h"
#include <string.h>

void wideReset(WideParser *p) { memset(p, 0, sizeof(*p)); }

/* ADTPro 2.1.0 Wide: C1, length LE16, command, header XOR;
   nonempty payloads are followed by a separate XOR byte. */
int wideFeed(WideParser *p, unsigned char byte)
{
    if (p->state == 0) {
        if (byte == 0xc1) { p->state = 1; p->index = 0; }
        return byte == 0xd9 ? WIDE_PING : WIDE_NONE;
    }
    if (p->state == 1) {
        p->header[p->index++] = byte;
        if (p->index < 4) return WIDE_NONE;
        p->state = 0;
        if ((unsigned char)(0xc1 ^ p->header[0] ^ p->header[1] ^ p->header[2]) != byte)
            return WIDE_BAD_HEADER;
        p->command = p->header[2];
        p->remaining = p->header[0] | ((unsigned short)p->header[1] << 8);
        p->length = p->remaining;
        p->stored = 0;
        p->check = 0;
        if (!p->remaining) return WIDE_COMMAND;
        p->state = 2;
        return WIDE_NONE;
    }
    if (p->state == 2) {
        if (p->stored < sizeof(p->payload)) p->payload[p->stored++] = byte;
        p->check ^= byte;
        if (!--p->remaining) p->state = 3;
        return WIDE_NONE;
    }
    p->state = 0;
    return byte == p->check ? WIDE_COMMAND : WIDE_BAD_PAYLOAD;
}

/* Upstream sendPacketWide differential RLE and CRC-CCITT (initial value 0). */
unsigned short wideEncode(const unsigned char *data, unsigned short length,
                          unsigned short block, unsigned char *out, unsigned short capacity)
{
    unsigned short i, n = 7, crc = 0;
    static unsigned short crcTable[256];
    static int crcReady;
    unsigned char previous = 0;
    if (!length || length % 256 || capacity < 9) return 0;
    if (!crcReady) {
        unsigned entry, bit;
        for (entry = 0; entry < 256; ++entry) {
            unsigned short value = entry << 8;
            for (bit = 0; bit < 8; ++bit)
                value = (value & 0x8000) ? (value << 1) ^ 0x1021 : value << 1;
            crcTable[entry] = value;
        }
        crcReady = 1;
    }
    out[0] = 0xc1; out[1] = length; out[2] = length >> 8; out[3] = 0xd3;
    out[4] = out[0] ^ out[1] ^ out[2] ^ out[3];
    out[5] = block; out[6] = block >> 8;
    for (i = 0; i < length;) {
        unsigned char difference = data[i] - previous;
        previous = data[i];
        if (n + 4 > capacity) return 0;
        out[n++] = difference;
        if (difference) ++i;
        else {
            unsigned short end = (i & ~255U) + 256;
            do { ++i; } while (i < end && data[i] == previous);
            out[n++] = i & 255;
        }
    }
    for (i = 0; i < length; ++i) {
        crc = (crc << 8) ^ crcTable[((crc >> 8) ^ data[i]) & 255];
    }
    out[n++] = crc; out[n++] = crc >> 8;
    return n;
}
