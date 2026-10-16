// Text drawn straight into pixels, in place of GDI's TextOutA. Microsoft Sans Serif, the TrueType
// face of the MS Sans Serif the game asked GDI for, rasterised without antialiasing as the
// original's NONANTIALIASED_QUALITY drew it.

#ifndef HYPERBLADE_TEXT_H
#define HYPERBLADE_TEXT_H

#include "common/handle.h"
#include <cstdint>

namespace Text
{

using Font = Handle<struct TextFontTag>;

// The face at a cell height in pixels, as CreateFont took a positive height; null when the font
// file is missing.
Font Find(int nHeight);

struct Size
{
    int nWidth;
    int nHeight;
};

// As GetTextExtentPoint32 measured: the advance and the cell height.
Size Measure(Font font, const char* pszText);

// Draws with the cell's top left at the point. A pixel the glyph covers at least half of takes the
// colour; the rest is left as it was, as GDI's transparent background drew.
void Draw(Font font, unsigned char* pBits, int nPitch, int nWidth, int nHeight, int nX, int nY, const char* pszText, unsigned char nIndex);
void Draw(Font font, std::uint32_t* pBits, int nPitch, int nWidth, int nHeight, int nX, int nY, const char* pszText, std::uint32_t nColour);

} // namespace Text

#endif
