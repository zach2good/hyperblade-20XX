// The message interactors' pool, a free list.

#include "common/compat.h"
#include "engine/gameengine.h"
#include "planvm/interactor.h"
#include "physics/msginteractorpool.h"
#include "planvm/msginteractor.h"
#include "interactorpoolcounts.h"

// A head node and then g_wMsgInteractorPoolSize more, each linked to the last.
// FUNCTION: HYPERX 0x0046bc40
void MsgInteractorPool::Init()
{
    auto* pNode = new MsgInteractor;

    m_pHead              = pNode;
    MsgInteractor* pTail = pNode;

    short nRemaining = g_wMsgInteractorPoolSize;

    while (nRemaining-- > 0)
    {
        pNode = new MsgInteractor;

        pTail->pNext = pNode;
        pTail        = pNode;
    }
}

// The list is refilled only when unlinking the head emptied it.

// FUNCTION: HYPERX 0x0046bd70
void* MsgInteractorPool::Alloc()
{
    Interactor* const pHead = m_pHead;

    m_pHead = pHead->pNext;

    pHead->pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = new MsgInteractor;
    }

    return pHead;
}

// FUNCTION: HYPERX 0x0046bed0
MsgInteractorPool::~MsgInteractorPool()
{
    while (m_pHead != nullptr)
    {
        Interactor* const pNext = m_pHead->pNext;

        delete m_pHead;

        m_pHead = pNext;
    }
}
