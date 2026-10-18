#include "common/compat.h"
#include <cstdio>
#include "graphics/cgpalette.h"
#include "common/pathstring.h"

constexpr auto FLAG_HAS_TABLE = 0x80;
constexpr auto FLAG_SIZE_MASK = 0x07;

// FUNCTION: HYPERX 0x004496e0
int CGPalette::ReadGifScreen(GifScreen* pScreen, FILE* pFile)
{
    unsigned short nCount = 0;

    short nWidth  = 0;
    short nHeight = 0;

    if (fread(pScreen->aSignature, 3, 1, pFile) != 1 || fread(pScreen->aVersion, 3, 1, pFile) != 1 || fread(&nWidth, sizeof(nWidth), 1, pFile) != 1 || fread(&nHeight, sizeof(nHeight), 1, pFile) != 1)
    {
        return -1;
    }

    pScreen->nWidth  = nWidth;
    pScreen->nHeight = nHeight;

    pScreen->cFlags      = static_cast<unsigned char>(fgetc(pFile));
    pScreen->cBackground = static_cast<unsigned char>(fgetc(pFile));
    pScreen->cAspect     = static_cast<unsigned char>(fgetc(pFile));

    if ((pScreen->cFlags & FLAG_HAS_TABLE) != 0)
    {
        nCount = static_cast<unsigned short>(1 << ((pScreen->cFlags & FLAG_SIZE_MASK) + 1));

        for (unsigned short nLeft = 0; nLeft < nCount; nLeft++)
        {
            pScreen->aColours[nLeft][0] = static_cast<unsigned char>(fgetc(pFile));
            pScreen->aColours[nLeft][1] = static_cast<unsigned char>(fgetc(pFile));
            pScreen->aColours[nLeft][2] = static_cast<unsigned char>(fgetc(pFile));
        }
    }

    if (ferror(pFile))
    {
        return -1;
    }

    return nCount;
}
