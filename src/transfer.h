#ifndef MACADTPRO_TRANSFER_H
#define MACADTPRO_TRANSFER_H
#include "image.h"
#include "serial.h"
#include "wide.h"
typedef struct {
    DiskImage image;
    unsigned char data[20480];
    unsigned char packet[40970];
    unsigned short packetLength, block, chunk, preferred;
    short state, retries;
    unsigned long lastActivity;
    char retryReason[64];
    char message[128];
} ImageTransfer;
OSErr transferCancel(ImageTransfer *transfer);
OSErr transferCommand(ImageTransfer *transfer, SerialConnection *serial,
                      const ImageFolder *folder, const WideParser *request, Boolean *handled);
OSErr transferPoll(ImageTransfer *transfer, SerialConnection *serial);
OSErr transferRetry(ImageTransfer *transfer);
#endif
