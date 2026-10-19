#ifndef MACADTPRO_DIRECTORY_H
#define MACADTPRO_DIRECTORY_H
#include "image_folder.h"
OSErr directoryPage(const ImageFolder *folder, const unsigned char *request,
                    unsigned short length, unsigned char *page, unsigned short *number);
/* Snapshot of the last displayed page for the client's indexed file picker. */
OSErr directorySelection(unsigned char clientPage, unsigned char clientRow, unsigned char *name);
/* Read filenames for the Macintosh UI. Each entry is a Pascal string. */
OSErr directoryFolderItems(const ImageFolder *folder, unsigned char names[][32],
                           unsigned maxNames, unsigned *count, Boolean *more);
#endif
