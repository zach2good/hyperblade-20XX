#include "common/compat.h"
#include "common/behobjglidepool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f230
void BehObjGlidePool::Init()
{
    GameBehaviorObject* pNode = new BehObj_Glide;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjGlide;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_Glide;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f2f0
void* BehObjGlidePool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_Glide;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420cc0
BehObjGlidePool::~BehObjGlidePool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
