#include "common/compat.h"
#include "common/cadjustaccelerationpool.h"
#include "planvm/agendaitem.h"
#include "engine/cadjustacceleration.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413560
void CAdjustAccelerationPool::Init()
{
    AgendaItem* pNode = new CAdjustAcceleration;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCAdjustAcceleration;

    while (nRemaining-- > 0)
    {
        pNode = new CAdjustAcceleration;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00413600
void* CAdjustAccelerationPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CAdjustAcceleration;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004153c0
CAdjustAccelerationPool::~CAdjustAccelerationPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
