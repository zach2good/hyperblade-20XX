#include "common/compat.h"
#include "common/ctrackpasserreceiverpool.h"
#include "planvm/agendaitem.h"
#include "engine/ctrackpasserreceiver.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414260
void CTrackPasser_ReceiverPool::Init()
{
    AgendaItem* pNode = new CTrackPasser_Receiver;

    m_pHead     = pNode;
    auto* pPool = pNode;

    auto nRemaining = g_wPoolCountCTrackPasser_Receiver;

    while (nRemaining-- > 0)
    {
        pNode = new CTrackPasser_Receiver;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414310
void* CTrackPasser_ReceiverPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CTrackPasser_Receiver;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415c80
CTrackPasser_ReceiverPool::~CTrackPasser_ReceiverPool()
{
    while (m_pHead != nullptr)
    {
        auto* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
