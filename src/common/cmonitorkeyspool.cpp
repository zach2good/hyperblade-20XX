#include "common/compat.h"
#include "common/cmonitorkeyspool.h"
#include "planvm/agendaitem.h"
#include "planvm/cmonitorkeys.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413af0
void CMonitorKeysPool::Init()
{
    AgendaItem* pNode = new CMonitorKeys;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCMonitorKeys;

    while (nRemaining-- > 0)
    {
        pNode = new CMonitorKeys;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00413b80
void* CMonitorKeysPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CMonitorKeys;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415780
CMonitorKeysPool::~CMonitorKeysPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
