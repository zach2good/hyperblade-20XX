#include "common/compat.h"
#include "buttonwidget.h"
#include "textinputwidget.h"
#include "framepos.h"
#include "widget.h"
#include <ctype.h>
#include <mbstring.h>
#include "ui/holdingwidget.h"
#include "engine/object.h"

#define TEXT_INPUT_CAPACITY 0x50

// The length is read back as the index, so the terminator lands at offset zero.
// FUNCTION: HYPERX100 0x004777b0
// FUNCTION: HYPERX 0x0042bce0
TextInputWidget::TextInputWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark)
: HoldingWidget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
{
    m_nLength   = 0;
    m_nCapacity = TEXT_INPUT_CAPACITY;
    m_nType     = WIDGET_TEXT_INPUT;

    m_pszInput = new char[TEXT_INPUT_CAPACITY];

    m_pszInput[m_nLength] = 0;
}

#ifdef HYPERX_V100
// FUNCTION: HYPERX100 0x004778a0
TextInputWidget::TextInputWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nCapacity, int nSoundAtMark)
: HoldingWidget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
{
    m_nCapacity = nCapacity;
    m_nLength   = 0;
    m_nType     = WIDGET_TEXT_INPUT;

    m_pszInput = new char[nCapacity];

    m_pszInput[m_nLength] = 0;
}
#endif

// Backspace on an empty field writes the terminator one byte before the buffer, as in the original.
// FUNCTION: HYPERX100 0x004779c0
// FUNCTION: HYPERX 0x0042bda0
int TextInputWidget::OnKeyPress(unsigned int nKey)
{
    const int nChar = nKey;

    if (nChar == VK_BACK)
    {
        m_nLength--;

        m_pszInput[m_nLength] = 0;

        SetText(m_pszInput);

        return 1;
    }

    if (_ismbcprint(nChar) && m_nLength < m_nCapacity)
    {
        m_pszInput[m_nLength] = (char)nChar;

        m_nLength++;

        m_pszInput[m_nLength] = 0;

        SetText(m_pszInput);

        return 1;
    }

    return 0;
}
