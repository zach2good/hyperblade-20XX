#include "common/compat.h"
#include "common/behobjflypool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f5b0
void BehObjFlyPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_Fly;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountBehObjFly);

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_Fly;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f660
void* BehObjFlyPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_Fly;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420ea0
BehObjFlyPool::~BehObjFlyPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
