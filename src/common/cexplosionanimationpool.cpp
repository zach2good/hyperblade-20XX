#include "common/compat.h"
#include "common/cexplosionanimationpool.h"
#include "planvm/agendaitem.h"
#include "engine/cexplosionanimation.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414b10
void CExplosionAnimationPool::Init()
{
    AgendaItem* pNode = new CExplosionAnimation;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCExplosionAnimation;

    while (nRemaining-- > 0)
    {
        pNode = new CExplosionAnimation;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x00414ba0
void* CExplosionAnimationPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CExplosionAnimation;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00416220
CExplosionAnimationPool::~CExplosionAnimationPool()
{
    while (m_pHead != nullptr)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
