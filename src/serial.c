#include "serial.h"
#include <Serial.h>
#include <Devices.h>
#include <OSUtils.h>
#include <MacMemory.h>
#include <Errors.h>
#include <string.h>
#include <stddef.h>

_Static_assert(offsetof(SerialConnection, sendBuffer) % 2 == 0,
               "Serial transmit buffer must be word aligned");
_Static_assert(offsetof(SerialConnection, writePB) % 2 == 0,
               "Serial parameter block must be word aligned");

/* Some interface-library glue leaves Device Manager PB fields uninitialized.
   Use fully cleared blocks for every operation, including synchronous calls. */
static OSErr portControl(short ref, short code, const void *params, unsigned size)
{
    ParamBlockRec pb;
    memset(&pb, 0, sizeof(pb));
    pb.cntrlParam.ioCRefNum = ref;
    pb.cntrlParam.csCode = code;
    if (size > sizeof(pb.cntrlParam.csParam)) return paramErr;
    if (size) memcpy(pb.cntrlParam.csParam, params, size);
    return PBControlSync(&pb);
}

static OSErr portBuffer(short ref, Ptr buffer, short length)
{
    ParamBlockRec pb;
    memset(&pb, 0, sizeof(pb));
    pb.cntrlParam.ioCRefNum = ref;
    pb.cntrlParam.csCode = kSERDInputBuffer;
    memcpy(pb.cntrlParam.csParam, &buffer, sizeof(buffer));
    pb.cntrlParam.csParam[2] = length;
    return PBControlSync(&pb);
}

static OSErr portClose(short ref)
{
    ParamBlockRec pb;
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = ref;
    return PBCloseSync(&pb);
}

static OSErr portOpen(const unsigned char *name, short *ref)
{
    ParamBlockRec pb;
    OSErr error;
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioNamePtr = (StringPtr)name;
    error = PBOpenSync(&pb);
    *ref = error ? 0 : pb.ioParam.ioRefNum;
    return error;
}

OSErr serialAbortWrite(SerialConnection *serial)
{
    ParamBlockRec pb;
    OSErr error;
    if (!serial->writing) return noErr;
    if (serial->writePB.ioParam.ioResult <= 0) {
        serial->writing = false;
        return noErr;
    }
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = serial->output;
    error = PBKillIOSync(&pb);
    if (!error && serial->writePB.ioParam.ioResult > 0) return ioErr;
    if (!error) serial->writing = false;
    return error;
}

OSErr serialClose(SerialConnection *serial)
{
    ParamBlockRec pb;
    OSErr error, firstError = noErr;
    memset(&pb, 0, sizeof(pb));
    if (serial->output) {
        error = serialAbortWrite(serial);
        if (error) firstError = error;
    }
    if (serial->input) {
        memset(&pb, 0, sizeof(pb));
        pb.ioParam.ioRefNum = serial->input;
        error = PBKillIOSync(&pb);
        if (error && !firstError) firstError = error;
        if (serial->receiveBufferInstalled) {
            error = portBuffer(serial->input, 0, 0);
            if (error) { if (!firstError) firstError = error; }
            else serial->receiveBufferInstalled = false;
        }
        error = portClose(serial->input);
        if (error) { if (!firstError) firstError = error; }
        else { serial->input = 0; serial->receiveBufferInstalled = false; }
    }
    if (serial->output) {
        error = portClose(serial->output);
        if (error) { if (!firstError) firstError = error; }
        else { serial->output = 0; serial->writing = false; }
    }
    if (serial->receiveBuffer && !serial->input && !serial->output && !serial->receiveBufferInstalled) {
        DisposePtr(serial->receiveBuffer);
        serial->receiveBuffer = 0;
    }
    return firstError;
}

OSErr serialOpen(SerialConnection *serial, short printer, short baudIndex,
                 short handshake)
{
    static const unsigned char aIn[] = {4, '.', 'A', 'I', 'n'};
    static const unsigned char aOut[] = {5, '.', 'A', 'O', 'u', 't'};
    static const unsigned char bIn[] = {4, '.', 'B', 'I', 'n'};
    static const unsigned char bOut[] = {5, '.', 'B', 'O', 'u', 't'};
    SerShk flags;
    OSErr error;
    if (baudIndex < 0 || baudIndex >= 5) return paramErr;
    error = serialClose(serial);
    if (error) return error;
    /* Driver interrupt code must not depend on an application data segment. */
    serial->receiveBuffer = NewPtrSysClear(8192);
    if (!serial->receiveBuffer) return memFullErr;
    error = portOpen(printer ? bIn : aIn, &serial->input);
    if (error) { serial->input = 0; goto failed; }
    error = portOpen(printer ? bOut : aOut, &serial->output);
    if (error) { serial->output = 0; goto failed; }
    error = serialSetBaud(serial, baudIndex);
    if (error) goto failed;
    error = portBuffer(serial->input, serial->receiveBuffer, 8192);
    if (error) goto failed;
    serial->receiveBufferInstalled = true;
    memset(&flags, 0, sizeof(flags));
    /* ImageWriter profile: CTS output gating and DTR receive flow control. */
    flags.fCTS = handshake ? 1 : 0;
    flags.fDTR = handshake ? 1 : 0;
    {
        ParamBlockRec controlPB;
        memset(&controlPB, 0, sizeof(controlPB));
        controlPB.cntrlParam.ioCRefNum = serial->output;
        /* Restore internal clocking; do not inherit another app's options. */
        controlPB.cntrlParam.csCode = kSERDMiscOptions;
        error = PBControlSync(&controlPB);
        if (error) goto failed;
        memset(controlPB.cntrlParam.csParam, 0, sizeof(controlPB.cntrlParam.csParam));
        memcpy(controlPB.cntrlParam.csParam, &flags, sizeof(flags));
        controlPB.cntrlParam.csCode = kSERDHandshake;
        error = PBControlSync(&controlPB);
        if (error) goto failed;
    }
    /* Ensure the IIc sees the host ready on cables carrying HSKo. */
    {
        ParamBlockRec controlPB;
        memset(&controlPB, 0, sizeof(controlPB));
        controlPB.cntrlParam.ioCRefNum = serial->output;
        controlPB.cntrlParam.csCode = kSERDAssertDTR;
        error = PBControlSync(&controlPB);
    }
    if (error) goto failed;
    serial->writeTimeout = 600;
    { static const unsigned long speeds[] = {300, 1200, 2400, 9600, 19200};
      serial->baud = speeds[baudIndex]; }
    serial->sentBytes = 0;
    serial->driverErrors = 0;
    serial->ctsHeld = serial->dtrHeld = false;
    return noErr;
failed:
    serialClose(serial);
    return error;
}

OSErr serialSetBaud(SerialConnection *serial, short baudIndex)
{
    static const short baud[] = {baud300, baud1200, baud2400, baud9600, baud19200};
    static const unsigned long speeds[] = {300, 1200, 2400, 9600, 19200};
    short configuration;
    OSErr error;
    if (!serial->output || baudIndex < 0 || baudIndex >= 5) return paramErr;
    configuration = baud[baudIndex] | data8 | stop10 | noParity;
    error = portControl(serial->output, kSERDConfiguration, &configuration,
                        sizeof(configuration));
    if (!error) serial->baud = speeds[baudIndex];
    return error;
}

OSErr serialDiagnostics(SerialConnection *serial)
{
    ParamBlockRec pb;
    unsigned char *status;
    OSErr error;
    if (!serial->input || !serial->output) return paramErr;
    memset(&pb, 0, sizeof(pb));
    pb.cntrlParam.ioCRefNum = serial->output;
    pb.cntrlParam.csCode = kSERDStatus;
    error = PBStatusSync(&pb);
    if (error) return error;
    status = (unsigned char *)pb.cntrlParam.csParam;
    serial->ctsHeld = status[4] != 0;
    serial->dtrHeld = (status[1] & 0x40) != 0;
    serial->driverErrors |= status[0];
    memset(&pb, 0, sizeof(pb));
    pb.cntrlParam.ioCRefNum = serial->input;
    pb.cntrlParam.csCode = kSERDStatus;
    error = PBStatusSync(&pb);
    if (error) return error;
    serial->driverErrors |= ((unsigned char *)pb.cntrlParam.csParam)[0];
    return noErr;
}

OSErr serialRead(SerialConnection *serial, unsigned char *bytes, long *count)
{
    ParamBlockRec pb;
    long available;
    OSErr error;
    memset(&pb, 0, sizeof(pb));
    pb.cntrlParam.ioCRefNum = serial->input;
    pb.cntrlParam.csCode = kSERDInputCount;
    error = PBStatusSync(&pb);
    if (error) return error;
    memcpy(&available, pb.cntrlParam.csParam, sizeof(available));
    if (available > *count) available = *count;
    *count = 0;
    if (!available) return noErr;
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = serial->input;
    pb.ioParam.ioBuffer = (Ptr)bytes;
    pb.ioParam.ioReqCount = available;
    error = PBReadSync(&pb);
    *count = pb.ioParam.ioActCount;
    return error;
}

OSErr serialSend(SerialConnection *serial, const unsigned char *bytes, long count)
{
    OSErr error;
    /* The peer can respond after the driver completes but before the event loop
       has observed that completion. Reap it here before declaring output busy. */
    if (serial->writing) {
        error = serialPollWrite(serial);
        if (error) return error;
    }
    if (serial->writing || count < 1 || (unsigned long)count > sizeof(serial->sendBuffer))
        return paramErr;
    memcpy(serial->sendBuffer, bytes, count);
    memset(&serial->writePB, 0, sizeof(serial->writePB));
    serial->writePB.ioParam.ioRefNum = serial->output;
    serial->writePB.ioParam.ioBuffer = (Ptr)serial->sendBuffer;
    serial->writePB.ioParam.ioReqCount = count;
    serial->writing = true;
    serial->writeStarted = TickCount();
    serial->writeTimeout = 7200 + (unsigned long)count * 600 / serial->baud;
    error = PBWriteAsync(&serial->writePB);
    if (error) {
        serial->writing = false;
        return error;
    }
    return noErr;
}

OSErr serialPollWrite(SerialConnection *serial)
{
    if (!serial->writing) return noErr;
    if (serial->writePB.ioParam.ioResult > 0) {
        if ((unsigned long)(TickCount() - serial->writeStarted) > serial->writeTimeout)
            return -1; /* Output held by CTS, or driver failed to complete. */
        return noErr;
    }
    serial->writing = false;
    serial->sentBytes += serial->writePB.ioParam.ioActCount;
    return serial->writePB.ioParam.ioResult;
}
