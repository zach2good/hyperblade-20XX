#include "common/compat.h"
#include "common/ccircletrackpool.h"
#include "planvm/agendaitem.h"
#include "engine/ccircletrack.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414ce0
void CCircleTrackPool::Init()
{
    AgendaItem* pNode = new CCircleTrack;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCCircleTrack;

    while (nRemaining-- > 0)
    {
        pNode = new CCircleTrack;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414d70
void* CCircleTrackPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CCircleTrack;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00416360
CCircleTrackPool::~CCircleTrackPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
