#ifndef HYPERBLADE_MODELCOMPONENT_H
#define HYPERBLADE_MODELCOMPONENT_H

#include "graphics/recordoffset.h"
#include "math/vector.h"
#include "graphics/shadowblock.h"

class MeshBoneDesc;
class FileObj;

struct ModelComponent
{
    // Free functions in the original.
    static void* __cdecl           Read(FileObj* pFile, int* pnSize);
    static ModelComponent* __cdecl ReadFirst(FileObj* pFile);

    const MeshBoneDesc* SlotDesc(short nSlot) const
    {
        return m_aSlotOffsets[nSlot].In(this);
    }

    int   m_nSize;          // 0x00
    short m_nId;            // 0x04
    char  m_szName[32];     // 0x06
    short m_nSubComponents; // 0x26
    // Every shipped MDLTXT WBM file holds zeros here.
    char m_acUnused[44]; // 0x28
    Vec3 m_vAngle;       // 0x54
    // Zero when there is no shadow block.
    RecordOffset<ShadowBlock> m_shadowOffset; // 0x60
    // One entry per slot. C++98 has no flexible array member, so the length is declared as one.
    RecordOffset<const MeshBoneDesc> m_aSlotOffsets[1]; // 0x62

    static int __cdecl ReadAll(FileObj* pFile, struct MeshBlock*** pppBlocks, unsigned short* pnCount);

    ShadowBlock* GetShadowBlock()
    {
        return m_shadowOffset.In(this);
    }
};

#endif
