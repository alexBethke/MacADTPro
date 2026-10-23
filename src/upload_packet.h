#ifndef MACADTPRO_UPLOAD_PACKET_H
#define MACADTPRO_UPLOAD_PACKET_H

enum { UPLOAD_PACKET_NONE, UPLOAD_PACKET_COMPLETE,
       UPLOAD_PACKET_BAD, UPLOAD_PACKET_CANCEL };

typedef struct {
    unsigned char data[20480];
    unsigned char header[4];
    unsigned short length, block, output, receivedCrc;
    unsigned char state, headerIndex, previous, crcIndex;
} UploadPacket;

void uploadPacketReset(UploadPacket *packet);
int uploadPacketFeed(UploadPacket *packet, unsigned char byte);

#endif
