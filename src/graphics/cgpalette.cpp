#include "common/compat.h"
#include <stdio.h>
#include "graphics/cgpalette.h"
#include "common/pathstring.h"
#ifdef HYPERX_V100
#include <stdlib.h>
#endif

#define FLAG_HAS_TABLE 0x80
#define FLAG_SIZE_MASK 0x07

// FUNCTION: HYPERX100 0x00499fd0
// FUNCTION: HYPERX 0x004496e0
int __cdecl CGPalette::ReadGifScreen(GifScreen* pScreen, FILE* pFile)
{
    unsigned short nCount = 0;

    fread(pScreen->aSignature, 3, 1, pFile);
    fread(pScreen->aVersion, 3, 1, pFile);

    short nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pScreen->nWidth = nWord;

    nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pScreen->nHeight = nWord;

    pScreen->cFlags      = (unsigned char)fgetc(pFile);
    pScreen->cBackground = (unsigned char)fgetc(pFile);
    pScreen->cAspect     = (unsigned char)fgetc(pFile);

    if ((pScreen->cFlags & FLAG_HAS_TABLE) != 0)
    {
        nCount = (unsigned short)(1 << ((pScreen->cFlags & FLAG_SIZE_MASK) + 1));

        for (unsigned short nLeft = 0; nLeft < nCount; nLeft++)
        {
            pScreen->aColours[nLeft][0] = (unsigned char)fgetc(pFile);
            pScreen->aColours[nLeft][1] = (unsigned char)fgetc(pFile);
            pScreen->aColours[nLeft][2] = (unsigned char)fgetc(pFile);
        }
    }

    if (ferror(pFile))
    {
        return -1;
    }

    return nCount;
}

#ifdef HYPERX_V100
// The block readers answer -1 on a read error, and 1 when a data buffer cannot be allocated.

// FUNCTION: HYPERX100 0x0049a130
int __cdecl CGPalette::ReadGifImage(GifImage* pImage, FILE* pFile)
{
    short nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pImage->nLeft = nWord;

    nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pImage->nTop = nWord;

    nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pImage->nWidth = nWord;

    nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pImage->nHeight = nWord;

    pImage->cFlags = (unsigned char)fgetc(pFile);

    if ((pImage->cFlags & FLAG_HAS_TABLE) != 0)
    {
        const unsigned short nCount = (unsigned short)(1 << ((pImage->cFlags & FLAG_SIZE_MASK) + 1));

        for (unsigned short i = 0; i < nCount; i++)
        {
            pImage->aColours[i][0] = (unsigned char)fgetc(pFile);
            pImage->aColours[i][1] = (unsigned char)fgetc(pFile);
            pImage->aColours[i][2] = (unsigned char)fgetc(pFile);
        }
    }

    if (ferror(pFile))
    {
        return -1;
    }

    return 0;
}

// FUNCTION: HYPERX100 0x0049a280
int __cdecl CGPalette::ReadGifControl(GifControl* pControl, FILE* pFile)
{
    pControl->cBlockSize = (unsigned char)fgetc(pFile);
    pControl->cFlags     = (unsigned char)fgetc(pFile);

    short nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pControl->nDelay = nWord;

    pControl->cTransparent = (unsigned char)fgetc(pFile);

    if (ferror(pFile))
    {
        return -1;
    }

    return 0;
}

// FUNCTION: HYPERX100 0x0049a300
int __cdecl CGPalette::ReadGifPlainText(GifPlainText* pText, FILE* pFile)
{
    pText->cBlockSize = (unsigned char)fgetc(pFile);

    short nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pText->nLeft = nWord;

    nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pText->nTop = nWord;

    nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pText->nWidth = nWord;

    nWord = 0;
    fread(&nWord, sizeof(nWord), 1, pFile);
    pText->nHeight = nWord;

    pText->cCellWidth  = (unsigned char)fgetc(pFile);
    pText->cCellHeight = (unsigned char)fgetc(pFile);
    pText->cForeground = (unsigned char)fgetc(pFile);
    pText->cBackground = (unsigned char)fgetc(pFile);

    pText->pData = CGPalette::ReadGifSubBlocks(pFile);

    if (pText->pData == 0)
    {
        return 1;
    }

    pText->cTerminator = 0;

    if (ferror(pFile))
    {
        return -1;
    }

    return 0;
}

// FUNCTION: HYPERX100 0x0049a440
int __cdecl CGPalette::ReadGifApplication(GifApplication* pApplication, FILE* pFile)
{
    pApplication->cBlockSize = (unsigned char)fgetc(pFile);

    for (int i = 0; i < 8; i++)
    {
        pApplication->aIdentifier[i] = (char)fgetc(pFile);
    }

    for (int j = 0; j < 3; j++)
    {
        pApplication->aAuthCode[j] = (char)fgetc(pFile);
    }

    pApplication->pData = CGPalette::ReadGifSubBlocks(pFile);

    if (pApplication->pData == 0)
    {
        return 1;
    }

    pApplication->cTerminator = 0;

    if (ferror(pFile))
    {
        return -1;
    }

    return 0;
}

// FUNCTION: HYPERX100 0x0049a540
int __cdecl CGPalette::ReadGifComment(GifComment* pComment, FILE* pFile)
{
    pComment->pData = CGPalette::ReadGifSubBlocks(pFile);

    if (pComment->pData == 0)
    {
        return 1;
    }

    pComment->cTerminator = 0;

    if (ferror(pFile))
    {
        return -1;
    }

    return 0;
}

// Keeps each sub-block's length byte. Every grow asks for only the next block's size, not the total.
// FUNCTION: HYPERX100 0x0049a580
char* __cdecl CGPalette::ReadGifSubBlocks(FILE* pFile)
{
    unsigned short nTotal = 0;
    unsigned char  nCount = (unsigned char)fgetc(pFile);
    char*          pData  = (char*)malloc(nCount + 1);

    if (pData == 0)
    {
        return 0;
    }

    char* p = pData;

    for (;;)
    {
        *p++ = (char)nCount;
        nTotal += nCount + 1;

        while (nCount-- != 0)
        {
            *p++ = (char)fgetc(pFile);
        }

        nCount = (unsigned char)fgetc(pFile);

        if (nCount == 0)
        {
            break;
        }

        pData = (char*)realloc(pData, nCount + 1);

        if (pData == 0)
        {
            return 0;
        }

        p = pData + nTotal;
    }

    *p = 0;

    return pData;
}
#endif
