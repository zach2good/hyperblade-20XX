#include "common/compat.h"
#include "common/cbungeetowardpool.h"
#include "planvm/agendaitem.h"
#include "planvm/cbungeetoward.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413750
void CBungeeTowardPool::Init()
{
    AgendaItem* pNode = new CBungeeToward;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCBungeeToward);

    while (nRemaining-- > 0)
    {
        pNode = new CBungeeToward;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004137d0
void* CBungeeTowardPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CBungeeToward;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415500
CBungeeTowardPool::~CBungeeTowardPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
