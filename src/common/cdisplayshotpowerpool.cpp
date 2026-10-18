#include "common/compat.h"
#include "common/cdisplayshotpowerpool.h"
#include "planvm/agendaitem.h"
#include "engine/cdisplayshotpower.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414820
void CDisplayShotPowerPool::Init()
{
    AgendaItem* pNode = new CDisplayShotPower;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCDisplayShotPower;

    while (nRemaining-- > 0)
    {
        pNode = new CDisplayShotPower;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004148b0
void* CDisplayShotPowerPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CDisplayShotPower;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00416040
CDisplayShotPowerPool::~CDisplayShotPowerPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
