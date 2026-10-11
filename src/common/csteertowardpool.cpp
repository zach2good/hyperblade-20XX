#include "common/compat.h"
#include "common/csteertowardpool.h"
#include "planvm/agendaitem.h"
#include "planvm/csteertoward.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413440
void CSteerTowardPool::Init()
{
    AgendaItem* pNode = new CSteerToward;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCSteerToward;

    while (nRemaining-- > 0)
    {
        pNode = new CSteerToward;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004134f0
void* CSteerTowardPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CSteerToward;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415320
CSteerTowardPool::~CSteerTowardPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
