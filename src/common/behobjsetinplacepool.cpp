#include "common/compat.h"
#include "common/behobjsetinplacepool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041fb70
void BehObjSetInPlacePool::Init()
{
    GameBehaviorObject* pNode = new BehObj_SetInPlace;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjSetInPlace;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_SetInPlace;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041fc30
void* BehObjSetInPlacePool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_SetInPlace;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004211c0
BehObjSetInPlacePool::~BehObjSetInPlacePool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
