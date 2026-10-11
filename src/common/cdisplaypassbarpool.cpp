#include "common/compat.h"
#include "common/cdisplaypassbarpool.h"
#include "planvm/agendaitem.h"
#include "engine/cdisplaypassbar.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414a10
void CDisplayPassBarPool::Init()
{
    AgendaItem* pNode = new CDisplayPassBar;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCDisplayPassBar;

    while (nRemaining-- > 0)
    {
        pNode = new CDisplayPassBar;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414aa0
void* CDisplayPassBarPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CDisplayPassBar;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00416180
CDisplayPassBarPool::~CDisplayPassBarPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
