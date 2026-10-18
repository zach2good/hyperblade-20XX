#include "common/compat.h"
#include "common/behobjflytowardpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f800
void BehObjFlyTowardPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_FlyToward;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = (short)g_nPoolCountBehObjFlyToward;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_FlyToward;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f8b0
void* BehObjFlyTowardPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_FlyToward;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420fe0
BehObjFlyTowardPool::~BehObjFlyTowardPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
