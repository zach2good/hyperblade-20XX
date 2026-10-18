#include "common/compat.h"
#include "lodnode.h"
#include <cstring>

// FUNCTION: HYPERX 0x00472740
LODNode* LODNode::Init(short nLodCount)
{
    m_pRecord    = nullptr;
    m_wLodCount  = nLodCount;
    m_pMaterials = nullptr;
    m_wRefCount  = 0;
    m_pNext      = nullptr;

    for (auto& pSlots : m_apLodMeshes)
    {
        pSlots = nullptr;
    }

    memset(m_aflFarther, 0, sizeof(m_aflFarther));
    memset(m_aflNearer, 0, sizeof(m_aflNearer));

    return this;
}

LODNode::LODNode() = default;

// Unlinks the rest of the list one node at a time, so a long list cannot exhaust the stack.
// FUNCTION: HYPERX 0x004727a0
LODNode::~LODNode()
{
    auto pNext = std::move(m_pNext);

    while (pNext != nullptr)
    {
        pNext = std::move(pNext->m_pNext);
    }
}
