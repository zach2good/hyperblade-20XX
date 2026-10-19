// A pool nobody primed faults on first allocation; a null from new is stored and the loop goes on.

#include "common/compat.h"
#include "engine/gameengine.h"
#include "planvm/interactor.h"
#include "physics/collinteractorpool.h"
#include "interactorpoolcounts.h"
#include <memory>
#include "planvm/collinteractor.h"
#include "planvm/msginteractor.h"

// FUNCTION: HYPERX 0x0046ba80
void CollInteractorPool::Init()
{
    auto* pNode = m_nodes.emplace_back(std::make_unique<CollInteractor>()).get();

    m_pHead     = pNode;
    auto* pTail = pNode;

    auto nRemaining = g_wCollInteractorPoolSize;

    while (nRemaining-- > 0)
    {
        pNode = m_nodes.emplace_back(std::make_unique<CollInteractor>()).get();

        pTail->pNext = pNode;
        pTail        = pNode;
    }
}

// FUNCTION: HYPERX 0x0046bbd0
void* CollInteractorPool::Alloc()
{
    auto* const pHead = m_pHead;

    m_pHead      = pHead->pNext;
    pHead->pNext = nullptr;

    if (m_pHead == nullptr)
    {
        m_pHead = m_nodes.emplace_back(std::make_unique<CollInteractor>()).get();
    }

    return pHead;
}
