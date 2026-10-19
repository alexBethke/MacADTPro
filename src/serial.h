#ifndef MACADTPRO_SERIAL_H
#define MACADTPRO_SERIAL_H
#include <Types.h>
#include <Files.h>
typedef struct {
    short input, output;
    Ptr receiveBuffer;
    Boolean receiveBufferInstalled;
    unsigned char sendBuffer[40970] __attribute__((aligned(2)));
    ParamBlockRec writePB;
    Boolean writing;
    unsigned long writeStarted;
    unsigned long writeTimeout;
    unsigned long baud;
    long sentBytes;
    unsigned char driverErrors;
    Boolean ctsHeld, dtrHeld;
} SerialConnection;
OSErr serialOpen(SerialConnection *serial, short printer, short baudIndex,
                 short handshake);
OSErr serialClose(SerialConnection *serial);
OSErr serialAbortWrite(SerialConnection *serial);
OSErr serialRead(SerialConnection *serial, unsigned char *bytes, long *count);
OSErr serialSend(SerialConnection *serial, const unsigned char *bytes, long count);
OSErr serialPollWrite(SerialConnection *serial);
OSErr serialDiagnostics(SerialConnection *serial);
#endif
