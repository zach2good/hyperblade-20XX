#include "common/compat.h"
#include "common/ctimeractivitypool.h"
#include "planvm/agendaitem.h"
#include "planvm/ctimeractivity.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413e60
void CTimerActivityPool::Init()
{
    AgendaItem* pNode = new CTimerActivity;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCTimerActivity;

    while (nRemaining-- > 0)
    {
        pNode = new CTimerActivity;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00413f00
void* CTimerActivityPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CTimerActivity;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415a00
CTimerActivityPool::~CTimerActivityPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
