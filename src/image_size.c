#include "image_size.h"
unsigned short imageBlocksForLength(long length, int dosOrder)
{
    if (length <= 0 || length % 512 || length / 512 > 65535L)
        return 0;
    if (dosOrder && length % 4096) return 0;
    return (unsigned short)(length / 512);
}
