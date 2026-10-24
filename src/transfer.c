/* SPDX-License-Identifier: GPL-2.0-or-later
 * Wide send state machine follows ADTPro CommsThread.sendDiskWide and
 * sendPacketWide, Copyright (C) 2006-2020 David Schmidt.
 */
#include "transfer.h"
#include "directory.h"
#include <OSUtils.h>
#include <Errors.h>
#include <stdio.h>
#include <string.h>

enum { IDLE, INITIAL_ACK, SEND_NEXT, PACKET_ACK, RESEND, FINAL_REPORT };
/* More forgiving than the desktop server on slow classic hardware. */
#define MAX_PACKET_RETRIES 10
#define ACK_TIMEOUT_TICKS 7200UL
#define DISK_ACK_TIMEOUT_TICKS 18000UL

static OSErr retryWithReason(ImageTransfer *t, const char *reason)
{
    /* A burst of noise schedules one resend, not several retry attempts. */
    if (t->state == RESEND) return noErr;
    if (t->state == PACKET_ACK) {
        if (++t->retries <= MAX_PACKET_RETRIES) {
            t->state = RESEND;
            t->lastActivity = TickCount();
            snprintf(t->retryReason, sizeof(t->retryReason), "%s", reason);
            snprintf(t->message, sizeof(t->message), "Block %u retry %d/%d: %s",
                     t->block, t->retries, MAX_PACKET_RETRIES, t->retryReason);
            return noErr;
        }
        snprintf(t->message, sizeof(t->message), "Stopped at block %u: %s (retry limit).", t->block, reason);
        return transferCancel(t);
    }
    /* No data packet exists yet during initial ACK; keep waiting for it. */
    strcpy(t->message, "Ignoring invalid ACK; waiting for a valid acknowledgement.");
    return noErr;
}

OSErr transferRetry(ImageTransfer *t)
{
    return retryWithReason(t, "damaged or incomplete ACK");
}

OSErr transferCancel(ImageTransfer *t)
{
    /* Stop the state machine even when the file close needs a later retry. */
    t->state = IDLE;
    OSErr error = imageClose(&t->image);
    return error;
}

static OSErr requestName(const WideParser *p, unsigned char *name, unsigned *end)
{
    unsigned i;
    if (!p->length || p->length > sizeof(p->payload)) return paramErr;
    for (i = 0; i < p->length && p->payload[i]; ++i) {}
    if (i == p->length) return paramErr;
    *end = i;
    if (p->payload[0] == 1) {
        if (i != 3) return paramErr;
        return directorySelection(p->payload[1], p->payload[2], name);
    }
    if (!i || i > 31) return paramErr;
    name[0] = i;
    for (i = 0; i < name[0]; ++i) name[i + 1] = p->payload[i] & 0x7f;
    return noErr;
}

OSErr transferCommand(ImageTransfer *t, SerialConnection *s,
                      const ImageFolder *folder, const WideParser *p, Boolean *handled)
{
    unsigned char name[32], reply[3];
    unsigned end;
    OSErr error;
    *handled = false;
    if (p->command == 0xd9 && t->state == FINAL_REPORT) {
        *handled = true;
        if (p->length == 1 && p->payload[0] == 0) {
            strcpy(t->message, "Image transfer completed and verified by Apple II.");
            return transferCancel(t);
        }
        strcpy(t->message, "Apple II reported errors after writing the disk.");
        return transferCancel(t);
    }
    if (p->command == 0xd8 && t->state) {
        *handled = true;
        strcpy(t->message, "Image transfer cancelled by Apple II.");
        return transferCancel(t);
    }
    if (p->command == 0xcb && t->state) {
        unsigned short next;
        *handled = true;
        if (p->length != 3) return transferRetry(t);
        next = p->payload[1] | ((unsigned short)p->payload[2] << 8);
        if (p->payload[0] == 0x18) {
            strcpy(t->message, "Image transfer cancelled by Apple II.");
            return transferCancel(t);
        }
        /* Old ACK/NAK frames can trail a successful resend. They must not
         * spend the next packet's retry allowance or request another resend. */
        if (t->state != INITIAL_ACK && next < t->block &&
            (p->payload[0] == 6 || p->payload[0] == 0x15)) return noErr;
        /* The previous ACK can arrive twice or trail a newly queued packet. */
        if (p->payload[0] == 6 && t->state != INITIAL_ACK && next == t->block)
            return noErr;
        if (t->state == SEND_NEXT) return noErr;
        if (p->payload[0] == 6 && next == t->block + (t->state == INITIAL_ACK ? 0 : t->chunk)) {
            if (t->state != INITIAL_ACK) t->block = next;
            t->retries = 0;
            t->retryReason[0] = 0;
            t->lastActivity = TickCount();
            snprintf(t->message, sizeof(t->message), "Acknowledged block %u/%u. Packet retries: 0.",
                     t->block, t->image.blocks);
            if (t->block == t->image.blocks) {
                t->state = FINAL_REPORT;
                t->lastActivity = TickCount();
                strcpy(t->message, "All blocks acknowledged; waiting for Apple II final status.");
                return noErr;
            }
            t->state = SEND_NEXT;
        } else {
            char reason[64];
            snprintf(reason, sizeof(reason), "ACK $%02X next %u; expected %u",
                     p->payload[0], next, t->block + (t->state == INITIAL_ACK ? 0 : t->chunk));
            return retryWithReason(t, reason);
        }
        return noErr;
    }
    if (p->command != 0xda && p->command != 0xc7) return noErr;
    *handled = true;
    if (s->writing) return paramErr;
    error = transferCancel(t);
    if (error) return error;
    reply[0] = reply[1] = 0;
    reply[2] = requestName(p, name, &end) ? 2 : imageOpen(folder, name, &t->image);
    if (p->command == 0xda) {
        if (!reply[2]) { reply[0] = t->image.blocks; reply[1] = t->image.blocks >> 8; }
        snprintf(t->message, sizeof(t->message), "Image size query: %u blocks, status %u.", t->image.blocks, reply[2]);
        error = transferCancel(t);
        return error ? error : serialSend(s, reply, 3);
    }
    if (!reply[2] && (end + 2 != p->length || !p->payload[end + 1] || p->payload[end + 1] > 40)) reply[2] = 4;
    if (reply[2]) {
        snprintf(t->message, sizeof(t->message), "Cannot send image (status %u). Use block-aligned .po or full-track .dsk/.do.", reply[2]);
        error = transferCancel(t);
        return error ? error : serialSend(s, reply + 2, 1);
    }
    memcpy(t->imageName, name + 1, name[0]);
    t->imageName[name[0]] = 0;
    t->preferred = p->payload[end + 1];
    /* Client advances by its BAOCNT, not by the payload's actual length. */
    t->block = t->chunk = 0;
    t->retries = 0;
    t->retryReason[0] = 0;
    t->state = INITIAL_ACK;
    t->lastActivity = TickCount();
    snprintf(t->message, sizeof(t->message), "Image: %u blocks (%lu bytes). Waiting for Apple II ACK.",
             t->image.blocks, (unsigned long)t->image.blocks * 512);
    return serialSend(s, reply, 1);
}

OSErr transferPoll(ImageTransfer *t, SerialConnection *s)
{
    OSErr error;
    if (!t->state) return noErr;
    if (s->writing) { t->lastActivity = TickCount(); return noErr; }
    if (t->state == FINAL_REPORT) {
        if ((unsigned long)(TickCount() - t->lastActivity) <= ACK_TIMEOUT_TICKS)
            return noErr;
        strcpy(t->message, "Timed out waiting for Apple II final status.");
        return transferCancel(t);
    }
    if (t->state == INITIAL_ACK || t->state == PACKET_ACK) {
        /* The client does not read another packet during physical disk writes.
         * Resending on silence here can put a duplicate in its next buffer. */
        unsigned long elapsed = (unsigned long)(TickCount() - t->lastActivity);
        Boolean diskWrite = t->state == PACKET_ACK &&
            ((t->block + t->chunk) % 40 == 0 ||
             t->block + t->chunk == t->image.blocks);
        if (diskWrite) {
            if (elapsed <= DISK_ACK_TIMEOUT_TICKS) {
                if (elapsed > 120)
                    snprintf(t->message, sizeof(t->message),
                             "Waiting for Apple II disk write/ACK %u. Retries: %d.",
                             t->block + t->chunk, t->retries);
                return noErr;
            }
            snprintf(t->message, sizeof(t->message),
                     "No disk-write ACK %u after 5 minutes; stopped. Check Apple II/drive.",
                     t->block + t->chunk);
            return transferCancel(t);
        }
        if (elapsed <= ACK_TIMEOUT_TICKS) return noErr;
        if (t->state == PACKET_ACK) return retryWithReason(t, "no ACK within 120 seconds");
        strcpy(t->message, "Initial image acknowledgement timed out.");
        return transferCancel(t);
    }
    if (t->state == RESEND) {
        unsigned long pause = t->retries < 4 ? t->retries * 15UL : 60UL;
        if ((unsigned long)(TickCount() - t->lastActivity) < pause) return noErr;
    }
    if (t->state == SEND_NEXT) {
        unsigned short available = t->image.blocks - t->block;
        unsigned short boundary = 40 - t->block % 40;
        t->chunk = t->preferred;
        if (t->chunk > available) t->chunk = available;
        if (t->chunk > boundary) t->chunk = boundary;
        error = imageRead(&t->image, t->block, t->chunk, t->data);
        if (error) { transferCancel(t); strcpy(t->message, "Image read failed."); return error; }
        t->packetLength = wideEncode(t->data, t->chunk * 512, t->block, t->packet, sizeof(t->packet));
        if (!t->packetLength) { transferCancel(t); return paramErr; }
    }
    error = serialSend(s, t->packet, t->packetLength);
    if (error) { transferCancel(t); return error; }
    t->state = PACKET_ACK;
    t->lastActivity = TickCount();
    if (t->retries)
        snprintf(t->message, sizeof(t->message), "Resending %u-%u/%u (%u bytes), retry %d: %s",
                 t->block, t->block + t->chunk, t->image.blocks,
                 t->packetLength, t->retries, t->retryReason);
    else
        snprintf(t->message, sizeof(t->message), "Sending %u-%u/%u (%u bytes). ACK %u; retries: 0.",
                 t->block, t->block + t->chunk, t->image.blocks,
                 t->packetLength, t->block + t->chunk);
    return noErr;
}
