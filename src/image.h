#ifndef MACADTPRO_IMAGE_H
#define MACADTPRO_IMAGE_H
#include "image_folder.h"
typedef struct {
    short ref;
    unsigned short blocks;
    Boolean dosOrder;
    long cachedTrack;
    unsigned char track[4096];
} DiskImage;
/* Wire status: 0 ready, 2 cannot open, 4 unsupported image format. */
unsigned char imageOpen(const ImageFolder *folder, const unsigned char *name, DiskImage *image);
OSErr imageClose(DiskImage *image);
OSErr imageRead(DiskImage *image, unsigned short start, unsigned short blocks, unsigned char *data);
#endif
