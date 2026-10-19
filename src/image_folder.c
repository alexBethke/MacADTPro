#include "image_folder.h"
#include <Files.h>
#include <Processes.h>
#include <Errors.h>
#include <string.h>

OSErr imageFolderDefault(ImageFolder *folder)
{
    static const unsigned char name[] = {5, 'd', 'i', 's', 'k', 's'};
    ProcessSerialNumber process;
    ProcessInfoRec info;
    FSSpec app;
    CInfoPBRec catalog;
    OSErr error;
    long directory;

    memset(folder, 0, sizeof(*folder));
    memset(&info, 0, sizeof(info));
    info.processInfoLength = sizeof(info);
    info.processAppSpec = &app;
    error = GetCurrentProcess(&process);
    if (error) return error;
    error = GetProcessInformation(&process, &info);
    if (error) return error;

    memset(&catalog, 0, sizeof(catalog));
    catalog.hFileInfo.ioNamePtr = (StringPtr)name;
    catalog.hFileInfo.ioVRefNum = app.vRefNum;
    catalog.hFileInfo.ioDirID = app.parID;
    catalog.hFileInfo.ioFDirIndex = 0;
    error = PBGetCatInfoSync(&catalog);
    if (error == fnfErr) {
        error = DirCreate(app.vRefNum, app.parID, name, &directory);
        if (error) return error;
    } else {
        if (error) return error;
        /* A file named disks is a conflict; leave it untouched. */
        if (!(catalog.hFileInfo.ioFlAttrib & 0x10)) return dupFNErr;
        directory = catalog.dirInfo.ioDrDirID;
    }
    folder->volume = app.vRefNum;
    folder->directory = directory;
    return noErr;
}
