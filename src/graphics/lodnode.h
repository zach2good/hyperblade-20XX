#ifndef HYPERBLADE_LODNODE_H
#define HYPERBLADE_LODNODE_H

#include "common/record.h"
#include "graphics/meshblock.h"
#include "graphics/modelcomponent.h"
#include "graphics/modeldatabase.h"
#include <memory>

// The detail levels a model can carry.
constexpr auto kLODLevelCount = 5;

class LODNode
{
public:
    LODNode* Init(short nLodCount);

    LODNode();
    ~LODNode();
    LODNode(const LODNode&)            = delete;
    LODNode& operator=(const LODNode&) = delete;
    LODNode(LODNode&&)                 = delete;
    LODNode& operator=(LODNode&&)      = delete;

    // The variable-length blob ModelComponent::Read pulls from the model file; see modelcomponent.h.
    Record<ModelComponent>               m_pRecord;                     // 0x000
    std::unique_ptr<Record<MeshBlock>[]> m_apLodMeshes[kLODLevelCount]; // 0x004
    std::unique_ptr<MaterialListNode>    m_pMaterials;                  // 0x018, the lookup the loader stores once the materials are found
    // engineobject.cpp reads the two float runs as squared distances.
    float m_aflFarther[kLODLevelCount]; // 0x01c
    float m_aflNearer[kLODLevelCount];  // 0x030
    short m_wLodCount;                  // 0x044
    // The reference count. cd3dtexturemanager.cpp decrements it and unlinks the node at nought.
    short                    m_wRefCount; // 0x046
    std::unique_ptr<LODNode> m_pNext;     // 0x048
};

#endif
