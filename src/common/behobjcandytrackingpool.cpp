#include "common/compat.h"
#include "common/behobjcandytrackingpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00420830
void BehObjCandyTrackingPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_CandyTracking;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = (short)g_nPoolCountBehObjCandyTracking;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_CandyTracking;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x004208e0
void* BehObjCandyTrackingPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_CandyTracking;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004218a0
BehObjCandyTrackingPool::~BehObjCandyTrackingPool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
