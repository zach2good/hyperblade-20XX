#include "common/compat.h"
#include "common/behobjglidetoobjectpool.h"
#include "pooledclasses.h"
#include "poolcounts.h"

// FUNCTION: HYPERX1000 0x004c88b0
// FUNCTION: HYPERX1001 0x0041ec50
void BehObjGlideToObjectPool::Init()
{
    GameBehaviorObject* pNode = new BehObj_GlideToObject;

    m_pHead                   = pNode;
    GameBehaviorObject* pPool = pNode;

    short nRemaining = (short)g_nPoolCountBehObjGlideToObject;

    while (nRemaining-- > 0)
    {
        pNode = new BehObj_GlideToObject;

        pPool->m_pStandby = pNode;
        pPool             = pNode;
    }
}

// Every behaviour pool's copy folds onto this one; v1.0.0.0 keeps them apart.
// FUNCTION: HYPERX1000 0x004c89d0
// FUNCTION: HYPERX1001 0x0041ed00
char BehObjGlideToObjectPool::Insert(void* pNode)
{
    GameBehaviorObject* const pItem = (GameBehaviorObject*)pNode;

    pItem->m_pStandby = m_pHead;
    m_pHead           = pItem;

    return 1;
}

// FUNCTION: HYPERX1000 0x004c9ef0
// FUNCTION: HYPERX1001 0x0041ed20
void* BehObjGlideToObjectPool::Alloc()
{
    GameBehaviorObject* const pHead = m_pHead;

    m_pHead           = pHead->m_pStandby;
    pHead->m_pStandby = 0;

    if (m_pHead == 0)
    {
        m_pHead = new BehObj_GlideToObject;
    }

    return pHead;
}

// FUNCTION: HYPERX1001 0x00420970
BehObjGlideToObjectPool::~BehObjGlideToObjectPool()
{
    while (m_pHead != 0)
    {
        GameBehaviorObject* const pNext = m_pHead->m_pStandby;

        delete m_pHead;

        m_pHead = pNext;
    }
}
