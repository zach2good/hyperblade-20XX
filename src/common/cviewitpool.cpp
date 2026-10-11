#include "common/compat.h"
#include "common/cviewitpool.h"
#include "planvm/agendaitem.h"
#include "planvm/cviewit.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414370
void CViewItPool::Init()
{
    AgendaItem* pNode = new CViewIt;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCViewIt;

    while (nRemaining-- > 0)
    {
        pNode = new CViewIt;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414400
void* CViewItPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CViewIt;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415d20
CViewItPool::~CViewItPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
