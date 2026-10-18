#include "common/compat.h"
#include "common/behobjsetflatinplacepool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041fca0
void BehObjSetFlatInPlacePool::Init()
{
    GameBehaviorObject* pNode = new BehObj_SetFlatInPlace;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjSetFlatInPlace;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_SetFlatInPlace;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041fd60
void* BehObjSetFlatInPlacePool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_SetFlatInPlace;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421260
BehObjSetFlatInPlacePool::~BehObjSetFlatInPlacePool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
