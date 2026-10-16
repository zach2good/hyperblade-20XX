#ifndef HYPERBLADE_MODELDATABASE_H
#define HYPERBLADE_MODELDATABASE_H

#include "graphics/materialname.h"

#include "common/compat.h"

class FileObj;
class LODNode;
class TexCacheEntry;

// The index stored in an entry is one based.

struct MaterialListNode
{
    explicit MaterialListNode(unsigned char nMatCount)
    {
        m_pMaterials = 0;
        m_pszName    = 0;
        m_nMatCount  = nMatCount;
        m_pNext      = 0;
        m_nRefs      = 0;
    }

    const char* GetName() const
    {
        return m_pszName;
    }

    struct MeshMaterial* m_pMaterials; // 0x000
    char*                m_pszName;    // 0x004, strcmp'd against the model name when the list is looked up
    unsigned char        m_nMatCount;  // 0x008, the bound of the material loop in engineobject.cpp
    MaterialListNode*    m_pNext;      // 0x00c
    short                m_nRefs;      // 0x010, incremented when a model takes the list and compared to MATERIAL_SHARED
};

class ModelDatabase
{
public:
    class TexCacheEntry* FindOrLoadTexture(char* pszName);
    MaterialListNode*    FindOrLoadMaterialList(LODNode* pLod, char* pszName);
    MaterialListNode*    CreateMaterialList(MeshMaterial* pMaterials, unsigned char nMatCount, MaterialName* paTexNames, short nTexCount);
    void                 ReadMaterials(FileObj* pFile, LODNode* pLodNode);
    LODNode*             LoadLOD(char* pszModelName);
    LODNode*             FindLOD(const char* pszName);
    short                LinkComponents(FileObj* pFile, LODNode* pLodNode);
    void                 ComputeNormals(struct MeshBlock* pBlob, class LODNode* pUnused);
    ~ModelDatabase();

    // Game::InitializeEngine adds a delta to the install level straight after the new.
    ModelDatabase()
    : m_nInstallLevel(0)
    , m_pListHead(0)
    , m_pListTail(0)
    , m_pTextureList(0)
    , m_nNodeCount(0)
    , m_nFaceMax(0)
    , m_nVertexMax(0)
    , m_nNextIndex(0)
    , m_wUnusedA(0)
    , m_wUnusedB(0)
    , m_flLodHysteresis(5.0f)
    {
    }

    int            m_nInstallLevel; // 0x00
    LODNode*       m_pListHead;     // 0x04
    LODNode*       m_pListTail;     // 0x08
    TexCacheEntry* m_pTextureList;  // 0x0c
    short          m_nNodeCount;    // 0x10
    short          m_nFaceMax;      // 0x12
    short          m_nVertexMax;    // 0x14
    short          m_nNextIndex;    // 0x16
    // Zeroed by the constructor and read by nothing else in the image.
    short m_wUnusedA; // 0x18
    short m_wUnusedB; // 0x1a

    int m_anUnusedC[4]; // 0x1c

    // How far a level-of-detail threshold has to be crossed before the model swaps.
    float m_flLodHysteresis; // 0x30
};

#endif
