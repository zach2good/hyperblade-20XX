#include "common/compat.h"
#include "common/behobjglidetopositionpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041ed90
void BehObjGlideToPositionPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_GlideToPosition;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjGlideToPosition;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_GlideToPosition;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041ee50
void* BehObjGlideToPositionPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_GlideToPosition;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420a40
BehObjGlideToPositionPool::~BehObjGlideToPositionPool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
