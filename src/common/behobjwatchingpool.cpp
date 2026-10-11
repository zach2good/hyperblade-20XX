#include "common/compat.h"
#include "common/behobjwatchingpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00420390
void BehObjWatchingPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_Watching;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjWatching;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_Watching;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x00420450
void* BehObjWatchingPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_Watching;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421620
BehObjWatchingPool::~BehObjWatchingPool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
