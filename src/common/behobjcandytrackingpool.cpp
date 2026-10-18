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

    auto nRemaining = static_cast<short>(g_nPoolCountBehObjCandyTracking);

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
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_CandyTracking;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004218a0
BehObjCandyTrackingPool::~BehObjCandyTrackingPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
