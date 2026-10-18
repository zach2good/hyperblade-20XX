#include "common/compat.h"
#include "common/crelaxyawpool.h"
#include "planvm/agendaitem.h"
#include "engine/crelaxyaw.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413930
void CRelaxYawPool::Init()
{
    AgendaItem* pNode = new CRelaxYaw;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountCRelaxYaw);

    while (nRemaining-- > 0)
    {
        pNode = new CRelaxYaw;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004139b0
void* CRelaxYawPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CRelaxYaw;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415640
CRelaxYawPool::~CRelaxYawPool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
