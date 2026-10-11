#include "common/compat.h"
#include "common/csteerbetweenfacingpool.h"
#include "planvm/agendaitem.h"
#include "planvm/csteerbetweenfacing.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413340
void CSteerBetweenFacingPool::Init()
{
    AgendaItem* pNode = new CSteerBetweenFacing;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCSteerBetweenFacing;

    while (nRemaining-- > 0)
    {
        pNode = new CSteerBetweenFacing;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004133e0
void* CSteerBetweenFacingPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CSteerBetweenFacing;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415280
CSteerBetweenFacingPool::~CSteerBetweenFacingPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
