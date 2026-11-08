#ifndef HYPERBLADE_CGPALETTE_H
#define HYPERBLADE_CGPALETTE_H

#include "common/compat.h"
#include <stdio.h>
#include "common/staticassert.h"

// A GIF's logical screen descriptor and global colour table.
struct GifScreen
{
    char  aSignature[3]; // 0x000
    char  aVersion[3];   // 0x003
    short nWidth;        // 0x006
    short nHeight;       // 0x008
    // Bit 7 flags a colour table of 2^(n+1) entries, n the low three bits.
    unsigned char cFlags;           // 0x00a
    unsigned char cBackground;      // 0x00b
    unsigned char cAspect;          // 0x00c
    unsigned char aColours[256][3]; // 0x00d, red, green and blue
};

STATIC_ASSERT(sizeof(GifScreen) == 0x30e);

#ifdef HYPERX_V100
#pragma pack(push, 1)

// The image descriptor and local colour table. The readers skip the leading separator or labels.
struct GifImage
{
    char          cSeparator;       // 0x000
    short         nLeft;            // 0x001
    short         nTop;             // 0x003
    short         nWidth;           // 0x005
    short         nHeight;          // 0x007
    unsigned char cFlags;           // 0x009
    unsigned char aColours[256][3]; // 0x00a
};

struct GifControl
{
    char          cIntroducer;  // 0x00
    char          cLabel;       // 0x01
    unsigned char cBlockSize;   // 0x02
    unsigned char cFlags;       // 0x03
    short         nDelay;       // 0x04
    unsigned char cTransparent; // 0x06
};

struct GifPlainText
{
    char          cIntroducer; // 0x00
    char          cLabel;      // 0x01
    unsigned char cBlockSize;  // 0x02
    short         nLeft;       // 0x03
    short         nTop;        // 0x05
    short         nWidth;      // 0x07
    short         nHeight;     // 0x09
    unsigned char cCellWidth;  // 0x0b
    unsigned char cCellHeight; // 0x0c
    unsigned char cForeground; // 0x0d
    unsigned char cBackground; // 0x0e
    char*         pData;       // 0x0f
    char          cTerminator; // 0x13
};

struct GifApplication
{
    char          cIntroducer;    // 0x00
    char          cLabel;         // 0x01
    unsigned char cBlockSize;     // 0x02
    char          aIdentifier[8]; // 0x03
    char          aAuthCode[3];   // 0x0b
    char*         pData;          // 0x0e
    char          cTerminator;    // 0x12
};

struct GifComment
{
    char  cIntroducer; // 0x00
    char  cLabel;      // 0x01
    char* pData;       // 0x02
    char  cTerminator; // 0x06
};

#pragma pack(pop)

STATIC_ASSERT(sizeof(GifImage) == 0x30a);
STATIC_ASSERT(sizeof(GifPlainText) == 0x14);
STATIC_ASSERT(sizeof(GifApplication) == 0x13);
#endif

namespace CGPalette
{

// Answers the number of colours in the table, or -1 on a read error.
int __cdecl ReadGifScreen(GifScreen* pScreen, FILE* pFile);

#ifdef HYPERX_V100
int __cdecl   ReadGifImage(GifImage* pImage, FILE* pFile);
int __cdecl   ReadGifControl(GifControl* pControl, FILE* pFile);
int __cdecl   ReadGifPlainText(GifPlainText* pText, FILE* pFile);
int __cdecl   ReadGifApplication(GifApplication* pApplication, FILE* pFile);
int __cdecl   ReadGifComment(GifComment* pComment, FILE* pFile);
char* __cdecl ReadGifSubBlocks(FILE* pFile);
#endif

} // namespace CGPalette

#endif
