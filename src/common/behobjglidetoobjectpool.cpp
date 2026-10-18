#include "common/compat.h"
#include "common/behobjglidetoobjectpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX 0x0041ec50
void BehObjGlideToObjectPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_GlideToObject;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    auto nRemaining = static_cast<short>(g_nPoolCountBehObjGlideToObject);

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_GlideToObject;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// FUNCTION: HYPERX 0x0041ed20
void* BehObjGlideToObjectPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new BehObj_GlideToObject;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x00420970
BehObjGlideToObjectPool::~BehObjGlideToObjectPool()
{
    while (m_pHead != nullptr)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
