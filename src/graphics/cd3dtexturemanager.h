// The model database is the same object, so the manager derives from it and adds no fields.

#ifndef HYPERBLADE_CD3DTEXTUREMANAGER_H
#define HYPERBLADE_CD3DTEXTUREMANAGER_H

#include "common/compat.h"
#include "modeldatabase.h"

class CD3DTextureManager : public ModelDatabase
{
public:
    static unsigned int __cdecl RestoreAllTextures(void);

public:
    // The body never reads this.
    int RescaleSoftTexCoords(short nMeshIndex, class LODNode* pLod, struct MaterialListNode* pList);

    char FreeTexEntry(TexCacheEntry* pEntry);
    char ReleaseTexEntry(TexCacheEntry* pEntry);
    char ReleaseModel(class LODNode* pLod);
    char DeactivateMaterialLookup(class LODNode* pLod, struct MaterialListNode* pBlock);
    char ReleaseLOD(class LODNode* pLod, struct MaterialListNode* pBlock);

    // The body never reads the object; both callers pass g_pTexManager.
    int LookupMeshForSlot(class LODNode* pNode, short nLevel, short nSlot, struct MeshBlock** ppOut);
};

#endif
