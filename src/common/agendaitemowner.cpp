#include "common/compat.h"
#include "globals.h"
#include "common/agendaitemowner.h"
#include "pooledclasses.h"

// FUNCTION: HYPERX 0x00414990
char AgendaItemOwner::Insert(void* pNode)
{
    auto* const pItem = (AgendaItem*)pNode;

    pItem->m_pNext = m_pHead;
    m_pHead        = pItem;

    return 1;
}
