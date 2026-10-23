#include "upload_packet.h"
#include "wide.h"
#include <assert.h>
#include <string.h>

static int feed(UploadPacket *packet, const unsigned char *wire, unsigned length)
{
    int result = UPLOAD_PACKET_NONE;
    unsigned i;
    for (i = 0; i < length; ++i) result = uploadPacketFeed(packet, wire[i]);
    return result;
}

int main(void)
{
    static unsigned char source[40 * 512], wire[40970];
    UploadPacket packet;
    unsigned short length;
    unsigned i, blocks;
    for (i = 0; i < sizeof(source); ++i)
        source[i] = i < 4096 ? (unsigned char)(i * 17 + i / 11) : 0;
    for (blocks = 1; blocks <= 40; ++blocks) {
        length = wideEncode(source, blocks * 512, 80, wire, sizeof(wire));
        assert(length);
        uploadPacketReset(&packet);
        assert(feed(&packet, wire, length) == UPLOAD_PACKET_COMPLETE);
        assert(packet.block == 80 && packet.length == blocks * 512);
        assert(!memcmp(packet.data, source, blocks * 512));
    }

    wire[length - 1] ^= 1;
    uploadPacketReset(&packet);
    assert(feed(&packet, wire, length) == UPLOAD_PACKET_BAD);

    {
        static const unsigned char cancel[] = {0xc1, 0, 0, 0xd8, 0x19};
        uploadPacketReset(&packet);
        assert(feed(&packet, cancel, sizeof(cancel)) == UPLOAD_PACKET_CANCEL);
    }
    return 0;
}
