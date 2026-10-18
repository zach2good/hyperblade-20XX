#include "common/compat.h"
#include "common/csteerbetweenfacingpool.h"
#include "planvm/agendaitem.h"
#include "planvm/csteerbetweenfacing.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413340
void CSteerBetweenFacingPool::Init()
{
    AgendaItem* pNode = new CSteerBetweenFacing;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCSteerBetweenFacing);

    while (nRemaining-- > 0)
    {
        pNode = new CSteerBetweenFacing;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004133e0
void* CSteerBetweenFacingPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CSteerBetweenFacing;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415280
CSteerBetweenFacingPool::~CSteerBetweenFacingPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
