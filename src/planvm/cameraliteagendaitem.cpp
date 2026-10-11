#include "common/compat.h"
#include "planvm/cameraliteagendaitem.h"
#include "globals.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x0040a0d0
void CameraLiteAgendaItem::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner    = 0;
        m_pWatchPos = 0;
        m_pWatchDir = 0;
        m_pNext     = 0;
        m_bReady    = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}
