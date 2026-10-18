#include "common/compat.h"
#include "globals.h"
#include "app/gameapp.h"
#include "cddtext.h"
#include "graphics/hudlayer.h"
#include "graphics/colorpalette.h"
#include <cstring>
#include <cstdlib>

// FUNCTION: HYPERX 0x0044f9f0
CDDText::~CDDText()
{
    if (m_pszText != nullptr)
    {
        free((void*)m_pszText);
    }
}

// Draws into whatever the back-buffer globals point at, the HUD layer during a match, in the palette
// entry nearest the colour, as GDI matched a colour against the DIB's table.
// FUNCTION: HYPERX 0x0044fa40
int CDDText::Draw()
{
    if (m_pszText != nullptr && *m_pszText != 0 && g_pBackBufferBitsCopy != nullptr)
    {
        const auto nIndex = (unsigned char)g_pColorPalette->NearestIndex(m_color, HudLayer::KEY_INDEX);

        Text::Draw(m_font, g_pBackBufferBitsCopy, g_nSurfacePitchCopy, g_nBackBufferWidth, g_nBackBufferHeight, m_ptPosition.x + m_ptDrawOffset.x, m_ptPosition.y + m_ptDrawOffset.y, m_pszText, nIndex);
    }

    return 1;
}

// A colour of -1 leaves the colour alone.
// FUNCTION: HYPERX 0x0044faf0
int CDDText::SetText(const char* pszText, std::uint32_t color)
{
    if (m_pszText != nullptr)
    {
        free((void*)m_pszText);

        m_pszText = nullptr;
    }

    if (color != (std::uint32_t)-1)
    {
        m_color = color;
    }

    m_ptSize.x = 0;
    m_ptSize.y = 0;

    if (pszText == nullptr)
    {
        return 1;
    }

    char* const pszCopy = (char*)malloc(strlen(pszText) + 1);

    if (pszCopy != nullptr)
    {
        strcpy(pszCopy, pszText);

        const auto size = Text::Measure(m_font, pszCopy);

        m_ptSize.x = size.nWidth;
        m_ptSize.y = size.nHeight;
    }

    m_pszText = pszCopy;

    return m_pszText != nullptr;
}

// A null face name drops the font and answers success. Every face draws as MS Sans Serif.
// FUNCTION: HYPERX 0x0044fc10
int CDDText::CreateFontFace(const char* pszFaceName, int nHeight)
{
    if (pszFaceName == nullptr)
    {
        m_font = Text::Font{};

        return 1;
    }

    m_font = Text::FindSansSerif(nHeight);

    return (bool)m_font;
}
