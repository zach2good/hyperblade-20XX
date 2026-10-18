#include "common/compat.h"
#include "common/cprojectandbouncepool.h"
#include "planvm/agendaitem.h"
#include "planvm/cprojectandbounce.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00412d90
void CProjectAndBouncePool::Init()
{
    AgendaItem* pNode = new CProjectAndBounce;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCProjectAndBounce;

    while (nRemaining-- > 0)
    {
        pNode = new CProjectAndBounce;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00412e70
void* CProjectAndBouncePool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CProjectAndBounce;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00414f60
CProjectAndBouncePool::~CProjectAndBouncePool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
