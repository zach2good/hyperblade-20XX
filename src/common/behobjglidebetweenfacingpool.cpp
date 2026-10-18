#include "common/compat.h"
#include "common/behobjglidebetweenfacingpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041eec0
void BehObjGlideBetweenFacingPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_GlideBetweenFacing;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountBehObjGlideBetweenFacing);

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_GlideBetweenFacing;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041ef70
void* BehObjGlideBetweenFacingPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_GlideBetweenFacing;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420ae0
BehObjGlideBetweenFacingPool::~BehObjGlideBetweenFacingPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
