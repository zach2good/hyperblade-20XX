#include "common/compat.h"
#include "ui/buttonwidget.h"
#include "signflipbookwidget.h"
#include "ui/flipbookwidget.h"

// FUNCTION: HYPERX 0x0042ba60
SignFlipbookWidget::SignFlipbookWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark)
: FlipbookWidget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark, -1, -1)
{
    m_nType = WIDGET_FLIPBOOK;
}
