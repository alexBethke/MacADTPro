#include "wide.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
static unsigned readInt(FILE *f) {
    unsigned n = 0;
    for (int i = 0; i < 4; ++i) { int c = fgetc(f); assert(c != EOF); n = (n << 8) | c; }
    return n;
}
int main(int argc, char **argv) {
    static unsigned char data[20480], expected[40970], actual[40970];
    assert(argc == 2);
    FILE *f = fopen(argv[1], "rb"); assert(f);
    for (int test = 0; test < 15; ++test) {
        unsigned pattern = readInt(f), blocks = readInt(f), length = readInt(f);
        assert(blocks <= 40 && length <= sizeof(expected));
        assert(fread(expected, 1, length, f) == length);
        for (unsigned i = 0; i < blocks * 512; ++i)
            data[i] = pattern == 0 ? 0 : pattern == 1 ? (i / 13) ^ (i * 7) : i < 512 ? i : 0;
        assert(wideEncode(data, blocks * 512, 280 - blocks, actual, sizeof(actual)) == length);
        assert(!memcmp(expected, actual, length));
    }
    assert(fgetc(f) == EOF); fclose(f);
    puts("15 packets match ADTPro 2.1.0 byte for byte, including zero-filled tails");
    return 0;
}
