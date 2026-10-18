#include "common/compat.h"
#include "common/behobjmonitorkeyboardpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f920
void BehObjMonitorKeyboardPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_MonitorKeyboard;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = g_wPoolCountBehObjMonitorKeyboard;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_MonitorKeyboard;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f9e0
void* BehObjMonitorKeyboardPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_MonitorKeyboard;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421080
BehObjMonitorKeyboardPool::~BehObjMonitorKeyboardPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
