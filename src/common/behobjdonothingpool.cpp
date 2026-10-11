#include "common/compat.h"
#include "common/behobjdonothingpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041fef0
void BehObjDoNothingPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_DoNothing;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjDoNothing;

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
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_DoNothing;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004213a0
BehObjDoNothingPool::~BehObjDoNothingPool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
