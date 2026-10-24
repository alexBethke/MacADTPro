#include "transfer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned long ticks;
static unsigned sends;
static OSErr closeFailure;
static unsigned short mockBlocks = 45;
static int sparseImage;
static long sentLength;
static unsigned char sent[40970];
static unsigned short readStart, readBlocks;
unsigned long TickCount(void) { return ticks; }
OSErr imageClose(DiskImage *image) { if (closeFailure) return closeFailure; image->ref = 0; return 0; }
unsigned char imageOpen(const ImageFolder *folder, const unsigned char *name, DiskImage *image)
{ (void)folder; assert(name[0] == 7); image->ref = 1; image->blocks = mockBlocks; return 0; }
OSErr imageRead(DiskImage *image, unsigned short start, unsigned short blocks, unsigned char *data)
{
    assert(image->ref); readStart = start; readBlocks = blocks;
    for (unsigned i = 0; i < (unsigned)blocks * 512; ++i) {
        unsigned offset = (unsigned)start * 512 + i;
        data[i] = sparseImage && offset < 150U * 512 ? (offset * 7) ^ (offset / 13) : 0;
    }
    return 0;
}
OSErr directorySelection(unsigned char page, unsigned char row, unsigned char *name)
{ (void)page; (void)row; memcpy(name, "\7test.po", 8); return 0; }
OSErr serialSend(SerialConnection *serial, const unsigned char *bytes, long count)
{ (void)serial; assert(count > 0 && count <= (long)sizeof(sent)); memcpy(sent, bytes, count); sentLength = count; ++sends; return 0; }

static void ack(ImageTransfer *t, SerialConnection *s, unsigned char code, unsigned short next)
{
    WideParser p = {0};
    ImageFolder folder = {0};
    Boolean handled;
    p.command = 0xcb; p.length = 3; p.payload[0] = code;
    p.payload[1] = next; p.payload[2] = next >> 8;
    assert(!transferCommand(t, s, &folder, &p, &handled)); assert(handled);
    if (strstr(t->message, "waiting for Apple II final status")) {
        memset(&p, 0, sizeof(p));
        p.command = 0xd9; p.length = 1; p.payload[0] = 0;
        assert(!transferCommand(t, s, &folder, &p, &handled)); assert(handled);
    }
}

/* Model RECVWIDE's writes/skips and UNDIFFWide separately, using the
 * client's zero-cleared 40-block buffer. Check CRC with an independent bit loop. */
static void receivePacket(unsigned char *buffer, unsigned offset, unsigned blocks)
{
    unsigned length = blocks * 512, position = 0, input = 7;
    unsigned short crc = 0;
    unsigned char previous = 0;
    assert(sent[0] == 0xc1 && sent[3] == 0xd3);
    assert((sent[1] | ((unsigned)sent[2] << 8)) == length);
    while (position < length) {
        assert(input < (unsigned)sentLength - 2);
        unsigned char delta = sent[input++];
        if (delta) buffer[offset + position++] = delta;
        else {
            unsigned end = sent[input++];
            end += position & ~255U;
            if (end <= position) end += 256;
            assert(end <= length);
            /* RECVWIDE leaves zero runs in the already-cleared buffer. */
            position = end;
        }
    }
    assert(input + 2 == (unsigned)sentLength);
    for (unsigned i = 0; i < length; ++i) {
        previous += buffer[offset + i];
        buffer[offset + i] = previous;
        crc ^= (unsigned short)previous << 8;
        for (int bit = 0; bit < 8; ++bit)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    assert(sent[input] == (crc & 255) && sent[input + 1] == (crc >> 8));
}

int main(void)
{
    static ImageTransfer t;
    static SerialConnection s;
    ImageFolder folder = {0};
    WideParser p = {0};
    Boolean handled;
    p.command = 0xda; p.length = 8; memcpy(p.payload, "test.po", 8);
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    assert(handled && sentLength == 3 && sent[0] == 45 && !sent[1] && !sent[2] && !t.image.ref);
    p.command = 0xc7; p.length = 9; p.payload[8] = 17;
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    assert(sentLength == 1 && sent[0] == 0);
    assert(!strcmp(t.imageName, "test.po"));
    ack(&t, &s, 6, 0);
    assert(!transferPoll(&t, &s)); assert(readStart == 0 && readBlocks == 17);
    ack(&t, &s, 0x15, 0);
    assert(strstr(t.message, "next 0; expected 17"));
    { unsigned previousSends = sends; assert(!transferPoll(&t, &s)); assert(sends == previousSends); ticks += 15; assert(!transferPoll(&t, &s)); assert(sends == previousSends + 1); }
    ack(&t, &s, 6, 17);
    while (t.state) {
        assert(!transferPoll(&t, &s));
        assert(readStart == t.block && readBlocks <= 17);
        assert(readStart / 40 == (readStart + readBlocks - 1) / 40);
        ack(&t, &s, 6, readStart + readBlocks);
    }
    assert(!t.state && !t.image.ref && strstr(t.message, "completed"));
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ticks += 7201; assert(!transferPoll(&t, &s)); assert(!t.state && !t.image.ref);
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 0x18, 0); assert(!t.state);
    p.payload[8] = 0;
    assert(!transferCommand(&t, &s, &folder, &p, &handled)); assert(sent[0] == 4 && !t.image.ref);
    p.payload[8] = 17;
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    closeFailure = -36;
    assert(transferCancel(&t) == -36);
    assert(!t.state && t.image.ref); /* No sending after a failed file close. */
    { unsigned previousSends = sends; assert(!transferPoll(&t, &s)); assert(sends == previousSends); }
    closeFailure = 0;
    assert(!transferCancel(&t) && !t.image.ref);
    mockBlocks = 280;
    p.payload[8] = 10;
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 6, 0);
    while (t.state) {
        assert(!transferPoll(&t, &s));
        assert(readBlocks == 10); /* Apple II increments by configured BAOCNT. */
        ack(&t, &s, 6, readStart + 10);
    }
    assert(t.block == 280 && !t.image.ref);
    assert(strstr(t.message, "completed"));
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 6, 0);
    assert(!transferPoll(&t, &s));
    for (short attempt = 1; attempt <= 10; ++attempt) {
        unsigned previousSends = sends;
        assert(!transferRetry(&t) && t.state && t.retries == attempt);
        /* Multiple damaged envelopes before a resend consume one attempt. */
        for (unsigned noise = 0; noise < 20; ++noise)
            assert(!transferRetry(&t) && t.retries == attempt);
        assert(!transferPoll(&t, &s) && sends == previousSends);
        ticks += attempt < 4 ? attempt * 15 : 60;
        assert(!transferPoll(&t, &s) && sends == previousSends + 1);
    }
    assert(!transferRetry(&t) && !t.state && !t.image.ref);
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 6, 0);
    ack(&t, &s, 6, 0); /* Duplicate initial ACK before sending. */
    assert(!transferPoll(&t, &s) && readStart == 0);
    ack(&t, &s, 6, 0); /* Stale ACK while awaiting packet acknowledgement. */
    assert(!t.retries && t.state);
    ack(&t, &s, 6, 10);
    ack(&t, &s, 6, 10);
    assert(!transferPoll(&t, &s) && readStart == 10);
    ticks += 3601;
    assert(!transferPoll(&t, &s) && !t.retries && t.state);
    assert(!transferCancel(&t));
    /* Silence while the Apple II writes a 40-block buffer must not resend. */
    mockBlocks = 280;
    p.payload[8] = 40;
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 6, 0);
    assert(!transferPoll(&t, &s));
    unsigned previousSends = sends;
    ticks += 7201;
    assert(!transferPoll(&t, &s) && t.state && !t.retries && sends == previousSends);
    assert(strstr(t.message, "disk write/ACK 40"));
    ack(&t, &s, 6, 40);
    while (t.block < 240) {
        assert(!transferPoll(&t, &s));
        ack(&t, &s, 6, t.block + t.chunk);
    }
    assert(!transferPoll(&t, &s));
    previousSends = sends;
    ticks += 7201;
    assert(!transferPoll(&t, &s) && t.state && !t.retries && sends == previousSends);
    ack(&t, &s, 6, 280);
    assert(!t.state && strstr(t.message, "completed"));
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 6, 0);
    assert(!transferPoll(&t, &s));
    previousSends = sends;
    ticks += 18001;
    assert(!transferPoll(&t, &s) && !t.state && sends == previousSends);
    assert(strstr(t.message, "No disk-write ACK 40"));
    /* Retry counts are per packet: exhaust most of an allowance repeatedly,
     * accept a delayed success during backoff, and discard prior-packet NAKs. */
    mockBlocks = 280;
    p.payload[8] = 10;
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 6, 0);
    for (unsigned block = 0; block < 280; block += 10) {
        assert(!transferPoll(&t, &s) && readStart == block);
        assert(!t.retries && !t.retryReason[0]);
        assert(strstr(t.message, "retries: 0"));
        if (block) {
            unsigned previousSends = sends;
            ack(&t, &s, 0x15, block - 10);
            assert(!transferPoll(&t, &s) && sends == previousSends && !t.retries);
        }
        for (short attempt = 1; attempt <= 8; ++attempt) {
            ack(&t, &s, 0x15, block);
            assert(t.retries == attempt && t.retryReason[0]);
            ticks += attempt < 4 ? attempt * 15 : 60;
            assert(!transferPoll(&t, &s));
        }
        ack(&t, &s, 0x15, block); /* Pending ninth retry, not yet sent. */
        unsigned previousSends = sends;
        ack(&t, &s, 6, block + 10); /* Late success cancels pending resend. */
        assert(!t.retries && !t.retryReason[0] && t.block == block + 10);
        assert(sends == previousSends);
        if (t.state) ack(&t, &s, 6, block + 10); /* Duplicate success. */
        assert(!t.retries);
    }
    assert(!t.state && strstr(t.message, "completed"));
    /* Real file-derived sizes: smaller/larger than 280, partial final packets. */
    {
        const unsigned short lengths[] = {1, 8, 24, 41, 45, 256, 279, 280, 320};
        for (unsigned i = 0; i < sizeof(lengths) / sizeof(lengths[0]); ++i) {
            mockBlocks = lengths[i];
            p.command = 0xda; p.length = 8;
            assert(!transferCommand(&t, &s, &folder, &p, &handled));
            assert(sentLength == 3 && !sent[2]);
            assert((sent[0] | ((unsigned short)sent[1] << 8)) == mockBlocks);
            p.command = 0xc7; p.length = 9; p.payload[8] = 10;
            assert(!transferCommand(&t, &s, &folder, &p, &handled));
            ack(&t, &s, 6, 0);
            unsigned total = 0;
            while (t.state) {
                assert(!transferPoll(&t, &s));
                assert(readStart == total && readStart + readBlocks <= mockBlocks);
                assert((sent[1] | ((unsigned short)sent[2] << 8)) == readBlocks * 512);
                total += readBlocks;
                ack(&t, &s, 6, total);
            }
            assert(total == mockBlocks && t.block == mockBlocks);
            assert(!t.image.ref && strstr(t.message, "completed"));
            unsigned previousSends = sends;
            assert(!transferPoll(&t, &s) && sends == previousSends);
        }
    }
    /* The last block ACK is followed by a separate final disk-write report. */
    mockBlocks = 1;
    p.command = 0xc7; p.length = 9; p.payload[8] = 1;
    assert(!transferCommand(&t, &s, &folder, &p, &handled));
    ack(&t, &s, 6, 0);
    assert(!transferPoll(&t, &s));
    {
        WideParser lastAck = {0};
        lastAck.command = 0xcb; lastAck.length = 3;
        lastAck.payload[0] = 6; lastAck.payload[1] = 1;
        assert(!transferCommand(&t, &s, &folder, &lastAck, &handled) && handled);
        assert(t.state && strstr(t.message, "waiting for Apple II final status"));
        ticks += 7201;
        assert(!transferPoll(&t, &s) && !t.state);
        assert(strstr(t.message, "Timed out"));
    }
    /* Full 140K image: last 130 blocks are empty. Exercise every BAOCNT.
     * Verify reconstructed bytes and the Apple client's actual ACK envelopes. */
    sparseImage = 1;
    mockBlocks = 280;
    for (unsigned preferred = 1; preferred <= 40; ++preferred) {
        static unsigned char clientBuffer[40 * 512];
        unsigned next = 0, packets = 0;
        p.command = 0xc7; p.length = 9; p.payload[8] = preferred;
        assert(!transferCommand(&t, &s, &folder, &p, &handled));
        ack(&t, &s, 6, 0);
        while (next < 280) {
            if (!(next % 40)) memset(clientBuffer, 0, sizeof(clientBuffer));
            unsigned count = preferred;
            if (count > 40 - next % 40) count = 40 - next % 40;
            if (count > 280 - next) count = 280 - next;
            assert(!transferPoll(&t, &s));
            assert(readStart == next && readBlocks == count);
            assert((sent[5] | ((unsigned)sent[6] << 8)) == next);
            receivePacket(clientBuffer, next % 40 * 512, count);
            for (unsigned i = 0; i < count * 512; ++i) {
                unsigned offset = next * 512 + i;
                unsigned char expected = offset < 150U * 512 ? (offset * 7) ^ (offset / 13) : 0;
                assert(clientBuffer[next % 40 * 512 + i] == expected);
            }
            if (next >= 150) assert(sentLength == 9 + count * 4);
            next += count;
            unsigned char wireAck[] = {0xc1, 3, 0, 0xcb, 9, 6, next & 255, next >> 8, 0};
            wireAck[8] = wireAck[5] ^ wireAck[6] ^ wireAck[7];
            WideParser incoming = {0};
            for (unsigned i = 0; i < sizeof(wireAck); ++i) {
                int result = wideFeed(&incoming, wireAck[i]);
                assert(result == (i + 1 == sizeof(wireAck) ? WIDE_COMMAND : WIDE_NONE));
            }
            assert(!transferCommand(&t, &s, &folder, &incoming, &handled) && handled);
            ++packets;
        }
        {
            WideParser final = {0};
            final.command = 0xd9;
            final.length = 1;
            final.payload[0] = 0;
            assert(strstr(t.message, "waiting for Apple II final status"));
            assert(!transferCommand(&t, &s, &folder, &final, &handled) && handled);
        }
        assert(packets && !t.state && t.block == 280 && !t.image.ref);
        assert(strstr(t.message, "completed"));
        unsigned previousSends = sends;
        assert(!transferPoll(&t, &s) && sends == previousSends);
    }
    puts("Full-image empty tails decode correctly for all 40 packet sizes; final status completes block 280 without padding");
    puts("Image transfer state tests passed");
    return 0;
}
