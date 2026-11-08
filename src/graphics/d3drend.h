#ifndef HYPERBLADE_D3DREND_H
#define HYPERBLADE_D3DREND_H

#include "common/compat.h"

struct IDirect3D;
struct IDirect3DDevice;
struct IDirect3DViewport;
struct FaceRecord;
class MeshBone;
struct MeshFace;
struct MeshMaterial;
struct TexCoord;
struct TransformedVertex;

#ifdef HYPERX_V100
// The render state globals in their address order, thirteen ints and the two fog distances.
struct D3DRenderStates
{
    int   nZEnable;
    int   nTexturePerspective;
    int   nShadeMode;
    int   nTextureFilter;
    int   nTextureMapBlend;
    int   nFillMode;
    int   nCullMode;
    int   nDither;
    int   nSpecular;
    int   nAntiAlias;
    int   nFog;
    int   nFogColour;
    int   nFogTableMode;
    float flFogStart;
    float flFogEnd;
};
#endif

namespace D3DRend
{

void            InitRenderState(void);
void            ResetState(void);
void __cdecl    SetQualityIfEnabled(int nWhich, int nValue);
void            ResetBatchBuffers(int nWhich);
void            InitBatchPools(void);
int             ReleaseAllBuffers(void);
void            SetRenderQuality(int nWhich, int nValue);
int __cdecl     SetupRenderStates(IDirect3DDevice* pDevice, IDirect3DViewport* pViewport, int nTexturePerspective);
int __cdecl     CreateMaterial(void);
void __cdecl    DrawFaceList(MeshBone* pRend, FaceRecord** ppRecords, short nCount);
int             BuildAndSubmitExecuteBuffer(int nVertexCount);
int             FlushBatch(int nWhich);
void __fastcall DrawFace(MeshBone* pRend, unsigned char cFlip, MeshFace* pFaceData, MeshMaterial** ppMaterials, int* pVertexColour, struct TransformedVertex* pVertexBufBase, struct Vec3* pFaceNormal, unsigned char cClipFlags);
int __cdecl     CreateExecuteBuffer(IDirect3DDevice* pDevice, IDirect3D* pUnused);
inline void     CopyUV(TexCoord& rDst, const TexCoord& rSrc);
inline int      UsesHardwareShading();
int             ExecuteScene(int nWhich);

#ifdef HYPERX_V100
void __cdecl GetRenderStates(D3DRenderStates* pStates);
int          CreateWorldMatrix(void);
#endif

} // namespace D3DRend

#endif
