#include "common/compat.h"
#include "planvm/baseresetagendaitem.h"
#include "globals.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x00412360
void BaseResetAgendaItem::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner = nullptr;
        m_pNext  = nullptr;
        m_bReady = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}
