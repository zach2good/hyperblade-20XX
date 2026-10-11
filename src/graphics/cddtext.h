#ifndef HYPERBLADE_CDDTEXT_H
#define HYPERBLADE_CDDTEXT_H

#include "common/compat.h"
#include "rlespritesurface.h"

// VTABLE: HYPERX 0x004b32d4
class CDDText : public CDDDrawable
{
public:
    // FUNCTION: HYPERX 0x00451c60
    CDDText()
    : m_pszText(0)
    , m_color(0)
    , m_hFont(0)
    {
    }

    virtual ~CDDText();
    virtual int Draw();

    // Not CreateFont: windows.h defines that as a macro for CreateFontA.
    int CreateFontFace(const char* pszFaceName, int nHeight);
    int SetText(const char* pszText, COLORREF color);

    void SetColour(COLORREF color)
    {
        m_color = color;
    }

    const char* m_pszText; // 0x1c
    COLORREF    m_color;   // 0x20
    HFONT       m_hFont;   // 0x24
};

// SYNTHETIC: HYPERX 0x0044fa20
// CDDText::`scalar deleting destructor'

#endif
