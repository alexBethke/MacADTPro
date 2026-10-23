#include "upload_packet.h"
#include <string.h>

void uploadPacketReset(UploadPacket *packet)
{
    memset(packet, 0, sizeof(*packet));
}

static unsigned short packetCrc(const unsigned char *data, unsigned short length)
{
    unsigned short crc = 0, i;
    unsigned char bit;
    for (i = 0; i < length; ++i) {
        crc ^= (unsigned short)data[i] << 8;
        for (bit = 0; bit < 8; ++bit)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

int uploadPacketFeed(UploadPacket *packet, unsigned char byte)
{
    if (!packet->state) {
        if (byte != 0xc1) return UPLOAD_PACKET_BAD;
        packet->state = 1;
        packet->headerIndex = 0;
        return UPLOAD_PACKET_NONE;
    }
    if (packet->state == 1) {
        packet->header[packet->headerIndex++] = byte;
        if (packet->headerIndex < 4) return UPLOAD_PACKET_NONE;
        packet->length = packet->header[0] | ((unsigned short)packet->header[1] << 8);
        if ((unsigned char)(0xc1 ^ packet->header[0] ^ packet->header[1] ^
                            packet->header[2]) != packet->header[3]) {
            uploadPacketReset(packet);
            return UPLOAD_PACKET_BAD;
        }
        if (packet->header[2] == 0xd8 && packet->length == 0) {
            uploadPacketReset(packet);
            return UPLOAD_PACKET_CANCEL;
        }
        if (packet->header[2] != 0xd3 || !packet->length ||
            packet->length > sizeof(packet->data) || packet->length % 512) {
            uploadPacketReset(packet);
            return UPLOAD_PACKET_BAD;
        }
        packet->output = 0;
        packet->previous = 0;
        packet->state = 2;
        return UPLOAD_PACKET_NONE;
    }
    if (packet->state == 2) {
        packet->block = byte;
        packet->state = 3;
        return UPLOAD_PACKET_NONE;
    }
    if (packet->state == 3) {
        packet->block |= (unsigned short)byte << 8;
        packet->state = 4;
        return UPLOAD_PACKET_NONE;
    }
    if (packet->state == 4) {
        if (byte) {
            packet->previous += byte;
            packet->data[packet->output++] = packet->previous;
            if (packet->output == packet->length) packet->state = 6;
        } else packet->state = 5;
        return UPLOAD_PACKET_NONE;
    }
    if (packet->state == 5) {
        unsigned short end = (packet->output & ~255U) + byte;
        if (end <= packet->output) end += 256;
        if (end > packet->length) {
            uploadPacketReset(packet);
            return UPLOAD_PACKET_BAD;
        }
        while (packet->output < end)
            packet->data[packet->output++] = packet->previous;
        packet->state = packet->output == packet->length ? 6 : 4;
        return UPLOAD_PACKET_NONE;
    }
    if (packet->state == 6) {
        packet->receivedCrc = byte;
        packet->state = 7;
        return UPLOAD_PACKET_NONE;
    }
    packet->receivedCrc |= (unsigned short)byte << 8;
    packet->state = 0;
    return packet->receivedCrc == packetCrc(packet->data, packet->length)
        ? UPLOAD_PACKET_COMPLETE : UPLOAD_PACKET_BAD;
}
