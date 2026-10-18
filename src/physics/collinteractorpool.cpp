// A pool nobody primed faults on first allocation; a null from new is stored and the loop goes on.

#include "common/compat.h"
#include "engine/gameengine.h"
#include "planvm/interactor.h"
#include "physics/collinteractorpool.h"
#include "interactorpoolcounts.h"
#include "planvm/collinteractor.h"
#include "planvm/msginteractor.h"

// FUNCTION: HYPERX 0x0046ba80
void CollInteractorPool::Init()
{
    CollInteractor* pNode = new CollInteractor;

    m_pHead               = pNode;
    CollInteractor* pTail = pNode;

    short nRemaining = g_wCollInteractorPoolSize;

    while (nRemaining-- > 0)
    {
        pNode = new CollInteractor;

        pTail->pNext = pNode;
        pTail        = pNode;
    }
}

// FUNCTION: HYPERX 0x0046bbd0
void* CollInteractorPool::Alloc()
{
    Interactor* const pHead = m_pHead;

    m_pHead      = pHead->pNext;
    pHead->pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new CollInteractor;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x0046be00
CollInteractorPool::~CollInteractorPool()
{
    while (m_pHead != nullptr)
    {
        Interactor* const pNext = m_pHead->pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
