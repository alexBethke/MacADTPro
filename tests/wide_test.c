#include "wide.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void checkEncoder(void)
{
    unsigned char source[1024], packet[2060], decoded[1024];
    unsigned short length;
    unsigned i, position, input;
    unsigned char previous;
    memset(source, 0, sizeof(source));
    length = wideEncode(source, sizeof(source), 0, packet, sizeof(packet));
    assert(length == 17);
    assert(packet[0] == 0xc1 && packet[1] == 0 && packet[2] == 4);
    assert(packet[3] == 0xd3 && packet[4] == 0x16);
    for (i = 5; i < length; ++i) assert(packet[i] == 0);
    assert(wideEncode(source, sizeof(source), 0, packet, 10) == 0);
    assert(wideEncode(source, 255, 0, packet, sizeof(packet)) == 0);
    for (i = 0; i < sizeof(source); ++i) source[i] = (i / 13) ^ (i * 7);
    length = wideEncode(source, sizeof(source), 0x1234, packet, sizeof(packet));
    assert(length && packet[5] == 0x34 && packet[6] == 0x12);
    input = 7; position = 0; previous = 0;
    while (position < sizeof(decoded)) {
        unsigned char delta = packet[input++];
        if (delta) {
            previous += delta;
            decoded[position++] = previous;
        } else {
            unsigned end = packet[input++];
            end += position & ~255U;
            if (end <= position) end += 256;
            assert(end <= sizeof(decoded));
            while (position < end) decoded[position++] = previous;
        }
    }
    assert(input + 2 == length);
    assert(!memcmp(source, decoded, sizeof(source)));
    /* Independently checked with Python binascii.crc_hqx(..., 0). */
    assert(packet[length - 2] == 0x8c && packet[length - 1] == 0x09);
    {
        static unsigned char large[20480], encoded[40970];
        memset(large, 0, sizeof(large));
        assert(wideEncode(large, sizeof(large), 40, encoded, sizeof(encoded)) == 169);
        assert(encoded[1] == 0 && encoded[2] == 0x50);
        for (i = 0; i < sizeof(large); ++i) large[i] = (i / 2) & 255;
        length = wideEncode(large, sizeof(large), 40, encoded, sizeof(encoded));
        assert(length > 20480 && length <= sizeof(encoded));
    }
}

static int feed(WideParser *parser, const unsigned char *bytes, unsigned count)
{
    unsigned i;
    int result = WIDE_NONE;
    for (i = 0; i < count; ++i) result = wideFeed(parser, bytes[i]);
    return result;
}

int main(void)
{
    WideParser parser;
    /* Upstream sendHome() packet, and DIR request containing "*". */
    const unsigned char home[] = {0xc1, 0, 0, 0xd8, 0x19};
    const unsigned char dir[] = {0xc1, 1, 0, 0xc4, 4, '*', '*'};
    const unsigned char brokenHeader[] = {0xc1, 1, 0, 0xc4, 5};
    const unsigned char brokenPayload[] = {0xc1, 1, 0, 0xc4, 4, '*', 0};
    const unsigned char embedded[] = {0xc1, 2, 0, 0xc4, 7, 0xc1, 0xd9, 0x18};
    unsigned long i;
    checkEncoder();
    wideReset(&parser);
    assert(feed(&parser, home, sizeof(home)) == WIDE_COMMAND);
    assert(parser.command == 0xd8);
    assert(feed(&parser, dir, sizeof(dir)) == WIDE_COMMAND);
    assert(parser.command == 0xc4);
    assert(parser.length == 1 && parser.stored == 1 && parser.payload[0] == '*');
    assert(feed(&parser, brokenHeader, sizeof(brokenHeader)) == WIDE_BAD_HEADER);
    assert(feed(&parser, brokenPayload, sizeof(brokenPayload)) == WIDE_BAD_PAYLOAD);
    assert(feed(&parser, embedded, sizeof(embedded)) == WIDE_COMMAND);
    /* Largest legal length is consumed without an allocation or wraparound. */
    assert(wideFeed(&parser, 0xc1) == WIDE_NONE);
    assert(wideFeed(&parser, 0xff) == WIDE_NONE);
    assert(wideFeed(&parser, 0xff) == WIDE_NONE);
    assert(wideFeed(&parser, 0xc4) == WIDE_NONE);
    assert(wideFeed(&parser, 5) == WIDE_NONE);
    for (i = 0; i < 65535UL; ++i) assert(wideFeed(&parser, 0) == WIDE_NONE);
    assert(wideFeed(&parser, 0) == WIDE_COMMAND);
    assert(wideFeed(&parser, 0xd9) == WIDE_PING);
    wideFeed(&parser, 0xc1);
    wideReset(&parser);
    assert(feed(&parser, home, sizeof(home)) == WIDE_COMMAND);
    puts("Wide parser tests passed");
    return 0;
}
