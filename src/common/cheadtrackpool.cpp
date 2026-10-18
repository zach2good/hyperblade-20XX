#include "common/compat.h"
#include "common/cheadtrackpool.h"
#include "engine/cheadtrack.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414620
void CHeadTrackPool::Init()
{
    AgendaItem* pNode = new CHeadTrack;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCHeadTrack;

    while (nRemaining-- > 0)
    {
        pNode = new CHeadTrack;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004146c0
void* CHeadTrackPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CHeadTrack;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415f00
CHeadTrackPool::~CHeadTrackPool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
