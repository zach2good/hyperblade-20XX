#include "common/compat.h"
#include "common/behobjcoachteampool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00420710
void BehObjCoachTeamPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_CoachTeam;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountBehObjCoachTeam);

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_CoachTeam;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x004207c0
void* BehObjCoachTeamPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_CoachTeam;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421800
BehObjCoachTeamPool::~BehObjCoachTeamPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
