#ifndef HYPERBLADE_MODELCOMPONENT_H
#define HYPERBLADE_MODELCOMPONENT_H

#include "common/record.h"
#include "graphics/recordoffset.h"
#include "math/vector.h"
#include "graphics/shadowblock.h"
#include <vector>

class MeshBoneDesc;
class FileObj;

struct ModelComponent
{
    // Free functions in the original.
    template <class T>
    static Record<T>              Read(FileObj* pFile, int* pnSize);
    static Record<ModelComponent> ReadFirst(FileObj* pFile);

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

    // False when a record is damaged. The list then stops before it, and the file moves to the list's end.
    static bool ReadAll(FileObj* pFile, std::vector<Record<struct MeshBlock> >* pBlocks);

    ShadowBlock* GetShadowBlock()
    {
        return m_shadowOffset.In(this);
    }
};

#endif
