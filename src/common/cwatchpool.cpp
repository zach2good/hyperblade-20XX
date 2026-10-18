#include "common/compat.h"
#include "common/cwatchpool.h"
#include "planvm/agendaitem.h"
#include "engine/cwatch.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414170
void CWatchPool::Init()
{
    AgendaItem* pNode = new CWatch;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCWatch;

    while (nRemaining-- > 0)
    {
        pNode = new CWatch;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414200
void* CWatchPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CWatch;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415be0
CWatchPool::~CWatchPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
