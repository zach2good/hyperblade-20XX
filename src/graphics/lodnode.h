#ifndef HYPERBLADE_LODNODE_H
#define HYPERBLADE_LODNODE_H

// The detail levels a model can carry.
#define LOD_LEVEL_COUNT 5

class LODNode
{
public:
    LODNode* Init(short nLodCount);

    ~LODNode();

    // The variable-length blob ModelComponent::Read pulls from the model file; see modelcomponent.h.
    struct ModelComponent*   m_pRecord;                      // 0x000
    struct MeshBlock**       m_apLodMeshes[LOD_LEVEL_COUNT]; // 0x004
    struct MaterialListNode* m_pMaterials;                   // 0x018, the lookup the loader stores once the materials are found
    // engineobject.cpp reads the two float runs as squared distances.
    float m_aflFarther[LOD_LEVEL_COUNT]; // 0x01c
    float m_aflNearer[LOD_LEVEL_COUNT];  // 0x030
    short m_wLodCount;                   // 0x044
    // The reference count. cd3dtexturemanager.cpp decrements it and unlinks the node at nought.
    short    m_wRefCount; // 0x046
    LODNode* m_pNext;     // 0x048
};

#endif
