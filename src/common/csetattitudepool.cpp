#include "common/compat.h"
#include "common/csetattitudepool.h"
#include "planvm/agendaitem.h"
#include "engine/csetattitude.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413830
void CSetAttitudePool::Init()
{
    AgendaItem* pNode = new CSetAttitude;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCSetAttitude;

    while (nRemaining-- > 0)
    {
        pNode = new CSetAttitude;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004138d0
void* CSetAttitudePool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CSetAttitude;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004155a0
CSetAttitudePool::~CSetAttitudePool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
