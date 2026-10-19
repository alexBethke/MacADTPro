#ifndef MACADTPRO_DISK_ORDER_H
#define MACADTPRO_DISK_ORDER_H
/* One 4096-byte DOS-order track, one ProDOS block index (0..7). */
int diskOrderBlock(const unsigned char *track, unsigned block, unsigned char *out);
#endif
