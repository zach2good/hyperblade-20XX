#include "common/compat.h"
#include "buttonwidget.h"
#include "widget.h"
#include "framepos.h"
#include "ui/holdingwidget.h"
#include "engine/object.h"
#include "common/stub.h"

// A type outside the four is a stub call rather than a default id.
// FUNCTION: HYPERX 0x0042bb90
ButtonWidget::ButtonWidget(int nType, int nX, int nY, int nWidth, int nHeight, int nHighlighted, int nDepressed, int nRows, int nFrameIdle, int nFrameHighlighted, int nTargetFrame, Screen* pOwner, int nSoundAtMark)
: HoldingWidget(nX, nY, nWidth, nHeight, nHighlighted, nDepressed, nRows, nFrameIdle, nFrameHighlighted, nTargetFrame, pOwner, nSoundAtMark)
{
    switch (nType)
    {
        case 0:
            m_nType = WIDGET_BUTTON_0;
            break;

        case 1:
            m_nType = WIDGET_BUTTON_1;
            break;

        case 2:
            m_nType = WIDGET_BUTTON_2;
            break;

        case 3:
            m_nType = WIDGET_BUTTON_3;
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// FUNCTION: HYPERX 0x0042bcb0
int ButtonWidget::OnMouseUp()
{
    m_pOwner->SetWidgetDepressedByType(GetTypeID(), 0);

    m_bDepressed = 1;
    m_bDirty     = 1;

    return 1;
}
