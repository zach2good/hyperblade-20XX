#include "common/compat.h"
#include "common/behobjavoidancepool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041efe0
void BehObjAvoidancePool::Init()
{
    GameBehaviorObject* pNode = new BehObj_Avoidance;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountBehObjAvoidance;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_Avoidance;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f0a0
void* BehObjAvoidancePool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_Avoidance;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420b80
BehObjAvoidancePool::~BehObjAvoidancePool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
