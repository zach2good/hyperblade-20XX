#include "common/compat.h"
#include "common/behobjrailslidepool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f480
void BehObjRailSlidePool::Init()
{
    GameBehaviorObject* pNode = new BehObj_RailSlide;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjRailSlide;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_RailSlide;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f540
void* BehObjRailSlidePool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_RailSlide;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420e00
BehObjRailSlidePool::~BehObjRailSlidePool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
