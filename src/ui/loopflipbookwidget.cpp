#include "common/compat.h"
#include "loopflipbookwidget.h"
#include "engine/object.h"

// FUNCTION: HYPERX 0x0042bac0
LoopFlipbookWidget::LoopFlipbookWidget(int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nLoopFlag, int nSoundAtMark)
: InertWidget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
{
    m_nLoopFlag = nLoopFlag;
    m_nType     = WIDGET_LOOP_FLIPBOOK;
}

// FUNCTION: HYPERX 0x0042bb20
void LoopFlipbookWidget::Draw(int nSlot)
{
    if (m_bHidden == 1)
    {
        return;
    }

    if (m_bDirty == 0)
    {
        return;
    }

    Widget::Draw(nSlot);

    if (m_nFrame != m_nTargetFrame)
    {
        return;
    }

    if (m_nLoopFlag == 0)
    {
        m_bDirty = 0;

        m_pOwner->OnWidgetMessage(kWidgetMsgLoopDone, nSlot);

        return;
    }

    m_nFrame = m_nFrameIdle;

    m_pOwner->OnWidgetMessage(kWidgetMsgLoopDone, nSlot);
}
