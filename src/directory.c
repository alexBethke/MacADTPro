/* SPDX-License-Identifier: GPL-2.0-or-later
 * Directory layout follows ADTPro CommsThread.sendDirectoryWide,
 * Copyright (C) 2006-2020 David Schmidt. HFS adapter added for MacADTPro.
 */
#include "directory.h"
#include <Files.h>
#include <Errors.h>
#include <string.h>
#include <ctype.h>
static unsigned char displayedNames[18][32];
static unsigned short displayedPage;
static unsigned displayedCount;

OSErr directoryFolderItems(const ImageFolder *folder, unsigned char names[][32],
                           unsigned maxNames, unsigned *count, Boolean *more)
{
    CInfoPBRec catalog;
    unsigned char name[256];
    long index;
    OSErr error;
    *count = 0;
    *more = false;
    for (index = 1; index <= 32767; ++index) {
        memset(&catalog, 0, sizeof(catalog));
        catalog.hFileInfo.ioNamePtr = name;
        catalog.hFileInfo.ioVRefNum = folder->volume;
        catalog.hFileInfo.ioDirID = folder->directory;
        catalog.hFileInfo.ioFDirIndex = index;
        error = PBGetCatInfoSync(&catalog);
        if (error == fnfErr) return noErr;
        if (error) return error;
        if (catalog.hFileInfo.ioFlAttrib & 0x10) continue;
        if (name[0] > 31) continue;
        if (*count == maxNames) { *more = true; return noErr; }
        memcpy(names[*count], name, name[0] + 1);
        ++*count;
    }
    return noErr;
}

OSErr directorySelection(unsigned char clientPage, unsigned char clientRow, unsigned char *name)
{
    unsigned row;
    if (!clientPage || clientRow < 3) return paramErr;
    row = clientRow - 3;
    if (clientPage - 1 != displayedPage || row >= displayedCount) return paramErr;
    memcpy(name, displayedNames[row], 32);
    return noErr;
}

static int matches(const char *pattern, const char *name)
{
    const char *star = 0, *retry = 0;
    while (*name) {
        if (*pattern == '*' ) { star = ++pattern; retry = name; }
        else if (*pattern == '?' || toupper((unsigned char)*pattern) == toupper((unsigned char)*name))
            { ++pattern; ++name; }
        else if (star) { pattern = star; name = ++retry; }
        else return 0;
    }
    while (*pattern == '*') ++pattern;
    return !*pattern;
}

OSErr directoryPage(const ImageFolder *folder, const unsigned char *request,
                    unsigned short length, unsigned char *page, unsigned short *number)
{
    CInfoPBRec catalog;
    unsigned char name[256];
    char pattern[256], filename[32];
    long index, matching = 0, start;
    unsigned offset = 0, j, rows = 0;
    OSErr error;
    const char *heading = "DIRECTORY OF: DISKS\r\r----------------------------------------";
    if (length < 2 || length > 256 || request[length - 2] != 0) return paramErr;
    *number = request[length - 1];
    displayedPage = *number;
    displayedCount = 0;
    start = (long)*number * 18;
    for (j = 0; j < (unsigned)length - 2; ++j) pattern[j] = request[j] & 0x7f;
    pattern[j] = 0;
    if (!pattern[0]) strcpy(pattern, "*");
    memset(page, 0, 1024);
    offset = strlen(heading);
    memcpy(page, heading, offset);
    for (index = 1; index <= 32767; ++index) {
        memset(&catalog, 0, sizeof(catalog));
        catalog.hFileInfo.ioNamePtr = name;
        catalog.hFileInfo.ioVRefNum = folder->volume;
        catalog.hFileInfo.ioDirID = folder->directory;
        catalog.hFileInfo.ioFDirIndex = index;
        error = PBGetCatInfoSync(&catalog);
        if (error == fnfErr) break;
        if (error) return error;
        if (catalog.hFileInfo.ioFlAttrib & 0x10) continue;
        if (name[0] > 31) continue;
        memcpy(filename, name + 1, name[0]); filename[name[0]] = 0;
        if (!matches(pattern, filename)) continue;
        if (matching++ < start) continue;
        if (rows == 18) { page[offset + 1] = 1; break; }
        memcpy(page + offset, filename, name[0]); offset += name[0];
        memcpy(displayedNames[rows], name, name[0] + 1);
        displayedCount = rows + 1;
        page[offset++] = '\r'; ++rows;
    }
    if (!rows) memcpy(page + offset, "NO FILES", 8);
    return noErr;
}
