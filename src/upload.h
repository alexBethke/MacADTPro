#ifndef MACADTPRO_UPLOAD_H
#define MACADTPRO_UPLOAD_H

#include "image_folder.h"
#include "serial.h"
#include "upload_packet.h"
#include "wide.h"
#include <Files.h>

typedef struct {
    UploadPacket packet;
    FSSpec temporary, destination;
    short ref;
    unsigned short blocks, done;
    Boolean active, awaitingFinal, dosOrder;
    unsigned long lastActivity;
    char imageName[32];
    char message[128];
} ImageUpload;

OSErr uploadCancel(ImageUpload *upload);
OSErr uploadCommand(ImageUpload *upload, SerialConnection *serial,
                    const ImageFolder *folder, const WideParser *command,
                    Boolean *handled);
OSErr uploadFeed(ImageUpload *upload, SerialConnection *serial,
                 unsigned char byte, Boolean *handled);

#endif
