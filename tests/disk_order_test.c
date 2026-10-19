#include "disk_order.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    unsigned char trackData[4096], actual[512], expected[512];
    unsigned track, sector, i, block;
    FILE *reference;
    assert(argc == 2);
    reference = fopen(argv[1], "rb");
    assert(reference);
    for (track = 0; track < 35; ++track) {
        for (sector = 0; sector < 16; ++sector)
            for (i = 0; i < 256; ++i)
                trackData[sector * 256 + i] = track * 17 + sector * 11 + i;
        for (block = 0; block < 8; ++block) {
            assert(diskOrderBlock(trackData, block, actual));
            assert(fread(expected, 1, 512, reference) == 512);
            assert(!memcmp(actual, expected, 512));
        }
    }
    assert(fgetc(reference) == EOF);
    assert(!diskOrderBlock(trackData, 8, actual));
    fclose(reference);
    puts("All 280 DOS-order blocks match ADTPro's AppleCommander library");
    return 0;
}
