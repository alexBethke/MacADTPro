#include "preferences.h"

enum { kPreferencesVersion = 1 };

void preferencesDefaults(Preferences *preferences)
{
    preferences->printerPort = 0;
    preferences->baudIndex = 4;
    preferences->imageWriterCable = 0;
    preferences->debugMode = 0;
}

void preferencesEncode(const Preferences *preferences,
                       unsigned char data[kPreferencesFileSize])
{
    unsigned char checksum = 0;
    short i;
    data[0] = 'M'; data[1] = 'A'; data[2] = 'D'; data[3] = 'P';
    data[4] = 0; data[5] = kPreferencesVersion;
    data[6] = 0; data[7] = 3;
    data[8] = (unsigned char)preferences->printerPort;
    data[9] = (unsigned char)preferences->baudIndex;
    data[10] = (unsigned char)preferences->imageWriterCable;
    data[11] = preferences->debugMode ? 1 : 0;
    for (i = 0; i < kPreferencesFileSize - 1; ++i) checksum ^= data[i];
    data[12] = checksum;
}

Boolean preferencesDecode(Preferences *preferences,
                          const unsigned char data[kPreferencesFileSize])
{
    unsigned char checksum = 0;
    short i;
    for (i = 0; i < kPreferencesFileSize - 1; ++i) checksum ^= data[i];
    if (data[0] != 'M' || data[1] != 'A' || data[2] != 'D' || data[3] != 'P' ||
        data[4] != 0 || data[5] != kPreferencesVersion ||
        data[6] != 0 || data[7] != 3 || data[12] != checksum ||
        data[8] > 1 || data[9] > 4 || data[10] > 1)
        return false;
    preferences->printerPort = data[8];
    preferences->baudIndex = data[9];
    preferences->imageWriterCable = data[10];
    preferences->debugMode = (data[11] != 0);
    return true;
}
