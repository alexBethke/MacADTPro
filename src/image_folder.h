#ifndef MACADTPRO_IMAGE_FOLDER_H
#define MACADTPRO_IMAGE_FOLDER_H
#include <Types.h>
typedef struct {
    short volume;
    long directory;
} ImageFolder;
/* Resolve/create "disks" relative to the running application, never the CWD. */
OSErr imageFolderDefault(ImageFolder *folder);
#endif
