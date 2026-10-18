#include "common/compat.h"
#include "common/ctrackpool.h"
#include "planvm/agendaitem.h"
#include "engine/ctrack.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00413f60
void CTrackPool::Init()
{
    AgendaItem* pNode = new CTrack;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCTrack;

    while (nRemaining-- > 0)
    {
        pNode = new CTrack;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00413ff0
void* CTrackPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CTrack;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415aa0
CTrackPool::~CTrackPool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
