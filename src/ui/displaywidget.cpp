#include "common/compat.h"
#include "displaywidget.h"
#include "engine/object.h"

// FUNCTION: HYPERX 0x0042be20
DisplayWidget::DisplayWidget(int nX, int nY, int nHighlighted, int nDepressed, Screen* pOwner, int nSoundAtMark)
: InertWidget(nX, nY, 0, 0, nHighlighted, nDepressed, 0, 0, 0, 0, pOwner, nSoundAtMark)
{
    m_nType = WidgetKind::kDisplay;
}
