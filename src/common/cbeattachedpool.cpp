#include "common/compat.h"
#include "common/cbeattachedpool.h"
#include "planvm/agendaitem.h"
#include "engine/cbeattached.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00412ef0
void CBeAttachedPool::Init()
{
    AgendaItem* pNode = new CBeAttached;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCBeAttached);

    while (nRemaining-- > 0)
    {
        pNode = new CBeAttached;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00412fa0
void* CBeAttachedPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CBeAttached;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415000
CBeAttachedPool::~CBeAttachedPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
