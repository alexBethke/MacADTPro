#ifndef MACADTPRO_PREFERENCES_H
#define MACADTPRO_PREFERENCES_H

#include <Types.h>

typedef struct {
    short printerPort;
    short baudIndex;
    short imageWriterCable;
    Boolean debugMode;
} Preferences;

enum { kPreferencesFileSize = 13 };

#define kDebugModeOffset 10

void preferencesDefaults(Preferences *preferences);
Boolean preferencesDecode(Preferences *preferences,
                          const unsigned char data[kPreferencesFileSize]);
void preferencesEncode(const Preferences *preferences,
                       unsigned char data[kPreferencesFileSize]);
OSErr preferencesLoad(Preferences *preferences);
OSErr preferencesSave(const Preferences *preferences);

#endif
