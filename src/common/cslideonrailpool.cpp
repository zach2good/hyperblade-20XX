#include "common/compat.h"
#include "common/cslideonrailpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00412c70
void CSlideOnRailPool::Init()
{
    AgendaItem* pNode = new CSlideOnRail;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    auto nRemaining = (short)g_nPoolCountCSlideOnRail;

    while (nRemaining-- > 0)
    {
        pNode = new CSlideOnRail;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00412d20
void* CSlideOnRailPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CSlideOnRail;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00414ec0
CSlideOnRailPool::~CSlideOnRailPool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
