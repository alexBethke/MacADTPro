/* SPDX-License-Identifier: GPL-2.0-or-later
 * Wide receive flow follows ADTPro 2.1.0 CommsThread.receiveDiskWide.
 */
#include "upload.h"
#include <Devices.h>
#include <Errors.h>
#include <OSUtils.h>
#include <Script.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static OSErr closeUpload(ImageUpload *upload)
{
    OSErr error;
    if (!upload->ref) return noErr;
    error = FSClose(upload->ref);
    if (!error) upload->ref = 0;
    return error;
}

OSErr uploadCancel(ImageUpload *upload)
{
    OSErr error = closeUpload(upload);
    OSErr deleteError = noErr;
    upload->active = false;
    upload->awaitingFinal = false;
    uploadPacketReset(&upload->packet);
    if (upload->temporary.name[0]) {
        deleteError = FSpDelete(&upload->temporary);
        if (deleteError == fnfErr) deleteError = noErr;
        if (!deleteError) upload->temporary.name[0] = 0;
    }
    return error ? error : deleteError;
}

static Boolean hasExtension(const unsigned char *name, const char *extension)
{
    unsigned length = strlen(extension), i;
    if (name[0] < length) return false;
    for (i = 0; i < length; ++i)
        if (toupper(name[name[0] - length + 1 + i]) != extension[i]) return false;
    return true;
}

static OSErr destinationName(const WideParser *command, unsigned short blocks,
                             unsigned char name[32], Boolean *dosOrder)
{
    unsigned end = 0, extensionLength, i;
    const char *extension;
    if (command->length < 4 || command->length > sizeof(command->payload))
        return paramErr;
    while (end + 2 < command->length && command->payload[end]) ++end;
    if (!end || end + 3 != command->length || end > 31) return paramErr;
    for (i = 0; i < end; ++i) {
        unsigned char character = command->payload[i] & 0x7f;
        if (character == ':' || character == '/' || character < 32)
            return paramErr;
    }
    name[0] = end;
    for (i = 0; i < end; ++i) name[i + 1] = command->payload[i] & 0x7f;
    *dosOrder = blocks == 280;
    if (*dosOrder && (hasExtension(name, ".DSK") || hasExtension(name, ".DO")))
        return noErr;
    if (!*dosOrder && hasExtension(name, ".PO")) return noErr;
    extension = *dosOrder ? ".dsk" : ".po";
    extensionLength = strlen(extension);
    if (name[0] + extensionLength > 31) name[0] = 31 - extensionLength;
    memcpy(name + 1 + name[0], extension, extensionLength);
    name[0] += extensionLength;
    return noErr;
}

static OSErr createUpload(ImageUpload *upload, const ImageFolder *folder,
                          const unsigned char *name)
{
    unsigned char temporaryName[32];
    char generated[32];
    short number;
    OSErr error;
    error = FSMakeFSSpec(folder->volume, folder->directory, name,
                         &upload->destination);
    if (error != fnfErr) return error == noErr ? dupFNErr : error;
    for (number = 1; number <= 99; ++number) {
        size_t length;
        sprintf(generated, "MacADTPro Upload %d", number);
        length = strlen(generated);
        temporaryName[0] = length;
        memcpy(temporaryName + 1, generated, length);
        error = FSMakeFSSpec(folder->volume, folder->directory, temporaryName,
                             &upload->temporary);
        if (error == fnfErr) break;
        if (error) return error;
    }
    if (number > 99) return dupFNErr;
    error = FSpCreate(&upload->temporary, FOUR_CHAR_CODE('MADP'),
                      FOUR_CHAR_CODE('dImg'), smSystemScript);
    if (error) return error;
    error = FSpOpenDF(&upload->temporary, fsRdWrPerm, &upload->ref);
    if (!error) error = SetEOF(upload->ref, (long)upload->blocks * 512);
    if (error) {
        uploadCancel(upload);
        return error;
    }
    return noErr;
}

static OSErr writeAt(ImageUpload *upload, const unsigned char *data,
                     long offset, long length)
{
    ParamBlockRec pb;
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = upload->ref;
    pb.ioParam.ioBuffer = (Ptr)data;
    pb.ioParam.ioReqCount = length;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = offset;
    {
        OSErr error = PBWriteSync(&pb);
        if (!error && pb.ioParam.ioActCount != length) error = dskFulErr;
        return error;
    }
}

static OSErr writePacket(ImageUpload *upload)
{
    static const unsigned char first[] = {0, 13, 11, 9, 7, 5, 3, 1};
    static const unsigned char second[] = {14, 12, 10, 8, 6, 4, 2, 15};
    unsigned short blocks = upload->packet.length / 512, i;
    if (!upload->dosOrder)
        return writeAt(upload, upload->packet.data,
                       (long)upload->packet.block * 512, upload->packet.length);
    for (i = 0; i < blocks; ++i) {
        unsigned short block = upload->packet.block + i;
        long track = block / 8;
        unsigned within = block % 8;
        OSErr error = writeAt(upload, upload->packet.data + (long)i * 512,
                              track * 4096 + first[within] * 256, 256);
        if (!error)
            error = writeAt(upload, upload->packet.data + (long)i * 512 + 256,
                            track * 4096 + second[within] * 256, 256);
        if (error) return error;
    }
    return noErr;
}

OSErr uploadCommand(ImageUpload *upload, SerialConnection *serial,
                    const ImageFolder *folder, const WideParser *command,
                    Boolean *handled)
{
    unsigned char name[32], reply;
    OSErr error;
    *handled = false;
    if (command->command == 0xd0 && !upload->active) {
        *handled = true;
        upload->blocks = command->length >= 2
            ? command->payload[command->length - 2] |
              ((unsigned short)command->payload[command->length - 1] << 8) : 0;
        error = !upload->blocks ? paramErr
            : destinationName(command, upload->blocks, name, &upload->dosOrder);
        if (!error) {
            memset(upload, 0, sizeof(*upload));
            upload->blocks = command->payload[command->length - 2] |
                ((unsigned short)command->payload[command->length - 1] << 8);
            error = destinationName(command, upload->blocks, name, &upload->dosOrder);
        }
        if (!error) error = createUpload(upload, folder, name);
        reply = error ? 2 : 0;
        if (!error) {
            upload->active = true;
            upload->lastActivity = TickCount();
            memcpy(upload->imageName, name + 1, name[0]);
            upload->imageName[name[0]] = 0;
            snprintf(upload->message, sizeof(upload->message),
                     "Receiving %s: 0/%u blocks.", upload->imageName, upload->blocks);
        } else {
            uploadCancel(upload);
            strcpy(upload->message, error == dupFNErr
                ? "Upload refused: destination already exists."
                : "Upload refused: cannot create destination file.");
        }
        {
            OSErr sendError = serialSend(serial, &reply, 1);
            return sendError ? sendError : noErr;
        }
    }
    if (upload->active && command->command == 0xd9) {
        *handled = true;
        if (!upload->awaitingFinal || command->length != 1 || command->payload[0]) {
            strcpy(upload->message, "Upload failed or was cancelled by Apple II.");
            return uploadCancel(upload);
        }
        error = closeUpload(upload);
        if (!error) error = FSpRename(&upload->temporary, upload->destination.name);
        if (!error) {
            upload->temporary.name[0] = 0;
            upload->active = false;
            upload->awaitingFinal = false;
            snprintf(upload->message, sizeof(upload->message),
                     "Upload completed: %s (%u blocks).",
                     upload->imageName, upload->blocks);
        } else uploadCancel(upload);
        return error;
    }
    if (upload->active && command->command == 0xd8) {
        *handled = true;
        strcpy(upload->message, "Upload cancelled by Apple II.");
        return uploadCancel(upload);
    }
    if (upload->active) {
        *handled = true;
        strcpy(upload->message,
               "Upload stopped: unexpected final response from Apple II.");
        return uploadCancel(upload);
    }
    return noErr;
}

OSErr uploadFeed(ImageUpload *upload, SerialConnection *serial,
                 unsigned char byte, Boolean *handled)
{
    int result;
    unsigned char reply;
    OSErr error;
    *handled = false;
    if (!upload->active || upload->awaitingFinal) return noErr;
    *handled = true;
    upload->lastActivity = TickCount();
    result = uploadPacketFeed(&upload->packet, byte);
    if (result == UPLOAD_PACKET_NONE) return noErr;
    if (result == UPLOAD_PACKET_CANCEL) {
        strcpy(upload->message, "Upload cancelled by Apple II.");
        return uploadCancel(upload);
    }
    if (result == UPLOAD_PACKET_COMPLETE &&
        upload->packet.block < upload->done) {
        reply = 6; /* A duplicate packet was already committed. */
    } else if (result == UPLOAD_PACKET_COMPLETE &&
               upload->packet.block == upload->done &&
               upload->done + upload->packet.length / 512 <= upload->blocks) {
        error = writePacket(upload);
        if (error) return error;
        upload->done += upload->packet.length / 512;
        reply = 6;
        snprintf(upload->message, sizeof(upload->message),
                 "Receiving %s: %u/%u blocks.",
                 upload->imageName, upload->done, upload->blocks);
        if (upload->done == upload->blocks) upload->awaitingFinal = true;
    } else reply = 0x15;
    uploadPacketReset(&upload->packet);
    return serialSend(serial, &reply, 1);
}
