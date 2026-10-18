#include "common/compat.h"
#include "common/csteertopositionpool.h"
#include "planvm/agendaitem.h"
#include "engine/csteertoposition.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413210
void CSteerToPositionPool::Init()
{
    AgendaItem* pNode = new CSteerToPosition;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCSteerToPosition;

    while (nRemaining-- > 0)
    {
        pNode = new CSteerToPosition;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004132d0
void* CSteerToPositionPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CSteerToPosition;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004151e0
CSteerToPositionPool::~CSteerToPositionPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
