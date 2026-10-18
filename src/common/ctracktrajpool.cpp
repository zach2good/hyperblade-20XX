#include "common/compat.h"
#include "common/ctracktrajpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414050
void CTrackTrajPool::Init()
{
    AgendaItem* pNode = new CTrackTraj;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCTrackTraj;

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
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CTrackTraj;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415b40
CTrackTrajPool::~CTrackTrajPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
