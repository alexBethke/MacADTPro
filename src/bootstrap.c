/* SPDX-License-Identifier: GPL-2.0-or-later
 * Uses the unmodified ADTPro 2.1.0 bootstrap payloads, except that the two
 * 115200-baud defaults are changed in transit to 19200 for classic Mac SCCs.
 */
#include "bootstrap.h"
#include <Resources.h>
#include <OSUtils.h>
#include <Errors.h>
#include <string.h>
#include <stdio.h>

enum { kBootstrapText = 200, kBootstrapProDOS = 201, kBootstrapClient = 202 };
enum { kGrubSpeedCharacter = 1214, kClientSpeed = 0x2984,
       kClientDefaultSpeed = 0x298a, kClientSavedSpeed = 0x298f };
static unsigned char payloadBuffer[17131];

static OSErr sendResource(SerialConnection *serial, short resourceID,
                          unsigned char signature, Boolean patchClient)
{
    Handle resource = GetResource('DATA', resourceID);
    long length;
    OSErr error;
    if (!resource) return ResError() ? ResError() : resNotFound;
    HLock(resource);
    length = GetHandleSize(resource);
    if (length < 1 || length + 3 > (long)sizeof(payloadBuffer)) {
        HUnlock(resource); ReleaseResource(resource); return paramErr;
    }
    payloadBuffer[0] = signature;
    payloadBuffer[1] = length;
    payloadBuffer[2] = length >> 8;
    memcpy(payloadBuffer + 3, *resource, length);
    HUnlock(resource);
    ReleaseResource(resource);
    if (patchClient) {
        /* PSPEED, DEFAULT speed, and SVSPEED in ADTPro 2.1.0. */
        payloadBuffer[3 + kClientSpeed] = 0;
        payloadBuffer[3 + kClientDefaultSpeed] = 0;
        payloadBuffer[3 + kClientSavedSpeed] = 0;
    }
    error = serialSend(serial, payloadBuffer, length + 3);
    return error;
}

OSErr bootstrapStart(Bootstrap *b)
{
    memset(b, 0, sizeof(*b));
    b->text = GetResource('DATA', kBootstrapText);
    if (!b->text) return ResError() ? ResError() : resNotFound;
    HLock(b->text);
    b->state = BOOTSTRAP_TEXT;
    strcpy(b->message, "Bootstrap 1/3: sending the monitor loader at 9600 baud.");
    return noErr;
}

void bootstrapCancel(Bootstrap *b)
{
    if (b->text) { HUnlock(b->text); ReleaseResource(b->text); }
    memset(b, 0, sizeof(*b));
}

Boolean bootstrapActive(const Bootstrap *b)
{
    return b->state != BOOTSTRAP_IDLE && b->state != BOOTSTRAP_DONE;
}

OSErr bootstrapPoll(Bootstrap *b, SerialConnection *serial)
{
    unsigned char line[256];
    long size;
    unsigned count = 0;
    OSErr error;
    if (b->state == BOOTSTRAP_SEND_PRODOS && !serial->writing) {
        b->state = BOOTSTRAP_WAIT_CLIENT;
        strcpy(b->message, "Bootstrap 2/3 loaded. Waiting for the ADTPro request.");
    } else if (b->state == BOOTSTRAP_SEND_CLIENT && !serial->writing) {
        b->state = BOOTSTRAP_DONE;
        strcpy(b->message, "ADTPro 2.1.0 is running in Apple II memory at 19200 baud.");
    }
    if (b->state != BOOTSTRAP_TEXT || serial->writing ||
        (long)(TickCount() - b->nextLine) < 0) return noErr;
    size = GetHandleSize(b->text);
    while (b->offset < (unsigned long)size && count < sizeof(line)) {
        unsigned long position = b->offset;
        unsigned char byte = (unsigned char)(*b->text)[b->offset++];
        if (byte == '\n') continue;
        if (position == kGrubSpeedCharacter) byte = '0'; /* BPS192K */
        if (byte == '\r') { line[count++] = 0x8d; break; }
        line[count++] = byte;
    }
    if (!count) {
        HUnlock(b->text); ReleaseResource(b->text); b->text = 0;
        error = serialSetBaud(serial, 4);
        if (error) return error;
        b->state = BOOTSTRAP_WAIT_PRODOS;
        strcpy(b->message, "Bootstrap loader started; switched to 19200. Waiting for ProDOS.");
        return noErr;
    }
    error = serialSend(serial, line, count);
    if (!error) {
        /* Upstream Speediboot allows 500 ms for the first four monitor
           lines, then 250 ms per line. */
        ++b->textLine;
        b->nextLine = TickCount() + (b->textLine <= 4 ? 30 : 15);
    }
    return error;
}

Boolean bootstrapCommand(Bootstrap *b, SerialConnection *serial,
                         unsigned char command, const unsigned char *payload,
                         unsigned short length, OSErr *error)
{
    if (!bootstrapActive(b) || command != 0xc6 || length != 1) return false;
    if (payload[0] == 0xb2 && b->state == BOOTSTRAP_WAIT_PRODOS) {
        *error = sendResource(serial, kBootstrapProDOS, 'P', false);
        if (!*error) {
            b->state = BOOTSTRAP_SEND_PRODOS;
            strcpy(b->message, "Bootstrap 2/3: loading ProDOS into Apple II memory.");
        }
        return true;
    }
    if (payload[0] == 0xb6 && b->state == BOOTSTRAP_WAIT_CLIENT) {
        *error = sendResource(serial, kBootstrapClient, 'A', true);
        if (!*error) {
            b->state = BOOTSTRAP_SEND_CLIENT;
            strcpy(b->message, "Bootstrap 3/3: loading the ADTPro 2.1.0 client.");
        }
        return true;
    }
    *error = noErr;
    return true;
}
