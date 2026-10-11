#include "common/compat.h"
#include "planvm/cspeedburstpool.h"
#include <new.h>
#include "planvm/agendaitem.h"
#include "engine/cspeedburst.h"
#include "common/poolcounts.h"

// FUNCTION: HYPERX 0x00413cd0
void CSpeedBurstPool::Init()
{
    AgendaItem* pNode = new CSpeedBurst;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCSpeedBurst;

    while (nRemaining-- > 0)
    {
        pNode = new CSpeedBurst;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// A failed refill writes null over the head, which is already null.
// FUNCTION: HYPERX 0x00413d30
void* CSpeedBurstPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CSpeedBurst;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004158c0
CSpeedBurstPool::~CSpeedBurstPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
