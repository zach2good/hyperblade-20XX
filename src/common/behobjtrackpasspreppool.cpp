#include "common/compat.h"
#include "common/behobjtrackpasspreppool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x004205e0
void BehObjTrackPassPrepPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_TrackPassPrep;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjTrackPassPrep;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_TrackPassPrep;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x004206a0
void* BehObjTrackPassPrepPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_TrackPassPrep;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421760
BehObjTrackPassPrepPool::~BehObjTrackPassPrepPool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
