#include "common/compat.h"
#include "framepos.h"
#include "widget.h"
#include "globals.h"
#include "ui/screen.h"
#include "ui/holdingwidget.h"

// FUNCTION: HYPERX 0x0042bd90
int HoldingWidget::OnMouseDown()
{
    m_bDepressed = 1;
    m_bDirty     = 1;

    return 1;
}
