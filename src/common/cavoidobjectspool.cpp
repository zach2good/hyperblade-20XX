#include "common/compat.h"
#include "common/cavoidobjectspool.h"
#include "planvm/agendaitem.h"
#include "planvm/cavoidobjects.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413660
void CAvoidObjectsPool::Init()
{
    AgendaItem* pNode = new CAvoidObjects;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCAvoidObjects;

    while (nRemaining-- > 0)
    {
        pNode = new CAvoidObjects;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004136f0
void* CAvoidObjectsPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CAvoidObjects;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415460
CAvoidObjectsPool::~CAvoidObjectsPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
