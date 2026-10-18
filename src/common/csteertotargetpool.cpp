#include "common/compat.h"
#include "common/csteertotargetpool.h"
#include "planvm/agendaitem.h"
#include "planvm/csteertotarget.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413010
void CSteerToTargetPool::Init()
{
    AgendaItem* pNode = new CSteerToTarget;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCSteerToTarget;

    while (nRemaining-- > 0)
    {
        pNode = new CSteerToTarget;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004130b0
void* CSteerToTargetPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CSteerToTarget;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004150a0
CSteerToTargetPool::~CSteerToTargetPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
