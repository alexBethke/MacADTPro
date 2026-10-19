#ifndef MACADTPRO_WIDE_H
#define MACADTPRO_WIDE_H
typedef struct {
    unsigned char header[4], command, check;
    unsigned short remaining;
    unsigned char state, index;
    unsigned short length, stored;
    unsigned char payload[256];
} WideParser;
enum { WIDE_NONE, WIDE_COMMAND, WIDE_BAD_HEADER, WIDE_BAD_PAYLOAD, WIDE_PING };
void wideReset(WideParser *parser);
int wideFeed(WideParser *parser, unsigned char byte);
unsigned short wideEncode(const unsigned char *data, unsigned short length,
                          unsigned short block, unsigned char *out, unsigned short capacity);
#endif
