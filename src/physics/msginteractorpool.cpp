// The message interactors' pool, a free list.

#include "common/compat.h"
#include "engine/gameengine.h"
#include "planvm/interactor.h"
#include "physics/msginteractorpool.h"
#include "planvm/msginteractor.h"
#include "interactorpoolcounts.h"
#include <memory>

// A head node and then g_wMsgInteractorPoolSize more, each linked to the last.
// FUNCTION: HYPERX 0x0046bc40
void MsgInteractorPool::Init()
{
    auto* pNode = m_nodes.emplace_back(std::make_unique<MsgInteractor>()).get();

    m_pHead     = pNode;
    auto* pTail = pNode;

    auto nRemaining = g_wMsgInteractorPoolSize;

    while (nRemaining-- > 0)
    {
        pNode = m_nodes.emplace_back(std::make_unique<MsgInteractor>()).get();

        pTail->pNext = pNode;
        pTail        = pNode;
    }
}

// The list is refilled only when unlinking the head emptied it.

// FUNCTION: HYPERX 0x0046bd70
void* MsgInteractorPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead = pHead->pNext;

    pHead->pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = m_nodes.emplace_back(std::make_unique<MsgInteractor>()).get();
    }

    return pHead;
}
