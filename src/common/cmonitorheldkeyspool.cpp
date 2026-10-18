#include "common/compat.h"
#include "common/cmonitorheldkeyspool.h"
#include "planvm/agendaitem.h"
#include "engine/cmonitorheldkeys.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413be0
void CMonitorHeldKeysPool::Init()
{
    AgendaItem* pNode = new CMonitorHeldKeys;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCMonitorHeldKeys;

    while (nRemaining-- > 0)
    {
        pNode = new CMonitorHeldKeys;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00413c70
void* CMonitorHeldKeysPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CMonitorHeldKeys;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415820
CMonitorHeldKeysPool::~CMonitorHeldKeysPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
