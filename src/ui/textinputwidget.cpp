#include "common/compat.h"
#include "input/keycodes.h"
#include "buttonwidget.h"
#include "textinputwidget.h"
#include "framepos.h"
#include "widget.h"
#include <cctype>
#include "ui/holdingwidget.h"
#include "engine/object.h"
#include <SDL3/SDL.h>

constexpr auto kTextInputCapacity = 0x50;

// The length is read back as the index, so the terminator lands at offset zero.
// FUNCTION: HYPERX 0x0042bce0
TextInputWidget::TextInputWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark)
: HoldingWidget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
{
    m_nLength   = 0;
    m_nCapacity = kTextInputCapacity;
    m_nType     = WIDGET_TEXT_INPUT;

    m_pszInput = new char[kTextInputCapacity];

    m_pszInput[m_nLength] = 0;
}

// Backspace on an empty field writes the terminator one byte before the buffer, as in the original.
// FUNCTION: HYPERX 0x0042bda0
int TextInputWidget::OnKeyPress(unsigned int nKey)
{
    const int nChar = nKey;

    if (nChar == Key::kBack)
    {
        m_nLength--;

        m_pszInput[m_nLength] = 0;

        SetText(m_pszInput);

        return 1;
    }

    if (SDL_isprint(nChar) && m_nLength < m_nCapacity)
    {
        m_pszInput[m_nLength] = static_cast<char>(nChar);

        m_nLength++;

        m_pszInput[m_nLength] = 0;

        SetText(m_pszInput);

        return 1;
    }

    return 0;
}
