#include "common/compat.h"
#include "common/cdisplaypassbarpool.h"
#include "planvm/agendaitem.h"
#include "engine/cdisplaypassbar.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414a10
void CDisplayPassBarPool::Init()
{
    AgendaItem* pNode = new CDisplayPassBar;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCDisplayPassBar;

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
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CDisplayPassBar;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00416180
CDisplayPassBarPool::~CDisplayPassBarPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
