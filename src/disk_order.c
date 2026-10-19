/* SPDX-License-Identifier: GPL-2.0-or-later
 * Sector pairing verified against DosOrder.readBlock in the AppleCommander
 * 1.3.5.13 library shipped with ADTPro 2.1.0.
 */
#include "disk_order.h"
#include <string.h>
int diskOrderBlock(const unsigned char *track, unsigned block, unsigned char *out)
{
    static const unsigned char first[] = {0, 13, 11, 9, 7, 5, 3, 1};
    static const unsigned char second[] = {14, 12, 10, 8, 6, 4, 2, 15};
    if (block >= 8) return 0;
    memcpy(out, track + first[block] * 256, 256);
    memcpy(out + 256, track + second[block] * 256, 256);
    return 1;
}
