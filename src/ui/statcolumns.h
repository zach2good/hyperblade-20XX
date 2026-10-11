// Row 41 is a percentage and row 44 a time in milliseconds shown as minutes and seconds.

#ifndef HYPERBLADE_STATCOLUMNS_H
#define HYPERBLADE_STATCOLUMNS_H

#include <stdio.h>

namespace StatColumns
{

inline void FormatClock(char* szText, short nValue)
{
    short nMinutes;
    short nSeconds;

    nMinutes = nValue / 60;
    nSeconds = nValue % 60;

    if (nSeconds < 10)
    {
        // STRING: HYPERX 0x004c3c84
        sprintf(szText, "%2hd:0%hd", nMinutes, nSeconds);
    }
    else
    {
        // STRING: HYPERX 0x004c3c78
        sprintf(szText, "%2hd:%2hd", nMinutes, nSeconds);
    }
}

} // namespace StatColumns

#endif
