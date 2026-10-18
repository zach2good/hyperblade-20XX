#include "common/compat.h"
#include "common/ctimeractivitypool.h"
#include "planvm/agendaitem.h"
#include "planvm/ctimeractivity.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413e60
void CTimerActivityPool::Init()
{
    AgendaItem* pNode = new CTimerActivity;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCTimerActivity;

    while (nRemaining-- > 0)
    {
        pNode = new CTimerActivity;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00413f00
void* CTimerActivityPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CTimerActivity;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415a00
CTimerActivityPool::~CTimerActivityPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
