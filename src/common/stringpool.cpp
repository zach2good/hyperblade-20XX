#include "compat.h"
#include "globals.h"
#include "stringpool.h"
#ifdef HYPERX_V100
#include <string.h>
#endif

#define STRING_POOL_BUFFER_SIZE 0x100

// The count is clamped up to two and every slot gets a fixed 256-byte buffer.

// FUNCTION: HYPERX100 0x00446340
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

    if (apBuffers != 0)
    {
        for (short i = 0; i < wCount; i++)
        {
            apBuffers[i] = new char[STRING_POOL_BUFFER_SIZE];
        }
    }
}

// FUNCTION: HYPERX100 0x00446430
// FUNCTION: HYPERX 0x00480af0
StringPool::~StringPool()
{
    if (apBuffers != 0)
    {
        for (short nSlot = 0; nSlot < wCount; nSlot++)
        {
            delete[] apBuffers[nSlot];
        }

        delete[] apBuffers;

        apBuffers = 0;
    }
}

// Reads a string resource into the next buffer of the ring and answers it.

// FUNCTION: HYPERX100 0x004464f0
// FUNCTION: HYPERX 0x00480b80
char* StringPool::LoadText(HINSTANCE hInstance, unsigned int uID)
{
    if (wIndex >= wCount)
    {
        wIndex = 0;
    }

    LoadStringA(hInstance, uID, apBuffers[wIndex], STRING_POOL_BUFFER_SIZE);

    return apBuffers[wIndex++];
}

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x00446550
char* StringPool::LoadTextCopy(HINSTANCE hInstance, unsigned int uID)
{
    if (wIndex >= wCount)
    {
        wIndex = 0;
    }

    const int   nLength = LoadStringA(hInstance, uID, apBuffers[wIndex], STRING_POOL_BUFFER_SIZE);
    char* const pCopy   = new char[nLength + 1];

    if (pCopy != 0)
    {
        strcpy(pCopy, apBuffers[wIndex]);
    }

    return pCopy;
}
#endif
