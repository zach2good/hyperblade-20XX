#ifndef HYPERBLADE_NODEPOOL_H
#define HYPERBLADE_NODEPOOL_H

#include <memory>
#include <vector>

#include "common/pooledownerroot.h"

// Owns every node it ever built; the free list threads the spare ones through Link.
template <typename Base, Base* Base::* Link, typename T, auto& Count>
class NodePool final : public PooledOwnerRoot
{
public:
    void Init() override
    {
        m_pHead     = m_apNodes.emplace_back(std::make_unique<T>()).get();
        auto* pTail = m_pHead;

        auto nRemaining = static_cast<short>(Count);

        while (nRemaining-- > 0)
        {
            auto* const pNode = m_apNodes.emplace_back(std::make_unique<T>()).get();

            pTail->*Link = pNode;
            pTail        = pNode;
        }
    }

    // FUNCTION: HYPERX 0x00414990
    // FUNCTION: HYPERX 0x0041ed00
    char Insert(void* pNode) override
    {
        auto* const pItem = static_cast<Base*>(pNode);

        pItem->*Link = m_pHead;
        m_pHead      = pItem;

        return 1;
    }

    void* Alloc() override
    {
        auto* const pHead = m_pHead;

        m_pHead      = pHead->*Link;
        pHead->*Link = nullptr;

        if (m_pHead == nullptr)
        {
            m_pHead = m_apNodes.emplace_back(std::make_unique<T>()).get();
        }

        return pHead;
    }

    // Destroys a lent node outright instead of taking it back.
    void Free(const Base* pNode)
    {
        std::erase_if(
            m_apNodes,
            [&](const auto& pOwned)
            {
                return pOwned.get() == pNode;
            });
    }

private:
    std::vector<std::unique_ptr<Base> > m_apNodes;
    Base*                               m_pHead = nullptr;
};

#endif
