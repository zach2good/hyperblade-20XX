#include "common/compat.h"
#include "common/cstepanimationpool.h"
#include "planvm/agendaitem.h"
#include "planvm/cstepanimation.h"
#include "poolcounts.h"

// FUNCTION: HYPERX1000 0x0041fb20
// FUNCTION: HYPERX1001 0x00413a10
void CStepAnimationPool::Init()
{
    AgendaItem* pNode = new CStepAnimation;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCStepAnimation;

    while (nRemaining-- > 0)
    {
        pNode = new CStepAnimation;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

char CStepAnimationPool::Insert(void* pNode)
{
    AgendaItem* const pItem = (AgendaItem*)pNode;

    pItem->m_pNext = m_pHead;
    m_pHead        = pItem;

    return 1;
}

// FUNCTION: HYPERX1000 0x00423280
// FUNCTION: HYPERX1001 0x00413a90
void* CStepAnimationPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CStepAnimation;
    }

    return pHead;
}

// FUNCTION: HYPERX1001 0x004156e0
CStepAnimationPool::~CStepAnimationPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
