#include "common/compat.h"
#include "common/behobjopenandclosepool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x00420020
void BehObjOpenAndClosePool::Init()
{
    GameBehaviorObject* pNode = new BehObj_OpenAndClose;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    auto nRemaining = (short)g_nPoolCountBehObjOpenAndClose;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_OpenAndClose;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x004200d0
void* BehObjOpenAndClosePool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_OpenAndClose;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00421440
BehObjOpenAndClosePool::~BehObjOpenAndClosePool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
