#include "common/compat.h"
#include "common/behobjglidelclpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041f360
void BehObjGlideLCLPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_GlideLCL;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    auto nRemaining = (short)g_nPoolCountBehObjGlideLCL;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_GlideLCL;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041f410
void* BehObjGlideLCLPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_GlideLCL;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420d60
BehObjGlideLCLPool::~BehObjGlideLCLPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
