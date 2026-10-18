#include "common/compat.h"
#include "common/cpansoundpool.h"
#include "planvm/agendaitem.h"
#include "engine/cpansound.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414460
void CPanSoundPool::Init()
{
    AgendaItem* pNode = new CPanSound;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    auto nRemaining = (short)g_nPoolCountCPanSound;

    while (nRemaining-- > 0)
    {
        pNode = new CPanSound;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004144e0
void* CPanSoundPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CPanSound;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415dc0
CPanSoundPool::~CPanSoundPool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
