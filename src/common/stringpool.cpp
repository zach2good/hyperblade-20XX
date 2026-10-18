#include "compat.h"
#include "stringtable.h"
#include <cstdio>
#include "globals.h"
#include "stringpool.h"

constexpr auto kStringPoolBufferSize = 0x100;

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

    apBuffers = new char*[nCount + 1];

    if (apBuffers != nullptr)
    {
        for (short i = 0; i < wCount; i++)
        {
            apBuffers[i] = new char[kStringPoolBufferSize];
        }
    }
}

// FUNCTION: HYPERX 0x00480af0
StringPool::~StringPool()
{
    if (apBuffers != nullptr)
    {
        for (short nSlot = 0; nSlot < wCount; nSlot++)
        {
            delete[] apBuffers[nSlot];
        }

        delete[] apBuffers;

        apBuffers = nullptr;
    }
}

// Copies a string into the next buffer of the ring and answers it.

// FUNCTION: HYPERX 0x00480b80
char* StringPool::LoadText(unsigned int uID)
{
    if (wIndex >= wCount)
    {
        wIndex = 0;
    }

    snprintf(apBuffers[wIndex], kStringPoolBufferSize, "%s", StringTable::Find(uID));

    return apBuffers[wIndex++];
}
