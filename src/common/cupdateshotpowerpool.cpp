#include "common/compat.h"
#include "common/cupdateshotpowerpool.h"
#include "planvm/agendaitem.h"
#include "planvm/cupdateshotpower.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00414910
void CUpdateShotPowerPool::Init()
{
    AgendaItem* pNode = new CUpdateShotPower;

    m_pHead           = pNode;
    AgendaItem* pPool = pNode;

    short nRemaining = (short)g_nPoolCountCUpdateShotPower;

    while (nRemaining-- > 0)
    {
        pNode = new CUpdateShotPower;

        pPool->m_pNext = pNode;
        pPool          = pNode;
    }
}

// FUNCTION: HYPERX 0x004149b0
void* CUpdateShotPowerPool::Alloc()
{
    AgendaItem* const pHead = m_pHead;

    m_pHead        = pHead->m_pNext;
    pHead->m_pNext = 0;

    if (m_pHead == 0)
    {
        m_pHead = new CUpdateShotPower;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x004160e0
CUpdateShotPowerPool::~CUpdateShotPowerPool()
{
    while (m_pHead != 0)
    {
        AgendaItem* const pNext = m_pHead->m_pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
