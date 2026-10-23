#include <assert.h>
#include <string.h>
#include "preferences.h"

int main(void)
{
    Preferences input = {1, 2, 1}, output;
    unsigned char data[kPreferencesFileSize];
    preferencesDefaults(&output);
    assert(output.printerPort == 0 && output.baudIndex == 4 &&
           output.imageWriterCable == 0);
    preferencesEncode(&input, data);
    assert(preferencesDecode(&output, data));
    assert(output.printerPort == 1 && output.baudIndex == 2 &&
           output.imageWriterCable == 1);
    data[9] = 5;
    assert(!preferencesDecode(&output, data));
    preferencesEncode(&input, data);
    data[3] ^= 1;
    assert(!preferencesDecode(&output, data));
    preferencesEncode(&input, data);
    data[11] ^= 1;
    assert(!preferencesDecode(&output, data));
    return 0;
}
