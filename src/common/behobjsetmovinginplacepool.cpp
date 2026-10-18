#include "common/compat.h"
#include "common/behobjsetmovinginplacepool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041fdd0
void BehObjSetMovingInPlacePool::Init()
{
    GameBehaviorObject* pNode = new BehObj_SetMovingInPlace;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = (short)g_nPoolCountBehObjSetMovingInPlace;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_SetMovingInPlace;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041fe80
void* BehObjSetMovingInPlacePool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_SetMovingInPlace;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421300
BehObjSetMovingInPlacePool::~BehObjSetMovingInPlacePool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
