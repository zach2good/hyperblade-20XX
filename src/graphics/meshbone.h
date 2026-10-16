// The world transform is the output and the local one the input.

#ifndef HYPERBLADE_MESHBONE_H
#define HYPERBLADE_MESHBONE_H
#include "common/compat.h"

#include "globals.h"
#include "graphicsglobals.h"
#include "meshblock.h"
#include "meshboundingbox.h"
#include "math/transformobject.h"

class EngineObject;
struct MeshMaterial;

// The link ID is the slot's own, not its parent's; m_vScale is a per model scale.

class MeshBoneDesc
{
public:
    short m_nLinkId;       // 0x00
    short m_nChildCount;   // 0x02
    Vec3  m_vRest;         // 0x04
    Vec3  m_vScale;        // 0x10
    int   m_nUnused;       // 0x1c
    short m_anChildIds[1]; // 0x20, m_nChildCount long
};

// MeshBoundingBox stays plain because ten collision bodies declare one as a bare local.
struct MeshBoneBlock : public MeshBoundingBox
{
    MeshBoneBlock();
};

inline MeshBoneBlock::MeshBoneBlock()
{
    vMin.x = g_flZeroMeshBone;
    vMin.y = g_flZeroMeshBone;
    vMin.z = g_flZeroMeshBone;
    vMax.x = g_flZeroMeshBone;
    vMax.y = g_flZeroMeshBone;
    vMax.z = g_flZeroMeshBone;

    bValid = 0;
}

struct FaceRecord
{
    short nFace;
    char  cFlip;
    char  cClip;
    float flZ;
};

static_assert(sizeof(FaceRecord) == 8);

// The bits of MeshBone::m_nFlags.
#define MESHBONE_UNBOUND         0x02
#define MESHBONE_ANIMATION_DIRTY 0x04
#define MESHBONE_COLOURS_PRESENT 0x08
#define MESHBONE_HIDDEN          0x10

class MeshBone
{
public:
    void __fastcall DrawFaceList(FaceRecord** ppRecords, int nCount);

    MeshBone();
    ~MeshBone();

    void PropagateTransforms(short nSlotIdx, TransformObject* pParent, TransformObject* pRoot, TransformObject* pParentLocal);
    void UpdateLeafTransform(unsigned int dwUnused, TransformObject* pParent);
    void SetFlags(unsigned short nFlags);

    // Clear once the LOD mesh has bound.
    unsigned char IsBound() const
    {
        return (unsigned char)~(unsigned char)(m_nFlags >> 1) & 1;
    }

    unsigned char IsUnbound() const
    {
        return (unsigned char)(m_nFlags >> 1) & 1;
    }

    unsigned char IsAnimationDirty() const
    {
        return (unsigned char)(m_nFlags >> 2) & 1;
    }

    unsigned char HasColours() const
    {
        return (unsigned char)(m_nFlags >> 3) & 1;
    }

    unsigned char IsHidden() const
    {
        return (unsigned char)(m_nFlags >> 4) & 1;
    }

    void ClearFlags(unsigned short nFlags);

    int  BindToLODMesh(class CD3DTextureManager* pLookup, class LODNode* pNode, class EngineObject* pOwner, short nSlot);
    int  BindToLODMeshShadowless(class CD3DTextureManager* pLookup, class LODNode* pNode, class EngineObject* pUnused, short nSlot);
    void CalcVertexColors(int nUseMdm);
    void DrawFaces();

    MeshBoundingBox* GetBoundingBox(MeshBoundingBox* pOut);
    void             ComputeBoundingSphere();
    void             PushToRenderStack(class CameraObject* pCamera);

    int m_nUnused; // 0x00
    // Both readers take 0x38 off it, which is EngineObject::m_pModelBlock.
    class EngineObject* m_pOwner;    // 0x04
    short               m_wMeshSlot; // 0x008

    MeshFace* GetFace(short nFace) const
    {
        const MeshBlock* pMesh;

        pMesh = m_pMesh;

        return pMesh->Face(nFace);
    }

    // Nought for a bone with no mesh or no vertex block.
    Vec3* GetVertexArray() const
    {
        const MeshBlock* pData;

        pData = m_pMesh;

        if (pData != 0 && pData->vertexOffset.nBytes != 0)
        {
            return pData->Vertices();
        }

        return 0;
    }

    // Nought for a bone with no mesh.
    short GetFaceCount() const
    {
        if (m_pMesh != 0)
        {
            return m_pMesh->nFaceCount;
        }

        return 0;
    }

    MeshBlock*          m_pMesh; // 0x00c
    const MeshBoneDesc* m_pDesc; // 0x10
    // Write-only: the descriptor's child ids.
    const short* m_pChildIds;  // 0x14
    MeshBone**   m_apChildren; // 0x18
    // SetFlags(8) replaces it with a zeroed buffer; the destructor deletes it.
    int* m_pFaceColours; // 0x1c
    // The bubble bound to this bone. BoundBubble::Init is the only writer and nothing reads it.
    class BoundBubble* m_pBubble;   // 0x20
    MeshBoneBlock      m_block;     // 0x24
    char               m_bHasScale; // 0x40
    Vec3               m_vCentre;   // 0x44
    float              m_flRadius;  // 0x50
    TransformObject    m_world;     // 0x54
    // Nothing in the image reads or writes these twelve bytes.
    int             m_anUnused[3]; // 0xd0
    unsigned short  m_nFlags;      // 0xdc
    TransformObject m_local;       // 0xe0

    static void StoreShadow(class TransformObject* pTarget, float flX, float flY, float flZ);

    static void __fastcall DispatchFace(MeshBone* pRend, char cFlip, MeshFace* pFaceData, MeshMaterial** ppMaterials, int* pVertexColour, struct TransformedVertex* pVertexBufBase, struct Vec3* pFaceNormal, char cClipFlags);
};

// The atexit thunk for DrawFaces' s_worldView.
// SYNTHETIC: HYPERX 0x0046f330
// $E11

#endif
