#include "common/compat.h"
#include "common/ctracktrajpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414050
void CTrackTrajPool::Init()
{
    AgendaItem* pNode = new CTrackTraj;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCTrackTraj;

    while (nRemaining-- > 0)
    {
        pNode = new CTrackTraj;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414100
void* CTrackTrajPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CTrackTraj;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415b40
CTrackTrajPool::~CTrackTrajPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
