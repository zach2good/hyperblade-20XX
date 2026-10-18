#include "common/compat.h"
#include "common/csetpositionrelativepool.h"
#include "planvm/agendaitem.h"
#include "engine/csetpositionrelative.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413110
void CSetPositionRelativePool::Init()
{
    AgendaItem* pNode = new CSetPositionRelative;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCSetPositionRelative);

    while (nRemaining-- > 0)
    {
        pNode = new CSetPositionRelative;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004131b0
void* CSetPositionRelativePool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CSetPositionRelative;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415140
CSetPositionRelativePool::~CSetPositionRelativePool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
