#include "common/compat.h"
#include "common/behobjdonothingpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041fef0
void BehObjDoNothingPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_DoNothing;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountBehObjDoNothing;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_DoNothing;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041ffb0
void* BehObjDoNothingPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_DoNothing;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004213a0
BehObjDoNothingPool::~BehObjDoNothingPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
