#include "common/compat.h"
#include "d3ddevice.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "batchtexgroup.h"
#include "renderstate.h"
#include "engine/devicector.h"
#include "modeldatabase.h"
#include "meshmaterial.h"
#include "meshbone.h"
#include "d3ddeviceinit.h"
#include "d3dshapes.h"
#include "ui/hudglobals.h"
#include "engine/cameraobject.h"
#include "graphics/texture.h"
#include "common/pathstring.h"
#include "graphics/cd3dtexture.h"
#include "common/staticinit.h"
#include "texcacheentry.h"
#include <stdlib.h>
#include <string.h>
#include <d3d.h>
#include "engine/engineobject.h"
#include "graphics/colorpalette.h"
#include "graphics/d3drend.h"
#include "graphics/face.h"
#include "graphics/d3dmacs.h"
#include "graphics/renderoptions.h"

int g_nD3DVertexPosMask = Face::VertexMask(0, 4);
int g_nD3DVertexUVMask  = Face::VertexMask(7, 2);

#define EXEC_BUFFER_BYTES 0xcb10

namespace D3DRend
{

template <class T>
inline void ReleaseObject(T*& pObject)
{
    if (pObject != 0)
    {
        pObject->Release();
        pObject = 0;
    }
}

} // namespace D3DRend

// FUNCTION: HYPERX 0x004310a0
void D3DRend::ResetState(void)
{
    D3DRend::InitRenderState();
}

// Safe to call twice: every pointer is cleared, except the matrix handles.
// FUNCTION: HYPERX 0x004310b0
int D3DRend::ReleaseAllBuffers(void)
{
    ReleaseObject(g_d3dState.pD3DMaterial);
    ReleaseObject(g_d3dState.pExecuteBuffer);

    if (g_d3dState.dwMatrixHandle2 != 0)
    {
        g_hrD3DResult = g_d3dState.pDevice->DeleteMatrix(g_d3dState.dwMatrixHandle2);
    }

    if (g_d3dState.dwMatrixHandle1 != 0)
    {
        g_hrD3DResult = g_d3dState.pDevice->DeleteMatrix(g_d3dState.dwMatrixHandle1);
    }

    if (g_d3dState.dwMatrixHandle0 != 0)
    {
        g_hrD3DResult = g_d3dState.pDevice->DeleteMatrix(g_d3dState.dwMatrixHandle0);
    }

    ReleaseObject(g_d3dState.apViewports[0]);
    ReleaseObject(g_d3dState.apViewports[1]);
    ReleaseObject(g_d3dState.pDevice);
    ReleaseObject(g_d3dState.pDirect3D);

    if (g_d3dState.pBatchVertexBase != 0)
    {
        free(g_d3dState.pBatchVertexBase);
        g_d3dState.pBatchVertexBase = 0;
    }

    if (g_d3dState.pBatchTriangleBase != 0)
    {
        free(g_d3dState.pBatchTriangleBase);
        g_d3dState.pBatchTriangleBase = 0;
    }

    if (g_d3dState.pBatchTexGroupBase != 0)
    {
        free(g_d3dState.pBatchTexGroupBase);
        g_d3dState.pBatchTexGroupBase = 0;
    }

    g_d3dState.pBatchVertex   = 0;
    g_d3dState.pBatchTriangle = 0;
    g_d3dState.pBatchTexGroup = 0;

    return 0;
}

#define BATCH_VERTEX_CAPACITY   1024
#define BATCH_TRIANGLE_CAPACITY 800

// FUNCTION: HYPERX 0x004316a0
void D3DRend::InitBatchPools(void)
{
    g_d3dState.pBatchVertex     = (D3DBatchVertex*)malloc(BATCH_VERTEX_CAPACITY * sizeof(D3DBatchVertex));
    g_d3dState.pBatchVertexBase = g_d3dState.pBatchVertex;

    g_d3dState.pBatchTriangle     = (D3DTRIANGLE*)malloc(BATCH_TRIANGLE_CAPACITY * sizeof(D3DTRIANGLE));
    g_d3dState.pBatchTriangleBase = g_d3dState.pBatchTriangle;

    g_d3dState.pBatchTexGroupBase = (BatchTexGroup*)malloc(BATCH_TRIANGLE_CAPACITY * sizeof(BatchTexGroup));

    D3DRend::ResetBatchBuffers(1);
}

// FUNCTION: HYPERX 0x00431970
void D3DRend::InitRenderState(void)
{
    memset(&g_d3dState, 0, sizeof(g_d3dState));

    g_d3dState.bOnlyEmulation = g_bEmulationOnly;

    g_nRenderStateZEnable            = FALSE;
    g_nRenderStateTexturePerspective = TRUE;
    g_nRenderStateShadeMode          = D3DSHADE_FLAT;
    g_nRenderStateTextureFilter      = D3DFILTER_NEAREST;
    g_nRenderStateTextureMapBlend    = D3DTBLEND_MODULATE;
    g_nRenderStateFillMode           = D3DFILL_SOLID;
    g_nRenderStateCullMode           = D3DCULL_CCW;
    g_nRenderStateDither             = 0;
    g_nRenderStateSpecular           = 0;
    g_nRenderStateAntiAlias          = 0;
    g_nRenderStateFog                = 0;
    g_nRenderStateFogColour          = 0;
    g_nRenderStateFogTableMode       = 0;
    g_flRenderStateFogStart          = 6.0f;
    g_flRenderStateFogEnd            = 11.0f;

    g_hrD3DResult = 0;
}

#define SETUP_BUFFER_BYTES 0x80

#define STATE_PAIR_COUNT 0x0f

// FUNCTION: HYPERX 0x00431f80
int __cdecl D3DRend::SetupRenderStates(IDirect3DDevice* pDevice, IDirect3DViewport* pViewport, int nTexturePerspective)
{
    D3DEXECUTEBUFFERDESC    desc;
    D3DEXECUTEDATA          data;
    IDirect3DExecuteBuffer* pBuffer;

    memset(&desc, 0, sizeof(desc));

    desc.dwSize       = sizeof(desc);
    desc.dwFlags      = D3DDEB_BUFSIZE;
    desc.dwBufferSize = SETUP_BUFFER_BYTES;

    g_hrD3DResult = g_d3dState.pDevice->CreateExecuteBuffer(&desc, &pBuffer, 0);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    g_hrD3DResult = pBuffer->Lock(&desc);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    memset(desc.lpData, 0, SETUP_BUFFER_BYTES);

    char* p;

    p                  = (char*)desc.lpData;
    char* const pStart = p;

    OP_STATE_RENDER(STATE_PAIR_COUNT, p);

    STATE_DATA(D3DRENDERSTATE_SHADEMODE, g_nRenderStateShadeMode, p);
    STATE_DATA(D3DRENDERSTATE_TEXTUREPERSPECTIVE, nTexturePerspective, p);
    STATE_DATA(D3DRENDERSTATE_ZENABLE, g_nRenderStateZEnable, p);
    STATE_DATA(D3DRENDERSTATE_ZWRITEENABLE, FALSE, p);
    STATE_DATA(D3DRENDERSTATE_ZFUNC, D3DCMP_NEVER, p);
    STATE_DATA(D3DRENDERSTATE_TEXTUREMAG, g_nRenderStateTextureFilter, p);
    STATE_DATA(D3DRENDERSTATE_TEXTUREMIN, g_nRenderStateTextureFilter, p);
    STATE_DATA(D3DRENDERSTATE_TEXTUREMAPBLEND, g_nRenderStateTextureMapBlend, p);
    STATE_DATA(D3DRENDERSTATE_FILLMODE, g_nRenderStateFillMode, p);
    STATE_DATA(D3DRENDERSTATE_DITHERENABLE, g_nRenderStateDither, p);
    STATE_DATA(D3DRENDERSTATE_SPECULARENABLE, g_nRenderStateSpecular, p);
    STATE_DATA(D3DRENDERSTATE_FOGENABLE, g_nRenderStateFog, p);
    STATE_DATA(D3DRENDERSTATE_FOGCOLOR, g_nRenderStateFogColour, p);
    STATE_DATA(D3DRENDERSTATE_ANTIALIAS, g_nRenderStateAntiAlias, p);
    STATE_DATA(D3DRENDERSTATE_CULLMODE, g_nRenderStateCullMode, p);

    OP_EXIT(p);

    g_hrD3DResult = pBuffer->Unlock();

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    memset(&data, 0, sizeof(data));

    data.dwSize              = sizeof(data);
    data.dwInstructionOffset = 0;
    data.dwInstructionLength = p - pStart;

    g_hrD3DResult = pBuffer->SetExecuteData(&data);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    g_hrD3DResult = pDevice->BeginScene();

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    g_hrD3DResult = pDevice->Execute(pBuffer, pViewport, D3DEXECUTE_UNCLIPPED);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    g_hrD3DResult = pDevice->EndScene();

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    g_hrD3DResult = pBuffer->Release();

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    return D3DRend::CreateExecuteBuffer(pDevice, g_d3dState.pDirect3D) != 0;
}

// The diffuse is a flat grey of 80/255 on each channel with alpha nought.

// FUNCTION: HYPERX 0x00432230
int __cdecl D3DRend::CreateMaterial(void)
{
    D3DMATERIAL material;

    g_hrD3DResult = g_d3dState.pDirect3D->CreateMaterial(&g_d3dState.pD3DMaterial, 0);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    memset(&material, 0, sizeof(material));

    material.ambient.r = 0.0f;
    material.ambient.g = 0.0f;
    material.ambient.b = 0.0f;

    material.dwSize = sizeof(material);

    material.diffuse.r = 80.0f / 255.0f;
    material.diffuse.g = 80.0f / 255.0f;
    material.diffuse.b = 80.0f / 255.0f;

    material.dwRampSize = 1;

    g_hrD3DResult = g_d3dState.pD3DMaterial->SetMaterial(&material);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    g_hrD3DResult = g_d3dState.pD3DMaterial->GetHandle(g_d3dState.pDevice,
                                                       &g_d3dState.hBackgroundMaterial);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    g_hrD3DResult = g_d3dState.apViewports[0]->SetBackground(g_d3dState.hBackgroundMaterial);

    return g_hrD3DResult == 0;
}

// FUNCTION: HYPERX 0x00432300
int __cdecl D3DRend::CreateExecuteBuffer(IDirect3DDevice* pDevice, IDirect3D*)
{
    memset(&g_d3dState.execBufferDesc, 0, sizeof(g_d3dState.execBufferDesc));

    g_d3dState.execBufferDesc.dwSize       = sizeof(g_d3dState.execBufferDesc);
    g_d3dState.execBufferDesc.dwFlags      = D3DDEB_BUFSIZE | D3DDEB_CAPS;
    g_d3dState.execBufferDesc.dwCaps       = D3DDEBCAPS_SYSTEMMEMORY;
    g_d3dState.execBufferDesc.dwBufferSize = EXEC_BUFFER_BYTES;

    g_d3dState.nExecuteBufferSize = EXEC_BUFFER_BYTES;

    g_hrD3DResult = pDevice->CreateExecuteBuffer(&g_d3dState.execBufferDesc, &g_d3dState.pExecuteBuffer, 0);

    return g_hrD3DResult == 0;
}

// The second material record is filled only when the first names a back material.
// FUNCTION: HYPERX 0x004323a0
void __cdecl D3DRend::DrawFaceList(MeshBone* pRend, FaceRecord** ppRecords, short nCount)
{
    EngineObject* const pRendOwner = pRend->m_pOwner;
    int* const          pColours   = pRend->m_pFaceColours;

    const int nState = pRend->HasColours() | ~1;

    MeshMaterial* apMaterials[2];
    int*          pColour;

    for (; nCount-- > 0;)
    {
        FaceRecord* const pRecord = *ppRecords;

        MeshFace* pFace = pRend->m_pMesh->Face(pRecord->nFace);

        apMaterials[0] = &pRendOwner->m_pModelBlock->m_pMaterials[pFace->cMaterial];

        if (apMaterials[0]->nBackIndex >= 0)
        {
            apMaterials[1] = &pRendOwner->m_pModelBlock->m_pMaterials[apMaterials[0]->nBackIndex];
        }

        if (pRecord->cFlip == 0 && nState == -1)
        {
            pColour = pColours + pRecord->nFace;
        }
        else
        {
            pColour = 0;
        }

        D3DRend::DrawFace(pRend, pRecord->cFlip, pFace, apMaterials, pColour, g_pModelVertexBuf, &pFace->vNormal, pRecord->cClip);

        ppRecords--;
    }
}

// FUNCTION: HYPERX 0x00432490
int D3DRend::BuildAndSubmitExecuteBuffer(int nVertexCount)
{
    D3DEXECUTEBUFFERDESC desc = g_d3dState.execBufferDesc;

    g_hrD3DResult = g_d3dState.pExecuteBuffer->Lock(&desc);

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    char* const pLocked = (char*)desc.lpData;
    char*       pWrite  = pLocked;

    if (pLocked != (char*)g_d3dState.pBatchVertexBase)
    {
        memcpy(pLocked, g_d3dState.pBatchVertexBase, nVertexCount * sizeof(D3DBatchVertex));
    }

    pWrite += nVertexCount * sizeof(D3DBatchVertex);
    int         nWritten = 0;
    char* const pStream  = pWrite;

    OP_PROCESS_VERTICES(1, pWrite);
    PROCESSVERTICES_DATA(D3DPROCESSVERTICES_COPY | D3DPROCESSVERTICES_UPDATEEXTENTS, 0, nVertexCount, pWrite);

    if (g_d3dState.bD3DTexturesEnabled == 0 || g_cMdmTexLoaded == 0)
    {
        OP_STATE_RENDER(1, pWrite);
        STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, 0, pWrite);

        if (QWORD_ALIGNED(pWrite))
        {
            OP_TRIANGLE_LIST(0, pWrite);
        }

        OP_TRIANGLE_LIST((short)g_d3dState.nBatchTriangleCount, pWrite);

        g_d3dState.nBatchInstructionOffset = pWrite - pLocked;

        if (pWrite != (char*)g_d3dState.pBatchTriangleBase)
        {
            memcpy(pWrite, g_d3dState.pBatchTriangleBase, g_d3dState.nBatchTriangleCount * sizeof(D3DTRIANGLE));
        }

        pWrite += g_d3dState.nBatchTriangleCount * sizeof(D3DTRIANGLE);
    }
    else
    {
        g_d3dState.pBatchTexGroup = g_d3dState.pBatchTexGroupBase;
        g_d3dState.pBatchTriangle = g_d3dState.pBatchTriangleBase;

        while (nWritten < g_d3dState.nBatchTriangleCount)
        {
            OP_STATE_RENDER(1, pWrite);
            STATE_DATA(D3DRENDERSTATE_TEXTUREHANDLE, g_d3dState.pBatchTexGroup->dwTextureHandle, pWrite);

            if (QWORD_ALIGNED(pWrite))
            {
                OP_TRIANGLE_LIST(0, pWrite);
            }

            OP_TRIANGLE_LIST(g_d3dState.pBatchTexGroup->wTriangleCount, pWrite);

            if (pWrite != (char*)g_d3dState.pBatchTriangle)
            {
                memcpy(pWrite, g_d3dState.pBatchTriangle, g_d3dState.pBatchTexGroup->wTriangleCount * sizeof(D3DTRIANGLE));
            }

            pWrite += g_d3dState.pBatchTexGroup->wTriangleCount * sizeof(D3DTRIANGLE);
            g_d3dState.pBatchTriangle += g_d3dState.pBatchTexGroup->wTriangleCount;
            nWritten += g_d3dState.pBatchTexGroup->wTriangleCount;
            g_d3dState.pBatchTexGroup++;
        }
    }

    OP_EXIT(pWrite);

    g_hrD3DResult = g_d3dState.pExecuteBuffer->Unlock();

    if (g_hrD3DResult != 0)
    {
        return 0;
    }

    memset(&g_d3dState.d3dExecuteData, 0, sizeof(g_d3dState.d3dExecuteData));

    g_d3dState.d3dExecuteData.dwSize              = sizeof(g_d3dState.d3dExecuteData);
    g_d3dState.d3dExecuteData.dwVertexCount       = nVertexCount;
    g_d3dState.d3dExecuteData.dwInstructionOffset = pStream - pLocked;
    g_d3dState.d3dExecuteData.dwInstructionLength = pWrite - pStream;

    g_hrD3DResult = g_d3dState.pExecuteBuffer->SetExecuteData(&g_d3dState.d3dExecuteData);

    return g_hrD3DResult == 0;
}

// The argument is never read.

// FUNCTION: HYPERX 0x00432730
void D3DRend::ResetBatchBuffers(int)
{
    D3DTRIANGLE* const pBatchTriangle = g_d3dState.pBatchTriangleBase;

    g_d3dState.nBatchTriangleCount = 0;
    g_d3dState.nBatchVertexCount   = 0;

    g_d3dState.pBatchVertex             = g_d3dState.pBatchVertexBase;
    BatchTexGroup* const pBatchTexGroup = g_d3dState.pBatchTexGroupBase;
    g_d3dState.pBatchTriangle           = pBatchTriangle;
    g_d3dState.pBatchTexGroup           = pBatchTexGroup;

    memset(pBatchTexGroup, 0, sizeof(BatchTexGroup));

    g_d3dState.nBatchTexHandle = -1;

    g_d3dState.pBatchTexGroup -= 1;
}

// An empty batch reports success.
// FUNCTION: HYPERX 0x00432780
int D3DRend::FlushBatch(int nWhich)
{
    if (g_d3dState.nBatchVertexCount != 0 && g_d3dState.nBatchTriangleCount != 0)
    {
        const int nBuilt = D3DRend::BuildAndSubmitExecuteBuffer(g_d3dState.nBatchVertexCount);

        if (nBuilt == 0)
        {
            return nBuilt;
        }

        g_hrD3DResult = D3DRend::ExecuteScene(nWhich);
    }

    return 1;
}

#pragma intrinsic(memcpy)

inline void D3DRend::CopyUV(TexCoord& rDst, const TexCoord& rSrc)
{
    memcpy(&rDst, &rSrc, sizeof(TexCoord));
}

#define BATCH_VERTEX_LIMIT 0x3f6

#define D3D_HARDWARE_SHADING 1

// FUNCTION: HYPERX 0x004327c0
int D3DRend::ExecuteScene(int nWhich)
{
    const int nViewport = g_pScene->nViewportId;

    if (g_bExecuteSuppressed == 0)
    {
        g_hrD3DResult = g_d3dState.pDevice->Execute(g_d3dState.pExecuteBuffer, g_d3dState.apViewports[nViewport], D3DEXECUTE_UNCLIPPED);
    }

    D3DRend::ResetBatchBuffers(nWhich);

    return g_hrD3DResult;
}

// Projects one batch vertex onto the viewport and appends it to the vertex batch.
#define D3D_APPEND_VERTEX(vert)                                                           \
    {                                                                                     \
        (vert).pos.w = 1.0f / (vert).pos.w;                                               \
        (vert).pos.x *= (vert).pos.w;                                                     \
        (vert).pos.y *= (vert).pos.w;                                                     \
                                                                                          \
        if ((vert).pos.x < 0.0f)                                                          \
        {                                                                                 \
            (vert).pos.x = 0.0f;                                                          \
        }                                                                                 \
                                                                                          \
        if ((vert).pos.y < 0.0f)                                                          \
        {                                                                                 \
            (vert).pos.y = 0.0f;                                                          \
        }                                                                                 \
                                                                                          \
        const int nViewport = g_pScene->nViewportId;                                      \
                                                                                          \
        if (nViewport > 0)                                                                \
        {                                                                                 \
            (vert).pos.x = (float)g_d3dState.aViewportRects[nViewport].x1 + (vert).pos.x; \
            (vert).pos.y = (float)g_d3dState.aViewportRects[nViewport].y1 + (vert).pos.y; \
        }                                                                                 \
                                                                                          \
        (vert).pos.x = (float)g_nViewportOriginX + (vert).pos.x;                          \
        (vert).pos.y = (float)g_nViewportOriginY + (vert).pos.y;                          \
                                                                                          \
        *g_d3dState.pBatchVertex = (vert);                                                \
        g_d3dState.pBatchVertex++;                                                        \
    }

// Copies the fan's current staged vertex into one batch vertex, then appends it.
#define D3D_APPEND_FAN_VERTEX(vert)                                           \
    {                                                                         \
        const ClipVertex* const pRow = &g_d3dFaceBuffer.aVertices[nWalkVert]; \
                                                                              \
        (vert).pos = pRow->pos;                                               \
                                                                              \
        if (bTextured != 0)                                                   \
        {                                                                     \
            CopyUV((vert).uv, pRow->uv);                                      \
        }                                                                     \
                                                                              \
        D3D_APPEND_VERTEX(vert)                                               \
    }

#pragma optimize("", off)

// FUNCTION: HYPERX 0x004328c0
void __fastcall D3DRend::DrawFace(MeshBone* pRend, unsigned char cFlip, MeshFace* pFaceData, MeshMaterial** ppMaterials, int* pVertexColour, struct TransformedVertex* pVertexBufBase, struct Vec3* pFaceNormal, unsigned char cClipFlags)
{
    short nPosVert;
    short nTexVertex;
    // A second name for the table, which the original's frame keeps.
    const TexCoord* pUVBase;
    CD3DTexture*    pTex;
    short*          pCornerUV;
    short           nFanStep;
    int             nWalkVert;
    Vec3            vFaceNormal;
    int             bTextured;
    short           nFanIndex;
    short*          pCorner;
    D3DTRIANGLE     triangle;
    D3DBatchVertex  aVert[3];

    pCorner                = pFaceData->aCorners;
    g_d3dFaceBuffer.nCount = pFaceData->nCorners;
    g_d3dFaceBuffer.nMask  = g_nD3DVertexPosMask;
    bTextured              = 0;
    g_dwCurrentTexHandle   = 0;

    if (g_d3dState.bD3DTexturesEnabled != 0 && g_cMdmTexLoaded != 0 && g_nMdmTexturesReady != 0 && ppMaterials[0]->pTexture != 0)
    {
        if ((pFaceData->cFlags & FACE_FLAG_HAS_UV) != 0)
        {
            pCornerUV = &pFaceData->aCorners[pFaceData->nCorners];

            if ((pFaceData->cFlags & FACE_FLAG_UV_SKIP) != 0)
            {
                pCornerUV += pFaceData->nCorners;
            }
        }
        else
        {
            pCornerUV = 0;
        }

        if (pCornerUV != 0)
        {
            pTex = ppMaterials[0]->pTexture->m_pD3DTexture;

            if (pTex != 0)
            {
                const TexCoord*  pMeshUVs;
                const MeshBlock* pBlock;
                unsigned int     dwTexHandle;

                dwTexHandle          = pTex->GetTextureHandle();
                g_dwCurrentTexHandle = dwTexHandle;
                bTextured            = 1;
                g_d3dFaceBuffer.nMask |= g_nD3DVertexUVMask;

                pBlock = pRend->m_pMesh;

                if (pBlock->nCoordBytes != 0)
                {
                    pMeshUVs = (const TexCoord*)(&pBlock->Vertices()[pBlock->nVertexCount] + pBlock->nNormalCount);
                }
                else
                {
                    pMeshUVs = 0;
                }

                pUVBase = pMeshUVs;

                for (nTexVertex = 0; nTexVertex < pFaceData->nCorners; nTexVertex++)
                {
                    memcpy(&g_d3dFaceBuffer.aVertices[nTexVertex].pos,
                           &pVertexBufBase[*pCorner++],
                           sizeof(TransformedVertex));
                    memcpy(&g_d3dFaceBuffer.aVertices[nTexVertex].uv,
                           &pUVBase[*pCornerUV++],
                           sizeof(TexCoord));
                }
            }
        }
    }

    if (bTextured == 0)
    {
        for (nPosVert = 0; nPosVert < pFaceData->nCorners; nPosVert++)
        {
            memcpy(&g_d3dFaceBuffer.aVertices[nPosVert].pos,
                   &pVertexBufBase[*pCorner++],
                   sizeof(TransformedVertex));
        }
    }

    if (cClipFlags != 0 && Face::ClipPolygon(&g_d3dFaceBuffer, cClipFlags) != 0)
    {
        return;
    }

    if ((cClipFlags & CLIP_NEAR) != 0)
    {
        char        bBackFacing;
        ClipVertex* pStageC;
        ClipVertex* pStageB;
        void*       pCamera;
        Vec3        vCornerA;
        Vec3        vPerp;
        float       flWInv;
        Vec3        vAToC;
        Vec3        vCornerB;
        Vec3        vAToB;
        Vec3        vCornerC;

        pStageC = &g_d3dFaceBuffer.aVertices[2];
        pStageB = &g_d3dFaceBuffer.aVertices[1];
        pCamera = g_pCameraTransform;

        flWInv     = 1.0f / g_d3dFaceBuffer.aVertices[0].pos.w;
        vCornerA.x = g_d3dFaceBuffer.aVertices[0].pos.x * flWInv;
        vCornerA.y = g_d3dFaceBuffer.aVertices[0].pos.y * flWInv;
        vCornerA.z = g_d3dFaceBuffer.aVertices[0].pos.z * flWInv;

        flWInv     = 1.0f / pStageB->pos.w;
        vCornerB.x = flWInv * pStageB->pos.x;
        vCornerB.y = flWInv * pStageB->pos.y;
        vCornerB.z = flWInv * pStageB->pos.z;

        flWInv     = 1.0f / pStageC->pos.w;
        vCornerC.x = flWInv * pStageC->pos.x;
        vCornerC.y = flWInv * pStageC->pos.y;
        vCornerC.z = flWInv * pStageC->pos.z;

        vAToB.x = vCornerB.x - vCornerA.x;
        vAToB.y = vCornerB.y - vCornerA.y;
        vAToB.z = vCornerB.z - vCornerA.z;

        vAToC.x = vCornerC.x - vCornerA.x;
        vAToC.y = vCornerC.y - vCornerA.y;
        vAToC.z = vCornerC.z - vCornerA.z;

        vPerp.x = vAToC.y * vAToB.z - vAToC.z * vAToB.y;
        vPerp.y = vAToC.z * vAToB.x - vAToC.x * vAToB.z;
        if ((vPerp.z = vAToC.x * vAToB.y - vAToC.y * vAToB.x) <= 0.0)
        {
            bBackFacing = 1;
        }
        else
        {
            bBackFacing = 0;
        }

        if (((unsigned char)bBackFacing) != 0)
        {
            if (ppMaterials[0]->nBackIndex < 0)
            {
                return;
            }

            cFlip = 1;
        }
    }

    MeshMaterial matUsed;

    if (cFlip != 0)
    {
        vFaceNormal.x = -pFaceNormal->x;
        vFaceNormal.y = -pFaceNormal->y;
        vFaceNormal.z = -pFaceNormal->z;

        matUsed = *ppMaterials[1];

        nFanStep  = 1;
        nWalkVert = 1;
    }
    else
    {
        vFaceNormal = *pFaceNormal;

        matUsed = *ppMaterials[0];

        nFanStep  = -1;
        nWalkVert = g_d3dFaceBuffer.nCount - 1;
    }

    if (g_d3dState.bD3DTexturesEnabled != 0 && g_cMdmTexLoaded != 0)
    {
        if (g_dwCurrentTexHandle != (unsigned int)g_d3dState.nBatchTexHandle)
        {
            g_d3dState.nBatchTexHandle = (int)g_dwCurrentTexHandle;
            g_d3dState.pBatchTexGroup++;
            g_d3dState.pBatchTexGroup->dwTextureHandle = (unsigned int)g_d3dState.nBatchTexHandle;
            g_d3dState.pBatchTexGroup->wTriangleCount  = (short)(g_d3dFaceBuffer.nCount - 2);
        }
        else
        {
            g_d3dState.pBatchTexGroup->wTriangleCount += (short)(g_d3dFaceBuffer.nCount - 2);
        }
    }

    memset(aVert, 0, sizeof(aVert));

    aVert[0].pos.x = g_d3dFaceBuffer.aVertices[0].pos.x;
    aVert[0].pos.y = g_d3dFaceBuffer.aVertices[0].pos.y;
    aVert[0].pos.z = g_d3dFaceBuffer.aVertices[0].pos.z;
    aVert[0].pos.w = g_d3dFaceBuffer.aVertices[0].pos.w;

    if (bTextured != 0)
    {
        CopyUV(aVert[0].uv, g_d3dFaceBuffer.aVertices[0].uv);
    }

    if (pVertexColour == 0)
    {
        unsigned int  dwPaletteColour;
        int           nPalIndex;
        unsigned char cLit;
        float         flLighting;
        float         flPaletteLevel;
        unsigned char cPaletteShade;
        unsigned char cPalIndex;

        // The palette path is the original's dead code.
        if (!D3D_HARDWARE_SHADING)
        {
            cPalIndex = (unsigned char)(matUsed.nColour & 0xff);
            nPalIndex = cPalIndex;

            if ((nPalIndex < g_pColorPalette->m_rampBounds.nFirst || nPalIndex > g_pColorPalette->m_rampBounds.nCount) != 0)
            {
                dwPaletteColour = matUsed.nColour;
            }
            else
            {
                flPaletteLevel = g_flZeroLightLevel;
                cPaletteShade  = (unsigned char)MeshMaterial::CalcLightIntensity(&flPaletteLevel,
                                                                                 &matUsed,
                                                                                 &vFaceNormal);

                dwPaletteColour =
                    ((unsigned int)ColorPalette::LookupColorIntensity(g_pColorPalette,
                                                                      cPaletteShade,
                                                                      flPaletteLevel,
                                                                      cPalIndex)
                     << 0x18) |
                    ((unsigned int)ColorPalette::LookupColorIntensity(g_pColorPalette,
                                                                      cPaletteShade,
                                                                      flPaletteLevel,
                                                                      cPalIndex)
                     << 0x10) |
                    ((unsigned int)ColorPalette::LookupColorIntensity(g_pColorPalette,
                                                                      cPaletteShade,
                                                                      flPaletteLevel,
                                                                      cPalIndex)
                     << 8) |
                    (unsigned int)ColorPalette::LookupColorIntensity(g_pColorPalette,
                                                                     cPaletteShade,
                                                                     flPaletteLevel,
                                                                     cPalIndex);
            }
        }
        else
        {
            flLighting = g_flZeroLightLevel;

            switch ((unsigned char)MeshMaterial::CalcLightIntensity(&flLighting, &matUsed, &vFaceNormal))
            {
                case 1:
                    if (bTextured != 0)
                    {
                        dwPaletteColour = 0xffffffff;

                        break;
                    }

                    dwPaletteColour = matUsed.nColour;

                    break;

                case 2:
                    if (bTextured != 0)
                    {
                        cLit = (unsigned char)(int)(255.0f * flLighting);

                        dwPaletteColour = (unsigned int)cLit << 0x18 | (unsigned int)cLit << 0x10 | (unsigned int)cLit << 8 | (unsigned int)cLit;

                        break;
                    }

                    dwPaletteColour = ((unsigned int)((int)((float)matUsed.GetRed() * flLighting) << 0x10) | 0xff000000) | (unsigned int)((int)((float)matUsed.GetGreen() * flLighting) << 8) | (unsigned int)(int)((float)matUsed.GetBlue() * flLighting);

                    break;

                default:
                    dwPaletteColour = 0xff000000;

                    break;
            }
        }

        aVert[0].dwColour = dwPaletteColour;
    }
    else
    {
        aVert[0].dwColour = (unsigned int)*pVertexColour;
    }

    D3D_APPEND_VERTEX(aVert[0])

    triangle.wV1 = (unsigned short)g_d3dState.nBatchVertexCount;
    g_d3dState.nBatchVertexCount++;

    if (g_d3dFaceBuffer.nCount > 3)
    {
        triangle.wFlags = (unsigned short)(D3DTRIFLAG_STARTFLAT(g_d3dFaceBuffer.nCount - 2) | D3DTRIFLAG_EDGEENABLETRIANGLE);
    }
    else
    {
        triangle.wFlags = D3DTRIFLAG_START | D3DTRIFLAG_EDGEENABLETRIANGLE;
    }

    D3D_APPEND_FAN_VERTEX(aVert[1])

    for (nFanIndex = 2; nFanIndex < g_d3dFaceBuffer.nCount; nFanIndex++)
    {
        nWalkVert += nFanStep;

        D3D_APPEND_FAN_VERTEX(aVert[2])

        triangle.wV2 = (unsigned short)g_d3dState.nBatchVertexCount;
        g_d3dState.nBatchVertexCount++;
        triangle.wV3 = (unsigned short)g_d3dState.nBatchVertexCount;

        *g_d3dState.pBatchTriangle = triangle;
        g_d3dState.pBatchTriangle++;

        g_d3dState.nBatchTriangleCount++;

        triangle.wFlags = D3DTRIFLAG_EVEN;
    }

    g_d3dState.nBatchVertexCount++;

    if (g_d3dState.nBatchVertexCount > BATCH_VERTEX_LIMIT)
    {
        D3DRend::FlushBatch(0);
    }
}

#pragma optimize("", on)

// FUNCTION: HYPERX 0x004337d0
void D3DRend::SetRenderQuality(int nWhich, int nValue)
{
    if (g_ddrawDevice.bD3DActive != 0)
    {
        switch (nWhich)
        {
            case RENDER_OPTION_PERSPECTIVE:
                g_nRenderStateTexturePerspective = nValue;
                break;

            case RENDER_OPTION_FILTERED:
                if (nValue != 0)
                {
                    g_nRenderStateTextureFilter = D3DFILTER_LINEAR;
                }
                else
                {
                    g_nRenderStateTextureFilter = D3DFILTER_NEAREST;
                }

                break;
        }

        D3DRend::SetupRenderStates(g_d3dState.pDevice, g_d3dState.apViewports[0], g_nRenderStateTexturePerspective);
    }
}

// FUNCTION: HYPERX 0x0044bee0
void __cdecl D3DRend::SetQualityIfEnabled(int nWhich, int nValue)
{
    if (g_cUseSoftRenderer == 0)
    {
        D3DRend::SetRenderQuality(nWhich, nValue);
    }
}
