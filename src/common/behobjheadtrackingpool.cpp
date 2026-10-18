#include "common/compat.h"
#include "common/behobjheadtrackingpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00420270
void BehObjHeadTrackingPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_HeadTracking;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountBehObjHeadTracking);

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_HeadTracking;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x00420320
void* BehObjHeadTrackingPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_HeadTracking;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421580
BehObjHeadTrackingPool::~BehObjHeadTrackingPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
