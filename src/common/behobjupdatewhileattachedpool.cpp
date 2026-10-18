#include "common/compat.h"
#include "common/behobjupdatewhileattachedpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041fa50
void BehObjUpdateWhileAttachedPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_UpdateWhileAttached;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    auto nRemaining = (short)g_nPoolCountBehObjUpdateWhileAttached;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_UpdateWhileAttached;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041fb00
void* BehObjUpdateWhileAttachedPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_UpdateWhileAttached;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421120
BehObjUpdateWhileAttachedPool::~BehObjUpdateWhileAttachedPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
