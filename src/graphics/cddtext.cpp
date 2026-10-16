#include "common/compat.h"
#include "globals.h"
#include "app/gameapp.h"
#include "cddtext.h"
#include <string.h>
#include <stdlib.h>

// FUNCTION: HYPERX 0x0044f9f0
CDDText::~CDDText() throw()
{
    if (m_hFont != 0)
    {
        DeleteObject(m_hFont);
    }

    if (m_pszText != 0)
    {
        free((void*)m_pszText);
    }
}

// FUNCTION: HYPERX 0x0044fa40
int CDDText::Draw()
{
    HGDIOBJ hOldFont = 0;

    if (m_pszText != 0 && *m_pszText != 0)
    {
        if (m_hFont != 0)
        {
            hOldFont = SelectObject(g_hTextDC, m_hFont);
        }

        const COLORREF oldColor = SetTextColor(g_hTextDC, m_color);

        TextOutA(g_hTextDC, m_ptPosition.x + m_ptDrawOffset.x, m_ptPosition.y + m_ptDrawOffset.y, m_pszText, (int)strlen(m_pszText));

        SetTextColor(g_hTextDC, oldColor);

        if (m_hFont != 0)
        {
            SelectObject(g_hTextDC, hOldFont);
        }
    }

    return 1;
}

// A colour of -1 leaves the colour alone.
// FUNCTION: HYPERX 0x0044faf0
int CDDText::SetText(const char* pszText, COLORREF color)
{
    HGDIOBJ hOldFont = 0;
    SIZE    size;

    if (m_pszText != 0)
    {
        free((void*)m_pszText);

        m_pszText = 0;
    }

    if (color != (COLORREF)-1)
    {
        m_color = color;
    }

    if (pszText != 0)
    {
        char* const pszCopy = (char*)malloc(strlen(pszText) + 1);

        if (pszCopy != 0)
        {
            strcpy(pszCopy, pszText);
        }

        m_pszText = pszCopy;

        if (pszCopy != 0 && *pszCopy != 0)
        {
            HDC hdc = g_hTextDC;

            if (hdc == 0)
            {
                hdc = g_gameApp.m_hScreenDC;
            }

            if (m_hFont != 0)
            {
                hOldFont = SelectObject(hdc, m_hFont);
            }

            if (GetTextExtentPoint32A(hdc, m_pszText, (int)strlen(m_pszText), &size) != 0)
            {
                m_ptSize.x = size.cx;
                m_ptSize.y = size.cy;
            }
            else
            {
                m_ptSize.y = 0;
                m_ptSize.x = 0;
            }

            if (m_hFont != 0)
            {
                SelectObject(hdc, hOldFont);
            }
        }

        return m_pszText != 0;
    }

    m_ptSize.y = 0;
    m_ptSize.x = 0;

    return 1;
}

// A null face name drops the font and answers success.
// FUNCTION: HYPERX 0x0044fc10
int CDDText::CreateFontFace(const char* pszFaceName, int nHeight)
{
    if (m_hFont != 0)
    {
        DeleteObject(m_hFont);

        m_hFont = 0;
    }

    if (pszFaceName != 0)
    {
        m_hFont = ::CreateFontA(nHeight, 0, 0, 0, FW_MEDIUM, 0, 0, 0, 0, 0, 0, NONANTIALIASED_QUALITY, VARIABLE_PITCH, pszFaceName);

        return m_hFont != 0;
    }

    return 1;
}
