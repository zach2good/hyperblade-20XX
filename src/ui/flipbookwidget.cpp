#include "common/compat.h"
#include "buttonwidget.h"
#include "textinputwidget.h"
#include "ui/flipbookwidget.h"
#include "ui/loopflipbookwidget.h"
#include "ui/displaywidget.h"
#include "engine/object.h"

// The base's twelfth argument is the constructor's last, not its twelfth.
// FUNCTION: HYPERX100 0x00477240
// FUNCTION: HYPERX 0x0042b9e0
FlipbookWidget::FlipbookWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundPastFirstLimit, int nSoundBeforeTarget, int nSoundAtMark)
: Widget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
{
    m_flipbook.m_nSoundPastFirstLimit = nSoundPastFirstLimit;
    m_flipbook.m_nSoundBeforeTarget   = nSoundBeforeTarget;
    m_nType                           = WIDGET_FLIPBOOK;
}

// FUNCTION: HYPERX100 0x00477320
// FUNCTION: HYPERX 0x0042ba50
int FlipbookWidget::OnMouseUp()
{
    m_bDepressed = 0;
    m_bDirty     = 1;

    return 1;
}

// FUNCTION: HYPERX100 0x00477310
// FUNCTION: HYPERX 0x0042bd90
int FlipbookWidget::OnMouseDown()
{
    m_bDepressed = 1;
    m_bDirty     = 1;

    return 1;
}
