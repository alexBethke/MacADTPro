#include "image.h"
#include "image_size.h"
#include "disk_order.h"
#include <Files.h>
#include <Devices.h>
#include <Errors.h>
#include <string.h>
#include <ctype.h>

OSErr imageClose(DiskImage *image)
{
    ParamBlockRec pb;
    OSErr error;
    if (!image->ref) return noErr;
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = image->ref;
    error = PBCloseSync(&pb);
    if (!error) image->ref = 0;
    return error;
}

unsigned char imageOpen(const ImageFolder *folder, const unsigned char *name, DiskImage *image)
{
    HParamBlockRec openPB;
    ParamBlockRec sizePB;
    long length;
    unsigned i;
    if (imageClose(image)) return 2;
    image->blocks = 0;
    image->cachedTrack = -1;
    image->dosOrder = false;
    if (!name[0] || name[0] > 31) return 2;
    for (i = 1; i <= name[0]; ++i)
        if (name[i] == ':' || name[i] == '/' || name[i] < 32) return 2;
    memset(&openPB, 0, sizeof(openPB));
    openPB.fileParam.ioNamePtr = (StringPtr)name;
    openPB.fileParam.ioVRefNum = folder->volume;
    openPB.fileParam.ioDirID = folder->directory;
    openPB.ioParam.ioPermssn = fsRdPerm;
    if (PBHOpenDFSync(&openPB)) return 2;
    image->ref = openPB.ioParam.ioRefNum;
    {
        char extension[5];
        unsigned dot = name[0];
        while (dot && name[dot] != '.') --dot;
        if (!dot || name[0] - dot > 3) {
            imageClose(image);
            return 4;
        }
        for (i = dot + 1; i <= name[0]; ++i) extension[i - dot - 1] = toupper(name[i]);
        extension[name[0] - dot] = 0;
        if (!strcmp(extension, "DSK") || !strcmp(extension, "DO")) image->dosOrder = true;
        else if (strcmp(extension, "PO")) {
            imageClose(image);
            return 4;
        }
    }
    memset(&sizePB, 0, sizeof(sizePB));
    sizePB.ioParam.ioRefNum = image->ref;
    if (PBGetEOFSync(&sizePB)) { imageClose(image); return 2; }
    length = sizePB.ioParam.ioMisc ? (long)sizePB.ioParam.ioMisc : 0;
    image->blocks = imageBlocksForLength(length, image->dosOrder);
    if (!image->blocks) {
        imageClose(image); return 4;
    }
    return 0;
}

static OSErr readBytes(DiskImage *image, long offset, long count, unsigned char *data)
{
    ParamBlockRec pb;
    OSErr error;
    memset(&pb, 0, sizeof(pb));
    pb.ioParam.ioRefNum = image->ref;
    pb.ioParam.ioBuffer = (Ptr)data;
    pb.ioParam.ioReqCount = count;
    pb.ioParam.ioPosMode = fsFromStart;
    pb.ioParam.ioPosOffset = offset;
    error = PBReadSync(&pb);
    if (!error && pb.ioParam.ioActCount != pb.ioParam.ioReqCount) return eofErr;
    return error;
}

OSErr imageRead(DiskImage *image, unsigned short start, unsigned short blocks, unsigned char *data)
{
    unsigned short i;
    OSErr error;
    if (!image->ref || !blocks || (unsigned long)start + blocks > image->blocks) return paramErr;
    if (!image->dosOrder) return readBytes(image, (long)start * 512, (long)blocks * 512, data);
    for (i = 0; i < blocks; ++i) {
        unsigned block = start + i;
        long track = block / 8;
        if (image->cachedTrack != track) {
            image->cachedTrack = -1;
            error = readBytes(image, track * 4096, 4096, image->track);
            if (error) return error;
            image->cachedTrack = track;
        }
        diskOrderBlock(image->track, block % 8, data + (long)i * 512);
    }
    return noErr;
}
