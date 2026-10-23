#include "preferences.h"
#include <Errors.h>
#include <Files.h>
#include <Folders.h>
#include <Script.h>

static const unsigned char kPreferencesName[] =
    {15, 'M', 'a', 'c', 'A', 'D', 'T', 'P', 'r', 'o', ' ', 'P', 'r', 'e', 'f', 's'};

static OSErr preferencesSpec(FSSpec *spec)
{
    short volume;
    long directory;
    OSErr error = FindFolder(kOnSystemDisk, kPreferencesFolderType, true,
                             &volume, &directory);
    if (error) return error;
    error = FSMakeFSSpec(volume, directory, kPreferencesName, spec);
    return error == fnfErr ? noErr : error;
}

OSErr preferencesLoad(Preferences *preferences)
{
    unsigned char data[kPreferencesFileSize];
    FSSpec spec;
    short refNum;
    long count = sizeof(data), eof;
    OSErr error;
    preferencesDefaults(preferences);
    error = preferencesSpec(&spec);
    if (error) return error;
    error = FSpOpenDF(&spec, fsRdPerm, &refNum);
    if (error == fnfErr) return noErr;
    if (error) return error;
    error = GetEOF(refNum, &eof);
    if (!error && eof != sizeof(data)) error = paramErr;
    if (!error) error = FSRead(refNum, &count, data);
    if (!error && count != sizeof(data)) error = eofErr;
    if (!error && !preferencesDecode(preferences, data)) error = paramErr;
    {
        OSErr closeError = FSClose(refNum);
        if (!error) error = closeError;
    }
    if (error) preferencesDefaults(preferences);
    return error;
}

OSErr preferencesSave(const Preferences *preferences)
{
    unsigned char data[kPreferencesFileSize];
    FSSpec spec;
    short refNum;
    long count = sizeof(data);
    OSErr error = preferencesSpec(&spec);
    if (error) return error;
    error = FSpCreate(&spec, FOUR_CHAR_CODE('MADP'), FOUR_CHAR_CODE('pref'),
                      smSystemScript);
    if (error != noErr && error != dupFNErr) return error;
    error = FSpOpenDF(&spec, fsRdWrPerm, &refNum);
    if (error) return error;
    preferencesEncode(preferences, data);
    error = SetEOF(refNum, 0);
    if (!error) error = FSWrite(refNum, &count, data);
    if (!error && count != sizeof(data)) error = dskFulErr;
    {
        OSErr closeError = FSClose(refNum);
        if (!error) error = closeError;
    }
    return error;
}
