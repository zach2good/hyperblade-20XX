#include "common/compat.h"
#include "planvm/cameraliteagendaitem.h"
#include "globals.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x0040a0d0
void CameraLiteAgendaItem::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner    = nullptr;
        m_pWatchPos = nullptr;
        m_pWatchDir = nullptr;
        m_pNext     = nullptr;
        m_bReady    = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}
