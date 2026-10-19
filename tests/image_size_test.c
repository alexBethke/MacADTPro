#include "image_size.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    assert(imageBlocksForLength(143360, 1) == 280);
    assert(imageBlocksForLength(131072, 1) == 256);
    assert(imageBlocksForLength(4096, 1) == 8);
    assert(imageBlocksForLength(163840, 1) == 320);
    assert(!imageBlocksForLength(143359, 1));
    assert(!imageBlocksForLength(512, 1));
    assert(imageBlocksForLength(512, 0) == 1);
    assert(imageBlocksForLength(23040, 0) == 45);
    assert(imageBlocksForLength(65535L * 512, 0) == 65535);
    assert(!imageBlocksForLength(65536L * 512, 0));
    assert(!imageBlocksForLength(0, 0));
    assert(!imageBlocksForLength(-512, 0));
    puts("Image length tests passed");
    return 0;
}
