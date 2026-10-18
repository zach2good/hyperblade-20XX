#include "common/compat.h"
#include "common/behobjupdaterelativepospool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f110
void BehObjUpdateRelativePosPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_UpdateRelativePos;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    auto nRemaining = (short)g_nPoolCountBehObjUpdateRelativePos;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_UpdateRelativePos;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f1c0
void* BehObjUpdateRelativePosPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_UpdateRelativePos;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420c20
BehObjUpdateRelativePosPool::~BehObjUpdateRelativePosPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
