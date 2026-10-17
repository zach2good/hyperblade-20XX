#ifndef HYPERBLADE_CDDTEXT_H
#define HYPERBLADE_CDDTEXT_H

#include "common/compat.h"
#include "rlespritesurface.h"
#include "graphics/text.h"
#include <cstdint>

// VTABLE: HYPERX 0x004b32d4
class CDDText : public CDDDrawable
{
public:
    // FUNCTION: HYPERX 0x00451c60
    CDDText()
    : m_pszText(0)
    , m_color(0)
    {
    }

    ~CDDText() override;
    int Draw() override;

    // Not CreateFont: windows.h defines that as a macro for CreateFontA.
    int CreateFontFace(const char* pszFaceName, int nHeight);
    int SetText(const char* pszText, std::uint32_t color);

    void SetColour(std::uint32_t color)
    {
        m_color = color;
    }

    const char*   m_pszText; // 0x1c
    std::uint32_t m_color;   // 0x20, 0x00BBGGRR
    Text::Font    m_font;
};

// SYNTHETIC: HYPERX 0x0044fa20
// CDDText::`scalar deleting destructor'

#endif
