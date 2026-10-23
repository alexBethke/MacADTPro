#ifndef MACADTPRO_PREFERENCES_H
#define MACADTPRO_PREFERENCES_H

#include <Types.h>

typedef struct {
    short printerPort;
    short baudIndex;
    short imageWriterCable;
} Preferences;

enum { kPreferencesFileSize = 12 };

void preferencesDefaults(Preferences *preferences);
Boolean preferencesDecode(Preferences *preferences,
                          const unsigned char data[kPreferencesFileSize]);
void preferencesEncode(const Preferences *preferences,
                       unsigned char data[kPreferencesFileSize]);
OSErr preferencesLoad(Preferences *preferences);
OSErr preferencesSave(const Preferences *preferences);

#endif
