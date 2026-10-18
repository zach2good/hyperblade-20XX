#include "common/compat.h"
#include "common/behobjflylclpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f6d0
void BehObjFlyLCLPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_FlyLCL;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountBehObjFlyLCL;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_FlyLCL;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f790
void* BehObjFlyLCLPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_FlyLCL;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420f40
BehObjFlyLCLPool::~BehObjFlyLCLPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
