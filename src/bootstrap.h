#ifndef MACADTPRO_BOOTSTRAP_H
#define MACADTPRO_BOOTSTRAP_H

#include "serial.h"

typedef struct {
    Handle text;
    unsigned long offset;
    unsigned long nextLine;
    unsigned short textLine;
    unsigned char state;
    char message[160];
} Bootstrap;

enum { BOOTSTRAP_IDLE, BOOTSTRAP_TEXT, BOOTSTRAP_WAIT_PRODOS,
       BOOTSTRAP_SEND_PRODOS, BOOTSTRAP_WAIT_CLIENT,
       BOOTSTRAP_SEND_CLIENT, BOOTSTRAP_DONE };

OSErr bootstrapStart(Bootstrap *bootstrap);
OSErr bootstrapPoll(Bootstrap *bootstrap, SerialConnection *serial);
Boolean bootstrapCommand(Bootstrap *bootstrap, SerialConnection *serial,
                         unsigned char command, const unsigned char *payload,
                         unsigned short length, OSErr *error);
void bootstrapCancel(Bootstrap *bootstrap);
Boolean bootstrapActive(const Bootstrap *bootstrap);

#endif
