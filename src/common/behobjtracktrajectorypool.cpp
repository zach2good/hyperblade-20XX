#include "common/compat.h"
#include "common/behobjtracktrajectorypool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x004204c0
void BehObjTrackTrajectoryPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_TrackTrajectory;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountBehObjTrackTrajectory);

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_TrackTrajectory;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x00420570
void* BehObjTrackTrajectoryPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_TrackTrajectory;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004216c0
BehObjTrackTrajectoryPool::~BehObjTrackTrajectoryPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
