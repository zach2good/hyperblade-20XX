#include "compat.h"
#include "engine/gameengine.h"
#include "engine/cheadtrack.h"
#include "planvm/agendaitem.h"
#include "planvm/crelaxtoposition.h"
#include "common/crelaxtopositionpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"
#include "planvm/collinteractor.h"

// FUNCTION: HYPERX 0x00413d80
void CRelaxToPositionPool::Init()
{
    AgendaItem* pNode = new CRelaxToPosition;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCRelaxToPosition;

    while (nRemaining-- > 0)
    {
        pNode = new CRelaxToPosition;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00413e00
void* CRelaxToPositionPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CRelaxToPosition;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415960
CRelaxToPositionPool::~CRelaxToPositionPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
