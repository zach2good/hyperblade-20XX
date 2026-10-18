#include "common/compat.h"
#include "planvm/agendaitem.h"
#include "planvm/releasingagendaitem.h"

// FUNCTION: HYPERX 0x0040ed80
void ReleasingAgendaItem::Release(int nArg)
{
    auto* const pNext = m_pNext;

    if (pNext != nullptr)
    {
        pNext->Release(nArg);
    }
}
