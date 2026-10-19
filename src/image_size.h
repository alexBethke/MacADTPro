#ifndef MACADTPRO_IMAGE_SIZE_H
#define MACADTPRO_IMAGE_SIZE_H
/* ADTPro counts 512-byte blocks. DOS-order mapping requires complete tracks. */
unsigned short imageBlocksForLength(long length, int dosOrder);
#endif
