#include "common/compat.h"
#include "common/cpaletteeffectpool.h"
#include "planvm/agendaitem.h"
#include "planvm/cpaletteeffect.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414720
void CPaletteEffectPool::Init()
{
    AgendaItem* pNode = new CPaletteEffect;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = g_wPoolCountCPaletteEffect;

    while (nRemaining-- > 0)
    {
        pNode = new CPaletteEffect;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004147c0
void* CPaletteEffectPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CPaletteEffect;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00415fa0
CPaletteEffectPool::~CPaletteEffectPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
