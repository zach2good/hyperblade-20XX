#include "common/compat.h"
#include "common/behobjtrackingpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00420140
void BehObjTrackingPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_Tracking;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjTracking;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_Tracking;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x00420200
void* BehObjTrackingPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_Tracking;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004214e0
BehObjTrackingPool::~BehObjTrackingPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
