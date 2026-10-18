#include "common/compat.h"
#include "common/ccheckaipool.h"
#include "planvm/agendaitem.h"
#include "engine/ccheckai.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414c00
void CCheckAIPool::Init()
{
    AgendaItem* pNode = new CCheckAI;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCCheckAI);

    while (nRemaining-- > 0)
    {
        pNode = new CCheckAI;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414c80
void* CCheckAIPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CCheckAI;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004162c0
CCheckAIPool::~CCheckAIPool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
