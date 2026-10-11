#include "common/compat.h"
#include "common/cdopplersndpool.h"
#include "planvm/agendaitem.h"
#include "planvm/cdopplersnd.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414540
void CDopplerSndPool::Init()
{
    AgendaItem* pNode = new CDopplerSnd;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCDopplerSnd;

    while (nRemaining-- > 0)
    {
        pNode = new CDopplerSnd;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004145c0
void* CDopplerSndPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CDopplerSnd;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415e60
CDopplerSndPool::~CDopplerSndPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
