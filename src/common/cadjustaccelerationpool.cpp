#include "common/compat.h"
#include "common/cadjustaccelerationpool.h"
#include "planvm/agendaitem.h"
#include "engine/cadjustacceleration.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413560
void CAdjustAccelerationPool::Init()
{
    AgendaItem* pNode = new CAdjustAcceleration;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCAdjustAcceleration);

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
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CAdjustAcceleration;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004153c0
CAdjustAccelerationPool::~CAdjustAccelerationPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
