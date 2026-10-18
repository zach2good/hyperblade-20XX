#include "common/compat.h"
#include <algorithm>
#include "planvm/planvm.h"
#include "math/mathglobals.h"
#include "math/transformobject.h"
#include "meshbone.h"
#include "engine/devicector.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "math/vector.h"
#include "lodnode.h"
#include "modeldatabase.h"
#include "meshmaterial.h"
#include "engine/gameengine.h"
#include "engine/cameraobject.h"
#include "engine/objectclasses.h"
#include "ui/hudglobals.h"
#include "transparentqueue.h"
#include "graphics/meshboundingbox.h"
#include "graphics/transformedvertex.h"
#include "colorpalette.h"
#include "common/staticinit.h"
#include "cd3dtexturemanager.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <new>
#include "app/gameapp.h"
#include "engine/engineobject.h"
#include "graphics/face.h"
#include "graphics/swrend.h"
#include "graphics/hwrenderer.h"
#include "graphics/hwmodels.h"
#include "graphics/graphicssettings.h"
#include <array>
#include "engine/objectclasses.h"
#include "modelcomponent.h"
#include "engine/cameraprojectionstate.h"

extern const float g_flZeroVertexColour    = 0.0f;
float              g_flUnusedVertexColourA = g_flZeroVertexColour;
float              g_flUnusedVertexColourB = g_flZeroVertexColour;
float              g_flUnusedVertexColourC = g_flZeroVertexColour;

// FUNCTION: HYPERX 0x0044be10
void MeshBone::DrawFaceList(FaceRecord** ppRecords, int nCount)
{
    SWRend::DrawFaceList(this, ppRecords, (short)nCount);
}

// FUNCTION: HYPERX 0x0044be40
void MeshBone::DispatchFace(MeshBone* pRend, char cFlip, MeshFace* pFaceData, MeshMaterial** ppMaterials, int* pVertexColour, struct TransformedVertex* pVertexBufBase, struct Vec3* pFaceNormal, char cClipFlags)
{
    SWRend::DrawFace(pRend, cFlip, pFaceData, ppMaterials, pVertexColour, pVertexBufBase, pFaceNormal, cClipFlags);
}

// The animation scale is one shot: applying it twice would shrink the bone.
// FUNCTION: HYPERX 0x0046ef10
void MeshBone::PropagateTransforms(short nSlotIdx, TransformObject* pParent, TransformObject* pRoot, TransformObject*)
{
    if ((m_nFlags & MESHBONE_ANIMATION_DIRTY) != 0)
    {
        const MeshBoneDesc* pDesc = m_pDesc;

        m_local.SetTranslation(m_local.m_a[0][3] * pDesc->m_vScale.x,
                               m_local.m_a[1][3] * pDesc->m_vScale.y,
                               m_local.m_a[2][3] * pDesc->m_vScale.z);

        ClearFlags(MESHBONE_ANIMATION_DIRTY);
    }

    pParent->Multiply(&m_world, &m_local);

    if (m_pDesc->m_nChildCount != 0)
    {
        for (short i = 0; i < m_pDesc->m_nChildCount; i++)
        {
            MeshBone* const pChild = m_apChildren[i];

            // There is no visited set, so a link ID cycle of three would recurse forever.
            if (pChild->m_pDesc->m_nLinkId != nSlotIdx)
            {
                pChild->PropagateTransforms(m_pDesc->m_nLinkId, &m_world, pRoot, &m_local);
            }
        }
    }

    // After the children, so scale is never inherited.
    if (m_bHasScale != 0)
    {
        m_world.ApplyScale();
    }
}

// FUNCTION: HYPERX 0x0046efe0
void MeshBone::UpdateLeafTransform(unsigned int, TransformObject* pParent)
{
    if ((m_nFlags & MESHBONE_ANIMATION_DIRTY) != 0)
    {
        const MeshBoneDesc* pDesc = m_pDesc;

        m_local.SetTranslation(m_local.m_a[0][3] * pDesc->m_vScale.x,
                               m_local.m_a[1][3] * pDesc->m_vScale.y,
                               m_local.m_a[2][3] * pDesc->m_vScale.z);

        ClearFlags(MESHBONE_ANIMATION_DIRTY);
    }

    pParent->Multiply(&m_world, &m_local);

    if (m_bHasScale != 0)
    {
        m_world.ApplyScale();
    }
}

// Float one.
// GLOBAL: HYPERX 0x004b38dc
extern const float g_flVertexW;

// GLOBAL: HYPERX 0x004d0930
extern Vec3 g_vLightDirModel;

extern class TransformObject* g_pCameraTransform;
extern int                    g_bDromePass;

// The block goes to the GPU whole: the bone's place in the camera's pixels, its light and its
// viewport, with nothing transformed or rasterised here.
void MeshBone::SubmitToHardware()
{
    if (m_pMesh == nullptr || m_pMesh->nFaceCount <= 0 || m_pMesh->vertexOffset.nBytes == 0)
    {
        return;
    }

    auto bone = HwModels::Bone{ m_pMesh, m_pOwner->m_pModelBlock->m_pMaterials, m_pOwner->m_pModelBlock->m_nMatCount, &m_world };

    const auto bArena = m_pOwner == g_pGameEngine->m_ppSlots[SLOT_DROME];

    // The drome is textured only while the original's F8 switch says so.
    const auto bWantTextures = GraphicsSettings::On(GraphicsSettings::Switch::TEXTURES) && (!bArena || (g_nDromeTextured != 0 && g_bShadedDromeModels == 0));
    const auto bTextured     = bWantTextures && HwModels::Find(*this, HwRenderer::Frame(), bone);

    TransformObject pixelFromModel;

    g_pCameraTransform->Multiply(&pixelFromModel, bone.pWorld);

    auto draw = HwRenderer::Draw{};

    draw.pBlock         = bone.pBlock;
    draw.pMaterials     = bone.pMaterials;
    draw.nMaterialCount = bone.nMaterialCount;
    draw.bPalettised    = !bTextured;

    for (int nRow = 0; nRow < 3; nRow++)
    {
        for (int nColumn = 0; nColumn < 4; nColumn++)
        {
            draw.aPixelFromModel[nRow * 4 + nColumn] = pixelFromModel.m_a[nRow][nColumn];
        }
    }

    // Without a fourth row the engine divides by one, as DrawFaces does.
    for (int nColumn = 0; nColumn < 4; nColumn++)
    {
        const auto flIdentity = [&]
        {
            if (nColumn == 3)
            {
                return 1.0f;
            }

            return 0.0f;
        }();

        draw.aPixelFromModel[12 + nColumn] = [&]
        {
            if (pixelFromModel.m_bHasRow3 != 0)
            {
                return pixelFromModel.m_a[3][nColumn];
            }

            return flIdentity;
        }();
    }

    draw.bArena = bArena;

    // The scene's lights, in the order GraphicsSettings numbers them.
    const auto aWorldLights = std::array<Vec3, 3>{ g_lightDirs[1].dir, g_lightDirs[2].dir, g_vLightDirExtra };

    // The arena is lit as Renderer::DrawAllMeshes bakes it: each bone by its own light, along the
    // drome pass's curve. With more than two bones the first takes the extra light. Everything
    // else takes the light the transparent pass picked for it.
    draw.nKeyLight = [&]
    {
        if (!draw.bArena)
        {
            const auto& vPicked = g_lightDirs[0].dir;

            if (vPicked.x == aWorldLights[1].x && vPicked.y == aWorldLights[1].y && vPicked.z == aWorldLights[1].z)
            {
                return 1;
            }

            return 0;
        }

        auto nBone = (int)(this - m_pOwner->m_pBones);

        if (m_pOwner->GetBoneCount() > 2)
        {
            if (nBone == 0)
            {
                return 2;
            }

            nBone--;
        }

        if (nBone == 0)
        {
            return 0;
        }

        return 1;
    }();

    using GraphicsSettings::Switch;

    // The original lights each face by its key light alone; with several lights the others fill in.
    constexpr auto FILL_STRENGTH  = 0.4f;
    constexpr auto LIGHT_SWITCHES = std::array<Switch, 3>{ Switch::LIGHT_303, Switch::LIGHT_54, Switch::LIGHT_180 };

    auto world = *bone.pWorld;

    for (int i = 0; i < 3; i++)
    {
        Vec3 vLight;

        world.TransformDirectionByRows(aWorldLights[(size_t)i], &vLight);

        const auto flStrength = [&]
        {
            if (!GraphicsSettings::On(LIGHT_SWITCHES[(size_t)i]))
            {
                return 0.0f;
            }

            if (i == draw.nKeyLight)
            {
                return 1.0f;
            }

            if (GraphicsSettings::On(Switch::SEVERAL_LIGHTS))
            {
                return FILL_STRENGTH;
            }

            return 0.0f;
        }();

        draw.aLightDirs[i][0] = vLight.x;
        draw.aLightDirs[i][1] = vLight.y;
        draw.aLightDirs[i][2] = vLight.z;
        draw.aLightDirs[i][3] = flStrength;
    }

    for (int nRow = 0; nRow < 3; nRow++)
    {
        for (int nColumn = 0; nColumn < 4; nColumn++)
        {
            draw.aWorldFromModel[nRow * 4 + nColumn] = bone.pWorld->m_a[nRow][nColumn];
        }
    }

    draw.aWorldFromModel[15] = 1.0f;

    // The rok shows one bone at a time, by its state, and carries its light on whichever it is.
    if (GraphicsSettings::On(Switch::ROK_LIGHT) && m_pOwner == g_pGameEngine->m_ppSlots[SLOT_ROK])
    {
        HwRenderer::SetPointLight(bone.pWorld->m_a[0][3], bone.pWorld->m_a[1][3], bone.pWorld->m_a[2][3], GraphicsSettings::RokLightRadius());
    }

    draw.nViewportX      = g_nViewportOriginX;
    draw.nViewportY      = g_nViewportOriginY;
    draw.nViewportWidth  = g_nPrevWidth;
    draw.nViewportHeight = g_nPrevHeight;

    HwRenderer::Submit(draw);
}

// The drawer walks the face list backwards from one past the last entry.
// FUNCTION: HYPERX 0x0046f060
void MeshBone::DrawFaces()
{
    const Vec3*            pVertBase;
    int                    nResult;
    char                   cClip;
    static TransformObject s_worldView;

    MeshBlock* pBlob = m_pOwner->m_pLod->m_apLodMeshes[m_pOwner->m_wLodLevel][m_wMeshSlot];
    m_pMesh          = pBlob;

    if (HwRenderer::Capturing())
    {
        SubmitToHardware();

        return;
    }

    if (pBlob == nullptr || pBlob->vertexOffset.nBytes == 0)
    {
        pVertBase = nullptr;
    }
    else
    {
        pVertBase = pBlob->Vertices();
    }

    TransformedVertex*     pOut = g_pModelVertexBuf;
    TransformObject* const pXf  = &m_world;

    g_pCameraTransform->Multiply(&s_worldView,
                                 pXf);

    short nVerts = m_pMesh->nVertexCount;
    int   nIndex = 0;

    while (nVerts-- > 0)
    {
        pOut->x = (s_worldView.m_a[0][0] * pVertBase[nIndex].x + (s_worldView.m_a[0][1] * pVertBase[nIndex].y)) + s_worldView.m_a[0][2] * pVertBase[nIndex].z + s_worldView.m_a[0][3];
        pOut->y = ((s_worldView.m_a[1][0] * pVertBase[nIndex].x) + s_worldView.m_a[1][2] * pVertBase[nIndex].z) + s_worldView.m_a[1][1] * pVertBase[nIndex].y + s_worldView.m_a[1][3];
        pOut->z = ((s_worldView.m_a[2][0] * pVertBase[nIndex].x) + s_worldView.m_a[2][2] * pVertBase[nIndex].z) + s_worldView.m_a[2][1] * pVertBase[nIndex].y + s_worldView.m_a[2][3];

        if (s_worldView.m_bHasRow3 != 0)
        {
            pOut->w = ((s_worldView.m_a[3][0] * pVertBase[nIndex].x) + s_worldView.m_a[3][2] * pVertBase[nIndex].z) + s_worldView.m_a[3][1] * pVertBase[nIndex].y + s_worldView.m_a[3][3];
        }
        else
        {
            pOut->w = g_flVertexW;
        }

        nIndex++;
        pOut++;
    }

    // Never false.
    if (pXf != nullptr)
    {
        pXf->TransformDirectionByRows(g_lightDirs[0].dir, &g_vLightDirModel);
    }
    else
    {
        g_vLightDirModel = g_lightDirs[0].dir;
    }

    FaceRecord** ppSlot = g_ppFaceList + 1;
    FaceRecord*  pEntry = g_pFaceRecords;

    for (short nFace = 0; nFace < GetFaceCount(); nFace++)
    {
        MeshFace* const     pFace = GetFace(nFace);
        MeshMaterial* const pMat  = &m_pOwner->m_pModelBlock->m_pMaterials[(int)pFace->cMaterial];

        nResult = Face::BackfaceCullAndClipCheck(pFace, g_pModelVertexBuf, (char)(pMat->nBackIndex >= 0), &cClip);

        if (nResult == -1)
        {
            *ppSlot       = pEntry;
            pEntry->nFace = nFace;
            pEntry->cFlip = 1;
        }
        else if (nResult == 1)
        {
            *ppSlot       = pEntry;
            pEntry->nFace = nFace;
            pEntry->cFlip = 0;
        }
        else
        {
            continue;
        }

        ppSlot++;

        const TransformedVertex* pVertexBuf = g_pModelVertexBuf;

        pEntry->flZ   = (pVertexBuf[pFace->aCorners[2]].w + pVertexBuf[pFace->aCorners[1]].w + pVertexBuf[pFace->aCorners[0]].w) * 0.33333334f;
        pEntry->cClip = cClip;

        pEntry++;
    }

    pBlob            = m_pMesh;
    const int nCount = (int)(pEntry - g_pFaceRecords);

    if (pBlob->SortsFaces() != 0)
    {
        SWRend::SortFaces((short)nCount);
    }

    if ((short)nCount > 0)
    {
        DrawFaceList(g_ppFaceList + (short)nCount, nCount);
    }
}

// The queue is one based, and the overflow guard tests the count before the rise.
// FUNCTION: HYPERX 0x0046f340
void MeshBone::PushToRenderStack(CameraObject* pCamera)
{
    const float (*pRows)[4];
    float flWorldX;
    float flWorldY;
    float flWorldZ;
    float flSortZ;

    const CameraProjectionState* pView = pCamera->m_pView;
    pRows                              = m_world.m_a;
    const Vec3* pOrigin                = &m_pMesh->vOrigin;

    flWorldX = m_world.m_a[0][0] * pOrigin->x + (pRows[0][1] * pOrigin->y + pRows[0][2] * pOrigin->z) + pRows[0][3];
    flWorldY = pRows[1][0] * pOrigin->x + (pRows[1][1] * pOrigin->y + pRows[1][2] * pOrigin->z) + pRows[1][3];
    flWorldZ = pRows[2][0] * pOrigin->x + (pRows[2][1] * pOrigin->y + pRows[2][2] * pOrigin->z) + pRows[2][3];

    g_renderPushEntry.pComponent = this;

    flSortZ = flWorldX * pView->m_a[2][0] + flWorldY * pView->m_a[2][1] + flWorldZ * pView->m_a[2][2] + pView->m_a[2][3] - m_pMesh->flDepthBias;

    g_renderPushEntry.flSortZ  = flSortZ;
    g_renderPushEntry.flWorldY = flWorldY;

    if (g_nTransparentCount > g_pGameEngine->m_nSlotCount)
    {
        // STRING: HYPERX 0x004c4fb0
        g_gameApp.Fatal("STACK OVERFLOW in pushModComp!");
    }

    g_nTransparentCount++;

    g_pTransparentQueue[g_nTransparentCount] = g_renderPushEntry;
}

// The first word of the box is its valid flag. The mesh pointer is not checked, as in the original.

// FUNCTION: HYPERX 0x0046f430
MeshBoundingBox* MeshBone::GetBoundingBox(MeshBoundingBox* pOut)
{
    Vec3* pVerts;

    if (m_block.bValid == 0)
    {
        MeshBlock* const pMesh = m_pMesh;

        if (pMesh == nullptr || pMesh->vertexOffset.nBytes == 0)
        {
            pVerts = nullptr;
        }
        else
        {
            pVerts = pMesh->Vertices();
        }

        short nCount = pMesh->nVertexCount;
        int   nIndex = 0;

        while (nCount-- > 0)
        {
            const Vec3* const pVert = &pVerts[nIndex];

            m_block.vMin.x = (std::min)(m_block.vMin.x, pVert->x);
            m_block.vMin.y = (std::min)(m_block.vMin.y, pVert->y);
            m_block.vMin.z = (std::min)(m_block.vMin.z, pVert->z);
            m_block.vMax.x = (std::max)(m_block.vMax.x, pVert->x);
            m_block.vMax.y = (std::max)(m_block.vMax.y, pVert->y);
            m_block.vMax.z = (std::max)(m_block.vMax.z, pVert->z);

            nIndex++;
        }

        m_block.bValid = 1;
    }

    *pOut = m_block;

    return pOut;
}

// The null test on m_world's matrix is dead, as in the original.

// FUNCTION: HYPERX 0x0046f530
void MeshBone::CalcVertexColors()
{
    if (m_pFaceColours == nullptr)
    {
        SetFlags(MESHBONE_COLOURS_PRESENT);
    }

    if (HasColours() == 0)
    {
        m_nFlags = (unsigned short)(m_nFlags | MESHBONE_COLOURS_PRESENT);
    }

    const TransformObject* pXf = &m_world;

    // Never false.
    if (pXf != nullptr)
    {
        Vec3 vLight = g_lightDirs[0].dir;

        g_vLightDirModel.x = vLight.x * pXf->m_a[0][0] + vLight.y * pXf->m_a[1][0] + vLight.z * pXf->m_a[2][0];
        g_vLightDirModel.y = vLight.x * pXf->m_a[0][1] + vLight.y * pXf->m_a[1][1] + vLight.z * pXf->m_a[2][1];
        g_vLightDirModel.z = vLight.x * pXf->m_a[0][2] + vLight.y * pXf->m_a[1][2] + vLight.z * pXf->m_a[2][2];
    }
    else
    {
        g_vLightDirModel = g_lightDirs[0].dir;
    }

    for (short i = 0; i < GetFaceCount(); i++)
    {
        unsigned int nColour;

        MeshFace* pFace = GetFace(i);

        MeshMaterial* pMaterial = m_pOwner->m_pModelBlock->m_pMaterials + pFace->cMaterial;

        const Vec3* pNormal = &pFace->vNormal;

        unsigned char cIndex;
        char          cShade;
        float         flIntensity;

        int nRangeFirst;

        cIndex = pMaterial->acChannel[0];

        nRangeFirst = g_pColorPalette->m_rampBounds.nFirst;

        if (cIndex < nRangeFirst || cIndex > g_pColorPalette->m_rampBounds.nCount)
        {
            nColour = pMaterial->nColour;
        }
        else
        {
            flIntensity = g_flZeroVertexColour;

            if (pNormal == nullptr)
            {
                cShade = SHADE_WHITE;
            }
            else
            {
                flIntensity = (g_vLightDirModel.x * pNormal->x + g_vLightDirModel.y * pNormal->y + g_vLightDirModel.z * pNormal->z) * pMaterial->flDiffuse + pMaterial->flAmbient;
                flIntensity += pMaterial->flEmissive;

                if (flIntensity > g_flZeroVertexColour)
                {
                    if (flIntensity < g_flVertexW)
                    {
                        cShade = SHADE_SCALED;
                    }
                    else
                    {
                        cShade = SHADE_WHITE;
                    }
                }
                else
                {
                    cShade = SHADE_BLACK;
                }
            }

            nColour = (((ColorPalette::LookupColorIntensity(
                             g_pColorPalette, cShade, flIntensity, cIndex)
                             << 8 |
                         ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flIntensity, cIndex))
                            << 8 |
                        ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flIntensity, cIndex))
                       << 8) |
                      ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flIntensity, cIndex);
        }

        m_pFaceColours[i] = nColour;
    }
}

// A mask of exactly 8 replaces the per-face colour buffer with a zeroed one.

// FUNCTION: HYPERX 0x0046f8c0
void MeshBone::SetFlags(unsigned short nFlags)
{
    if (nFlags == MESHBONE_COLOURS_PRESENT)
    {
        if (m_pFaceColours != nullptr)
        {
            delete[] m_pFaceColours;
        }

        const short nFaceCount = GetFaceCount();

        m_pFaceColours = new int[nFaceCount];

        memset(m_pFaceColours, 0, nFaceCount * sizeof(int));
    }

    m_nFlags |= nFlags;
}

// Clearing the colours flag alone releases the per-face colour buffer.
// FUNCTION: HYPERX 0x0046f930
void MeshBone::ClearFlags(unsigned short nFlags)
{
    m_nFlags &= ~nFlags;

    if (nFlags == MESHBONE_COLOURS_PRESENT && m_pFaceColours != nullptr)
    {
        delete[] m_pFaceColours;

        m_pFaceColours = nullptr;
    }
}

// m_block is filled by its constructor and then cleared again, as in the original.
// FUNCTION: HYPERX 0x0046fa50
MeshBone::MeshBone()
: m_nUnused(0)
, m_pOwner(nullptr)
, m_pDesc(nullptr)
, m_pChildIds(nullptr)
, m_apChildren(nullptr)
, m_pFaceColours(nullptr)
, m_pBubble(nullptr)
, m_bHasScale(0)
, m_flRadius(0)
{
    memset((void*)&m_block, 0, sizeof(m_block));

    m_vCentre.x = m_vCentre.y = m_vCentre.z = 0;
}

// FUNCTION: HYPERX 0x0046fb10
MeshBone::~MeshBone()
{
    if (m_apChildren != nullptr)
    {
        delete[] m_apChildren;

        m_apChildren = nullptr;
    }

    if (m_pFaceColours != nullptr)
    {
        delete[] m_pFaceColours;

        m_pFaceColours = nullptr;
    }
}

inline void MeshBone::StoreShadow(TransformObject* pTarget, float flX, float flY, float flZ)
{
    pTarget->m_vTranslate.x = flX;
    pTarget->m_vTranslate.y = flY;
    pTarget->m_vTranslate.z = flZ;
}

// A lookup answer of 1 computes the bounding sphere; any other sets flag two.
// FUNCTION: HYPERX 0x0046fba0
int MeshBone::BindToLODMesh(CD3DTextureManager* pLookup, LODNode* pNode, EngineObject* pOwner, short nSlot)
{
    const MeshBoneDesc* pEntry;

    m_pOwner = pOwner;

    m_nFlags    = 0;
    m_wMeshSlot = nSlot;

    const int nResult = pLookup->LookupMeshForSlot(pNode, 0, nSlot, &m_pMesh);

    if (nResult != -1)
    {
        if (nResult == 1)
        {
            ComputeBoundingSphere();
        }
        else
        {
            SetFlags(MESHBONE_UNBOUND);
        }

        if (pNode->m_pRecord != nullptr)
        {
            pEntry = pNode->m_pRecord->SlotDesc(nSlot);
        }
        else
        {
            pEntry = nullptr;
        }

        m_pDesc     = pEntry;
        m_pChildIds = pEntry->m_anChildIds;

        // A second name keeps the client's schedule; without it 92.98.
        TransformObject* const pTarget = &m_local;

        StoreShadow(pTarget, pEntry->m_vRest.x, pEntry->m_vRest.y, pEntry->m_vRest.z);

        pTarget->ApplyTranslation();

        m_apChildren = nullptr;
    }

    return nResult != -1;
}

// A node with no block leaves the entry null and the reads go through it.
// FUNCTION: HYPERX 0x0046fc50
int MeshBone::BindToLODMeshShadowless(CD3DTextureManager* pLookup, LODNode* pNode, class EngineObject*, short nSlot)
{
    const MeshBoneDesc* pEntry;

    m_nFlags = 0;

    const int nResult = pLookup->LookupMeshForSlot(pNode, 0, nSlot, &m_pMesh);

    if (nResult != -1)
    {
        if (nResult == 0)
        {
            SetFlags(MESHBONE_UNBOUND);
        }

        if (pNode->m_pRecord != nullptr)
        {
            pEntry = pNode->m_pRecord->SlotDesc(nSlot);
        }
        else
        {
            pEntry = nullptr;
        }

        m_pDesc     = pEntry;
        m_pChildIds = pEntry->m_anChildIds;

        // Slot eight of the transform's table is SetTranslation.
        m_local.SetTranslation(pEntry->m_vRest.x,
                               pEntry->m_vRest.y,
                               pEntry->m_vRest.z);

        if (m_apChildren != nullptr)
        {
            delete[] m_apChildren;

            m_apChildren = nullptr;
        }
    }

    return nResult != -1;
}

// The centre is the vertices' mean and the radius the furthest of them.
// FUNCTION: HYPERX 0x0046fcf0
void MeshBone::ComputeBoundingSphere()
{
    Vec3* pVerts;
    Vec3  vSum;
    short i;

    MeshBlock* const pMesh = m_pMesh;

    if (pMesh == nullptr || pMesh->vertexOffset.nBytes == 0)
    {
        pVerts = nullptr;
    }
    else
    {
        pVerts = pMesh->Vertices();
    }

    const short nCount = pMesh->nVertexCount;

    vSum.x = g_flZeroMeshBone;
    vSum.y = g_flZeroMeshBone;
    vSum.z = g_flZeroMeshBone;

    for (i = 0; i < nCount; i++)
    {
        vSum.x += pVerts[i].x;
        vSum.y += pVerts[i].y;
        vSum.z += pVerts[i].z;
    }

    m_flRadius = 0;

    m_vCentre.x = vSum.x / nCount;
    m_vCentre.y = vSum.y / nCount;
    m_vCentre.z = vSum.z / nCount;

    for (i = 0; i < nCount; i++)
    {
        const float flDx = pVerts[i].x - m_vCentre.x;
        const float flDy = pVerts[i].y - m_vCentre.y;
        const float flDz = pVerts[i].z - m_vCentre.z;

        const float flDistance = (float)sqrt(flDx * flDx + flDy * flDy + flDz * flDz);

        if (m_flRadius < flDistance)
        {
            m_flRadius = flDistance;
        }
    }
}
