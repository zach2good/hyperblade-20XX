#include "compat.h"
#include "stringtable.h"
#include <cstdio>
#include "globals.h"
#include "stringpool.h"

// The count is clamped up to two and every slot gets a fixed 256-byte buffer.

// FUNCTION: HYPERX 0x00480a50
StringPool::StringPool(short nCount)
: wIndex(0)
{
    if (nCount < 2)
    {
        nCount = 2;
    }

    wCount = nCount;

    apBuffers.resize(wCount);
}

// Copies a string into the next buffer of the ring and answers it.

// FUNCTION: HYPERX 0x00480b80
char* StringPool::LoadText(unsigned int uID)
{
    if (wIndex >= wCount)
    {
        wIndex = 0;
    }

    snprintf(apBuffers[wIndex].data(), kBufferSize, "%s", StringTable::Find(uID));

    return apBuffers[wIndex++].data();
}
