#include "math/floatbits.h"
#include "common/compat.h"
#include "globals.h"
#include "graphicsglobals.h"
#include "engine/devicector.h"
#include "modeldatabase.h"
#include "meshmaterial.h"
#include "meshbone.h"
#include "engine/engineobject.h"
#include "app/bringup.h"
#include "ui/hudglobals.h"
#include "projrender.h"
#include "d3dshapes.h"
#include "softtex.h"
#include "texcacheentry.h"
#include "colorpalette.h"
#include "common/staticinit.h"
#include "rlesprite.h"
#include "graphics/face.h"
#include "graphics/swrend.h"

float        g_flUnusedSWRendA  = g_flZeroSWRend;
float        g_flUnusedSWRendB  = g_flZeroSWRend;
float        g_flUnusedSWRendC  = g_flZeroSWRend;
int          g_nSWVertexPosMask = Face::VertexMask(0, 4);
int          g_nSWVertexUVMask  = Face::VertexMask(7, 2);
float        g_flUnusedDisplayA = g_flZeroDisplay;
float        g_flUnusedDisplayB = g_flZeroDisplay;
float        g_flUnusedDisplayC = g_flZeroDisplay;
SurfacePoint g_ptHudClockAnchor(0, 0);
SurfacePoint g_ptHudAnchorC(0, 0);
SurfacePoint g_ptHudAnchorD(0, 0);

extern Vec3 g_vLightDirModel;

// The addresses are named in symbols/float_constants.csv.
#define SW_LIGHT_CEILING 1.0f

#define COLOUR_ALPHA_BITS 0xffffff00

#define COLOUR_OPAQUE 0xff000000

#define SW_ROUND_BIAS 12582912.0f

// Screen coordinates are rounded by adding a large constant and reading the float's low word.
// FUNCTION: HYPERX 0x0044a200
void __fastcall SWRend::DrawFace(MeshBone* pRend, char cFlip, MeshFace* pFaceData, MeshMaterial** ppMaterials, int* pVertexColour, struct TransformedVertex* pVertexBufBase, struct Vec3* pFaceNormal, char cClipFlags)
{
    const TexCoord*     pUVTable;
    TexCacheEntry*      pMatTexture;
    const MeshMaterial* pMaterial;
    ClipVertex*         pVert;
    float               flLevel;
    float               flLevel2;
    unsigned char       cShade;
    unsigned char       cColourIndex;
    int                 nKind;
    short               nStep;
    short               nFirst;
    short               nEnd;
    short               i;

    g_swFaceBuffer.nCount = pFaceData->nCorners;
    SoftTex* pTexture     = 0;
    short*   pIndex       = pFaceData->aCorners;
    g_swFaceBuffer.nMask  = g_nSWVertexPosMask;

    // Only an MDM-textured face that carries UV indices takes texture coordinates.
    short* pUVIndex = g_cMdmTexLoaded != 0 && (pMatTexture = ppMaterials[0]->pTexture) != 0 && (pFaceData->cFlags & FACE_FLAG_HAS_UV) != 0 ? pFaceData->UVIndices() : 0;

    if (pUVIndex != 0)
    {
        const MeshBlock* pBlob = pRend->m_pMesh;

        if (pBlob->nCoordBytes != 0)
        {
            pUVTable = pBlob->Coords();
        }
        else
        {
            pUVTable = 0;
        }

        pTexture             = pMatTexture->m_pSoftTexture;
        g_swFaceBuffer.nMask = g_nSWVertexUVMask | g_nSWVertexPosMask;

        for (i = 0; i < pFaceData->nCorners; i++)
        {
            const TexCoord* pUV;

            const TransformedVertex* const pSource = &pVertexBufBase[*pIndex++];

            pVert = &g_swFaceBuffer.aVertices[i];

            pVert->pos = *pSource;

            pUV = &pUVTable[*pUVIndex++];

            pVert->uv = *pUV;
        }
    }
    else
    {
        for (i = 0; i < pFaceData->nCorners; i++)
        {
            pVert = &g_swFaceBuffer.aVertices[i];

            pVert->pos = pVertexBufBase[*pIndex++];
        }
    }

    if (cClipFlags != 0 && Face::ClipPolygon(&g_swFaceBuffer, cClipFlags) != 0)
    {
        return;
    }

#ifdef HYPERX_BRINGUP
    // A clipped face can come back degenerate, and its projection divides by zero.
    if (BringUp::FaceUnprojectable() != 0)
    {
        return;
    }
#endif

    char cBackFace = cFlip;

    if ((cClipFlags & CLIP_NEAR) != 0)
    {
        Vec3 vCorner0;
        Vec3 vCorner1;
        Vec3 vCorner2;

        vCorner0.x           = (1.0f / g_swFaceBuffer.aVertices[0].pos.w) * g_swFaceBuffer.aVertices[0].pos.x;
        vCorner0.y           = (1.0f / g_swFaceBuffer.aVertices[0].pos.w) * g_swFaceBuffer.aVertices[0].pos.y;
        const float flScale1 = 1.0f / g_swFaceBuffer.aVertices[1].pos.w;
        vCorner1.x           = flScale1 * g_swFaceBuffer.aVertices[1].pos.x;
        vCorner1.y           = flScale1 * g_swFaceBuffer.aVertices[1].pos.y;
        const float flScale2 = 1.0f / g_swFaceBuffer.aVertices[2].pos.w;
        vCorner2.x           = flScale2 * g_swFaceBuffer.aVertices[2].pos.x;
        vCorner2.y           = flScale2 * g_swFaceBuffer.aVertices[2].pos.y;

        vCorner1.x -= vCorner0.x;
        vCorner1.y -= vCorner0.y;
        vCorner0.x = vCorner2.x - vCorner0.x;
        vCorner0.y = vCorner2.y - vCorner0.y;

        if (vCorner0.x * vCorner1.y - vCorner0.y * vCorner1.x <= 0.0)
        {
            if (ppMaterials[0]->nBackIndex < 0)
            {
                return;
            }

            cBackFace = 1;
        }
    }

    if (g_cMdmTexLoaded != 0 && pTexture != 0 && ppMaterials[0]->pTexture != 0 && cBackFace == 0)
    {
        pTexture->DrawTexPoly(g_pCameraTransform, &g_swFaceBuffer, g_flZeroSWRend);

        return;
    }

    unsigned int dwColour;

    if (cBackFace != 0)
    {
        pMaterial = ppMaterials[1];

        Vec3 vNormal;

        vNormal.x = -pFaceNormal->x;
        vNormal.y = -pFaceNormal->y;
        vNormal.z = -pFaceNormal->z;

        if (g_cD3DActive == 0)
        {
            cColourIndex = pMaterial->acChannel[0];

            if (cColourIndex >= g_pColorPalette->m_rampBounds.nFirst && cColourIndex <= g_pColorPalette->m_rampBounds.nCount)
            {
                flLevel             = g_flZeroSWRend;
                Vec3* const pNormal = &vNormal;

                if (pNormal == 0)
                {
                    cShade = SHADE_WHITE;
                }
                else
                {
                    const float flPartial = Vec3::Dot(pNormal, &g_vLightDirModel) * pMaterial->flDiffuse + pMaterial->flAmbient;

                    flLevel = flPartial + pMaterial->flEmissive;

                    if (flLevel > g_flZeroSWRend)
                    {
                        if (flLevel < SW_LIGHT_CEILING)
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

                dwColour =
                    (((ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel, cColourIndex)
                           << 8 |
                       ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel, cColourIndex))
                          << 8 |
                      ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel, cColourIndex))
                     << 8) |
                    ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel, cColourIndex);
            }
            else
            {
                dwColour = (unsigned int)pMaterial->nColour;
            }
        }
        else
        {
            flLevel = g_flZeroSWRend;

            nKind = MeshMaterial::CalcLightIntensity(&flLevel, pMaterial, &vNormal) & 0xff;

            switch (nKind)
            {
                case SHADE_WHITE:
                    dwColour = (unsigned int)pMaterial->nColour;
                    break;

                case SHADE_SCALED:
                    dwColour =
                        ((((unsigned int)(int)(pMaterial->acChannel[2] * flLevel) | COLOUR_ALPHA_BITS)
                              << 8 |
                          (unsigned int)(int)(pMaterial->acChannel[1] * flLevel))
                         << 8) |
                        (unsigned int)(int)(pMaterial->acChannel[0] * flLevel);

                    break;

                default:
                    dwColour = COLOUR_OPAQUE;
                    break;
            }
        }
    }
    else
    {
        if (pVertexColour != 0)
        {
            dwColour = (unsigned int)*pVertexColour;
        }
        else
        {
            pMaterial = ppMaterials[0];

            if (g_cD3DActive == 0)
            {
                cColourIndex = pMaterial->acChannel[0];

                if (cColourIndex >= g_pColorPalette->m_rampBounds.nFirst && cColourIndex <= g_pColorPalette->m_rampBounds.nCount)
                {
                    flLevel2 = g_flZeroSWRend;

                    cShade = (unsigned char)MeshMaterial::CalcLightIntensity(&flLevel2,
                                                                             pMaterial,
                                                                             pFaceNormal);

                    dwColour =
                        (((ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel2, cColourIndex)
                               << 8 |
                           ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel2, cColourIndex))
                              << 8 |
                          ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel2, cColourIndex))
                         << 8) |
                        ColorPalette::LookupColorIntensity(g_pColorPalette, cShade, flLevel2, cColourIndex);
                }
                else
                {
                    dwColour = (unsigned int)pMaterial->nColour;
                }
            }
            else
            {
                flLevel2 = g_flZeroSWRend;

                nKind = MeshMaterial::CalcLightIntensity(&flLevel2, pMaterial, pFaceNormal) & 0xff;

                switch (nKind)
                {
                    case SHADE_WHITE:
                        dwColour = (unsigned int)pMaterial->nColour;
                        break;

                    case SHADE_SCALED:
                        dwColour =
                            ((((unsigned int)(int)(pMaterial->acChannel[2] * flLevel2) | COLOUR_ALPHA_BITS)
                                  << 8 |
                              (unsigned int)(int)(pMaterial->acChannel[1] * flLevel2))
                             << 8) |
                            (unsigned int)(int)(pMaterial->acChannel[0] * flLevel2);

                        break;

                    default:
                        dwColour = COLOUR_OPAQUE;
                        break;
                }
            }
        }
    }

    g_nSWFaceColour = dwColour;

    i          = 0;
    int nIndex = 0;

    if ((g_bBottomUpDIB != 0 && cBackFace != 0) || (g_bBottomUpDIB == 0 && cBackFace == 0))
    {
        nFirst = nIndex;
        nStep  = 1;
        nEnd   = g_swFaceBuffer.nCount;
    }
    else
    {
        nFirst = g_swFaceBuffer.nCount - 1;
        nStep  = -1;
        nEnd   = -1;
    }

    g_wSpanYMax      = (short)g_nViewportOriginY;
    g_bRightEdgeUsed = 0;
    g_wSpanYMin      = (short)(g_nPrevHeight + g_nViewportOriginY - 1);
    g_bLeftEdgeUsed  = 0;

    nIndex = nFirst;

    float flRecip = SW_LIGHT_CEILING / g_swFaceBuffer.aVertices[(short)nIndex].pos.w;
    pVert         = &g_swFaceBuffer.aVertices[(short)nIndex];
    pVert->pos.x  = flRecip * pVert->pos.x;
    pVert->pos.y  = flRecip * pVert->pos.y;

    for (i = 0; i < g_swFaceBuffer.nCount; i++)
    {
        const float flPrevY = pVert->pos.y;
        const float flPrevX = pVert->pos.x;

        nIndex += nStep;

        if ((short)nIndex == nEnd)
        {
            pVert = &g_swFaceBuffer.aVertices[nFirst];
        }
        else
        {
            flRecip      = SW_LIGHT_CEILING / g_swFaceBuffer.aVertices[(short)nIndex].pos.w;
            pVert        = &g_swFaceBuffer.aVertices[(short)nIndex];
            pVert->pos.x = flRecip * pVert->pos.x;
            pVert->pos.y = flRecip * pVert->pos.y;
        }

        // The rounding adds the bias constant, and the result is the sum's own low bits.
        const float flRoundX1 = flPrevX + SW_ROUND_BIAS;
        const float flRoundY1 = flPrevY + SW_ROUND_BIAS;
        const float flRoundX2 = pVert->pos.x + SW_ROUND_BIAS;
        const float flRoundY2 = pVert->pos.y + SW_ROUND_BIAS;

#ifdef HYPERX_BRINGUP
        // An edge outside the frame's rows would scan past the edge tables.
        if (BringUp::EdgeOffScreen(flRoundY1, flRoundY2) != 0)
        {
            continue;
        }
#endif

        SWRend::ScanEdge((unsigned short)((int)FloatBits(flRoundX1) + g_nViewportOriginX),
                         (unsigned short)((int)FloatBits(flRoundY1) + g_nViewportOriginY),
                         (unsigned short)((int)FloatBits(flRoundX2) + g_nViewportOriginX),
                         (unsigned short)((int)FloatBits(flRoundY2) + g_nViewportOriginY));
    }

    if (g_bLeftEdgeUsed != 0 && g_bRightEdgeUsed != 0)
    {
        SWRend::FillSpans(g_nSWFaceColour);
    }
}

// A one-indexed heapsort keyed on the depth four bytes into each face.
// FUNCTION: HYPERX 0x0044aa90
void __fastcall SWRend::SortFaces(short nCount)
{
    FaceRecord* pHeld;
    FaceRecord* pLeft;

    if (nCount < 2)
    {
        return;
    }

    int nLast  = nCount;
    int nStart = (nLast >> 1) + 1;

    for (;;)
    {
        if (nStart > 1)
        {
            nStart--;
            pHeld = g_ppFaceList[nStart];
        }
        else
        {
            pHeld               = g_ppFaceList[nLast];
            g_ppFaceList[nLast] = g_ppFaceList[1];

            nLast--;

            if (nLast == 1)
            {
                g_ppFaceList[1] = pHeld;

                return;
            }
        }

        int nHole  = nStart;
        int nChild = nStart + nStart;

        while (nChild <= nLast)
        {
            if (nChild < nLast)
            {
                pLeft = g_ppFaceList[nChild];

                if (pLeft->flZ < g_ppFaceList[nChild + 1]->flZ)
                {
                    nChild++;
                }
            }

            pLeft = g_ppFaceList[nChild];

            if (pHeld->flZ < pLeft->flZ)
            {
                g_ppFaceList[nHole] = pLeft;

                nHole = nChild;
                nChild += nChild;
            }
            else
            {
                nChild = nLast + 1;
            }
        }

        g_ppFaceList[nHole] = pHeld;
    }
}

// Hand-written assembly in the original: each arm saves EBP by hand and walks the right edge
// stack in it as a data register; the unrolled fill makes two dead loads, of the dwords at 0 and
// 0x20 into EBX and EDX, before it overwrites them; and it clears registers with MOV reg, 0,
// which VC6 never emits at /O2.
// What it does: fills the spans between the two edge stacks, one row at a time, from
// g_wSpanYMin to g_wSpanYMax. A span of more than eight bytes is filled with bytes up to a dword
// boundary, then dwords, then bytes. Without the hardware flip the dwords go sixty four bytes at
// a time, and the colour goes through g_nFillColour because that arm needs every register.
// Why assembly: C cannot take EBP as a data register inside a framed function, and has no dead
// load.
// FUNCTION: HYPERX 0x0044ab40
void __cdecl SWRend::FillSpans(int nColour)
{
    if (g_bHWFlipAvailable == 0)
    {
        __asm
        {
            push ebp
            mov ebx, 0
            mov ecx, 0
            mov eax, dword ptr g_nSurfacePitchCopy
            mov edx, nColour
            mov cx, word ptr g_wSpanYMin
            mov bx, word ptr g_wSpanYMax
            mov dword ptr g_nFillColour, edx
            mov esi, dword ptr g_pLeftStack
            mov edi, dword ptr g_pRightStack
            sub ebx, ecx
            mul ecx
            lea esi, [esi+ecx*2]
            lea edi, [edi+ecx*2]
            mov ecx, 0
            mov edx, ebx
            mov ebx, 0
            mov ebp, edi
            mov edi, dword ptr g_pBackBufferBitsCopy
            mov bx, word ptr [esi]
            mov cx, word ptr [ebp]
            add edi, eax
            mov eax, dword ptr g_nFillColour
            push edi
            add edi, ebx
            inc edx
            sub ecx, ebx
        checkSpan:
            js nextRow
            inc ecx
            cmp ecx, 8
            jle fillBytes
            mov ebx, ecx
            mov ecx, edi
            neg ecx
            and ecx, 3
            sub ebx, ecx
            rep stosb
            mov ecx, ebx
            cmp ecx, 40h
            jl fillDwords
            shr ecx, 6
            push edx
        fillBlock64:
            push ebx
            mov ebx, dword ptr [edi]
            mov edx, dword ptr [edi+20h]
            mov dword ptr [edi+1ch], eax
            mov dword ptr [edi+3ch], eax
            mov dword ptr [edi+18h], eax
            mov dword ptr [edi+38h], eax
            mov dword ptr [edi+14h], eax
            mov dword ptr [edi+34h], eax
            mov dword ptr [edi+10h], eax
            mov dword ptr [edi+30h], eax
            mov dword ptr [edi+0ch], eax
            mov dword ptr [edi+2ch], eax
            mov dword ptr [edi+8], eax
            mov dword ptr [edi+28h], eax
            mov dword ptr [edi+4], eax
            mov dword ptr [edi+24h], eax
            mov dword ptr [edi], eax
            mov dword ptr [edi+20h], eax
            pop ebx
            sub ebx, 40h
            add edi, 40h
            dec ecx
            jnz fillBlock64
            pop edx
            mov ecx, ebx
        fillDwords:
            and ebx, 3
            shr ecx, 2
            rep stosd
            mov ecx, ebx
        fillBytes:
            rep stosb
        nextRow:
            pop edi
            mov ecx, dword ptr g_nSurfacePitchCopy
            lea ebp, [ebp+2]
            lea esi, [esi+2]
            add edi, ecx
            mov ebx, 0
            mov cx, word ptr [ebp]
            dec edx
            mov bx, word ptr [esi]
            jz rowsDone
            push edi
            add edi, ebx
            sub ecx, ebx
            jmp checkSpan
        rowsDone:
            pop ebp
        }
    }
    else
    {
        __asm
        {
            push ebp
            xor ecx, ecx
            mov cx, word ptr g_wSpanYMin
            xor ebx, ebx
            mov bx, word ptr g_wSpanYMax
            sub ebx, ecx
            inc ebx
            mov esi, dword ptr g_pLeftStack
            mov edi, dword ptr g_pRightStack
            lea esi, [esi+ecx*2]
            lea edi, [edi+ecx*2]
            mov eax, dword ptr g_nSurfacePitchCopy
            mul ecx
            mov edx, ebx
            add eax, dword ptr g_pBackBufferBitsCopy
            mov ebx, eax
            mov eax, nColour
            mov ebp, edi
            mov edi, ebx
            xor ebx, ebx
        flipRow:
            push edi
            xor ebx, ebx
            xor ecx, ecx
            mov bx, word ptr [esi]
            mov cx, word ptr [ebp]
            sub ecx, ebx
            js flipNextRow
            inc ecx
            add edi, ebx
            cmp ecx, 8
            jle flipBytes
            test edi, 3
            jz flipDwords
            mov byte ptr [edi], al
            inc edi
            dec ecx
            test edi, 3
            jz flipDwords
            mov byte ptr [edi], al
            inc edi
            dec ecx
            test edi, 3
            jz flipDwords
            mov byte ptr [edi], al
            inc edi
            dec ecx
        flipDwords:
            push ecx
            shr ecx, 2
            rep stosd
            pop ecx
            and ecx, 3
        flipBytes:
            rep stosb
        flipNextRow:
            pop edi
            add edi, dword ptr g_nSurfacePitchCopy
            dec edx
            jz flipDone
            lea esi, [esi+2]
            lea ebp, [ebp+2]
            jmp flipRow
        flipDone:
            pop ebp
        }
    }
}

// Hand-written assembly in the original: it saves ESP, EBP and the flags by hand with PUSH ESP,
// PUSH EBP and PUSHF; it takes EBP as the step counter; it keeps the leftward flag in BH; and
// it passes values between its arms through PUSH and POP.
// What it does: walks one edge of a polygon down the scanline stacks with Bresenham's method,
// storing one column for each row. An edge that runs down the screen goes into the left stack,
// one that runs up goes into the right. A rising edge widens the span range with its top row
// only. It returns whether it walked the edge, which a flat edge does not.
// Why assembly: C cannot save and restore ESP and the flags, or use EBP as a data register
// inside a framed function.
// FUNCTION: HYPERX 0x0044ad00
unsigned char __cdecl SWRend::ScanEdge(unsigned short nX1, unsigned short nY1, unsigned short nX2, unsigned short nY2)
{
    __asm
    {
        push esp
        push ebp
        pushf
        xor ebx, ebx
        xor edx, edx
        mov dx, nY1
        xor eax, eax
        mov ax, nY2
        mov esi, eax
        sub eax, edx
        jg fallingEdge
        jz flatEdge
        neg eax
        sub dx, ax
        mov edi, dword ptr g_pRightStack
        lea edi, [edi+edx*2]
        mov byte ptr g_bRightEdgeUsed, 1
        cmp dx, word ptr g_wSpanYMax
        jle risingBelowMax
        mov word ptr g_wSpanYMax, dx
    risingBelowMax:
        cmp word ptr g_wSpanYMin, si
        jle risingAboveMin
        mov word ptr g_wSpanYMin, si
    risingAboveMin:
        xor ecx, ecx
        mov cx, nX2
        push edx
        mov dx, nX1
        pop ebp
        sub edx, ecx
        jge risingRightward
        neg edx
        or bh, 1
    risingRightward:
        cmp edx, eax
        jg xMajor
        jmp yMajor

    fallingEdge:
        mov byte ptr g_bLeftEdgeUsed, 1
        mov edi, dword ptr g_pLeftStack
        lea edi, [edi+edx*2]
        cmp si, word ptr g_wSpanYMax
        jle fallingBelowMax
        mov word ptr g_wSpanYMax, si
    fallingBelowMax:
        cmp word ptr g_wSpanYMin, dx
        jle fallingAboveMin
        mov word ptr g_wSpanYMin, dx
    fallingAboveMin:
        xor ecx, ecx
        mov cx, nX1
        push edx
        mov dx, nX2
        pop ebp
        sub edx, ecx
        jge fallingRightward
        neg edx
        or bh, 1
    fallingRightward:
        cmp edx, eax
        jg xMajor
        jmp yMajor

    xMajor:
        push edx
        lea eax, [eax*2+0]
        lea edx, [edx*2+0]
        mov esi, eax
        sub eax, edx
        mov edx, esi
        pop ebp
        sub edx, ebp
        mov word ptr [edi], cx
        test bh, 1
        jnz xMajorLeft
    xMajorRight:
        cmp edx, 0
        jle xMajorRightSameRow
        add edx, eax
        inc ecx
        lea edi, [edi+2]
        mov word ptr [edi], cx
        dec ebp
        jg xMajorRight
        jmp walked
    xMajorRightSameRow:
        add edx, esi
        inc ecx
        dec ebp
        jg xMajorRight
        mov word ptr [edi], cx
        jmp walked
    xMajorLeft:
        cmp edx, 0
        jle xMajorLeftSameRow
        mov word ptr [edi], cx
        add edx, eax
        dec ecx
        lea edi, [edi+2]
        dec ebp
        jg xMajorLeft
        mov word ptr [edi], cx
        jmp walked
    xMajorLeftSameRow:
        add edx, esi
        dec ecx
        dec ebp
        jg xMajorLeft
        mov word ptr [edi], cx
        jmp walked

    yMajor:
        mov ebp, eax
        lea eax, [eax*2+0]
        lea edx, [edx*2+0]
        mov esi, edx
        sub edx, eax
        mov eax, esi
        sub eax, ebp
        mov word ptr [edi], cx
        test bh, 1
        jz yMajorRight
    yMajorLeft:
        cmp eax, 0
        jle yMajorLeftSameColumn
        add eax, edx
        dec ecx
        lea edi, [edi+2]
        mov word ptr [edi], cx
        dec ebp
        jg yMajorLeft
        jmp walked
    yMajorLeftSameColumn:
        add eax, esi
        lea edi, [edi+2]
        mov word ptr [edi], cx
        dec ebp
        jg yMajorLeft
        jmp walked

    flatEdge:
        mov byte ptr g_bEdgeWalked, 0
        jmp restore

    yMajorRight:
        cmp eax, 0
        jle yMajorRightSameColumn
        add eax, edx
        inc ecx
        lea edi, [edi+2]
        mov word ptr [edi], cx
        dec ebp
        jg yMajorRight
        jmp walked
    yMajorRightSameColumn:
        add eax, esi
        lea edi, [edi+2]
        mov word ptr [edi], cx
        dec ebp
        jg yMajorRight
        jmp walked

    walked:
        mov byte ptr g_bEdgeWalked, 1
    restore:
        popf
        pop ebp
        pop esp
    }

    const bool bWalked = g_bEdgeWalked != 0;

    return bWalked;
}

// FUNCTION: HYPERX 0x0044aeb0
void __cdecl SWRend::FillPolygon(const POINT* pPoints, int nPoints, unsigned int nColour)
{
    g_wSpanYMin = (short)(g_nBackBufferHeight - 1);
    g_wSpanYMax = 0;

    nColour |= ((nColour << 8 | nColour) << 8 | nColour) << 8;

    SWRend::ScanEdge((unsigned short)pPoints[nPoints - 1].x, (unsigned short)pPoints[nPoints - 1].y, (unsigned short)pPoints[0].x, (unsigned short)pPoints[0].y);

    for (int i = 1; i < nPoints; i++)
    {
        SWRend::ScanEdge((unsigned short)pPoints[i - 1].x, (unsigned short)pPoints[i - 1].y, (unsigned short)pPoints[i].x, (unsigned short)pPoints[i].y);
    }

    if (g_bLeftEdgeUsed != 0 && g_bRightEdgeUsed != 0)
    {
        SWRend::FillSpans(nColour);
    }

    g_wSpanYMin = (short)(g_nBackBufferHeight - 1);
    g_wSpanYMax = 0;
}

// Hand-written assembly in the original: it saves EBP by hand and takes it as the step counter;
// it keeps the colour in BL and two direction flags in BH; it passes values between its arms
// through PUSH and POP; and it clears EBX with MOV EBX, 0, which VC6 never emits at /O2.
// What it does: draws a line into the locked back buffer, one byte per pixel, with Bresenham's
// method. BH bit 0 means leftward and bit 1 upward. A line whose height is at least its width
// and at most one row draws only its first pixel.
// Why assembly: C cannot use EBP as a data register inside a framed function, or hand a value
// from one arm to the next on the stack.
// FUNCTION: HYPERX 0x0044af70
void __cdecl SWRend::DrawLine(int nX1, int nY1, int nX2, int nY2, int nColour)
{
    __asm
    {
        push ebp
        mov eax, nY1
        mul dword ptr g_nSurfacePitchCopy
        add eax, dword ptr g_pBackBufferBitsCopy
        add eax, nX1
        mov edi, eax
        push edi
        mov ebx, 0
        mov bl, byte ptr nColour
        mov edi, nY1
        mov eax, nY2
        mov ecx, nX1
        mov edx, nX2
        mov ebp, eax
        sub eax, edi
        jge downward
        neg eax
        or bh, 2
    downward:
        pop edi
        sub edx, ecx
        jge rightward
        neg edx
        or bh, 1
    rightward:
        cmp edx, eax
        jg xMajor
        jmp yMajor

    xMajor:
        push edx
        shl eax, 1
        shl edx, 1
        mov esi, eax
        sub eax, edx
        mov edx, esi
        pop ebp
        sub edx, ebp
        mov byte ptr [edi], bl
    xMajorStep:
        cmp edx, 0
        jle xMajorSameRow
        add edx, eax
        test bh, 2
        jz xMajorDown
        sub edi, dword ptr g_nSurfacePitchCopy
        jmp xMajorAcross
    xMajorDown:
        add edi, dword ptr g_nSurfacePitchCopy
    xMajorAcross:
        test bh, 1
        jz xMajorRight
        dec edi
        jmp xMajorPlot
    xMajorRight:
        inc edi
    xMajorPlot:
        mov byte ptr [edi], bl
        dec ebp
        jle done
        jmp xMajorStep
    xMajorSameRow:
        add edx, esi
        jmp xMajorAcross

    yMajor:
        push eax
        shl eax, 1
        shl edx, 1
        mov esi, edx
        sub edx, eax
        mov eax, esi
        pop ebp
        sub eax, ebp
        mov byte ptr [edi], bl
        cmp ebp, 1
        jle done
    yMajorStep:
        cmp eax, 0
        jle yMajorSameColumn
        add eax, edx
        test bh, 1
        jz yMajorRight
        dec edi
        jmp yMajorAlong
    yMajorRight:
        inc edi
    yMajorAlong:
        test bh, 2
        jz yMajorDown
        sub edi, dword ptr g_nSurfacePitchCopy
        jmp yMajorPlot
    yMajorDown:
        add edi, dword ptr g_nSurfacePitchCopy
    yMajorPlot:
        mov byte ptr [edi], bl
        dec ebp
        jle done
        mov byte ptr [edi], bl
        jmp yMajorStep
    yMajorSameColumn:
        add eax, esi
        jmp yMajorAlong

    done:
        pop ebp
    }
}

// FUNCTION: HYPERX 0x0044b150
BOOL SWRend::SetBltDestRect(int nLeft, int nTop, int nRight, int nBottom)
{
    return SetRect(&g_rcBltDest, nLeft, nTop, nRight, nBottom);
}

// FUNCTION: HYPERX 0x0044c980
int __cdecl SWRend::RefreshBltDestRect(HWND hWnd, WPARAM, int, int)
{
    POINT ptOrigin;
    RECT  rcClient;

    if (g_bFullScreen != 0)
    {
        SWRend::SetBltDestRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    }
    else
    {
        ptOrigin.x = 0;
        ptOrigin.y = 0;
        GetClientRect(hWnd, &rcClient);
        ClientToScreen(hWnd, &ptOrigin);
        SWRend::SetBltDestRect(ptOrigin.x, ptOrigin.y, rcClient.right + ptOrigin.x, rcClient.bottom + ptOrigin.y);
    }

    g_bSWClipRectDirty = 0;
    return 0;
}

// Hand-written assembly in the original: the run copy only. It copies an odd byte with REP MOVSB,
// then a word with REP MOVSW, then dwords with REP MOVSD. VC6 never emits REP MOVSW, and its
// memcpy copies dwords first. The block also gives the function its frame pointer.
// What it does: draws a run-length sprite into the back buffer, clipped to its right and bottom
// edges. Each row is a list of runs; a byte with the top bit set starts a run of that many
// opaque pixels, which are copied, and any other byte skips that many transparent ones.
// Why assembly: no C spelling of the copy gives the byte, word, dword order the original has.
// FUNCTION: HYPERX 0x0044f240
void __cdecl SWRend::BlitRLESprite(int nX, int nY, int nWidth, int nHeight, RLESprite* pSprite)
{
    char**         ppRows;
    unsigned char* pData;
    unsigned char* pRow;
    unsigned char  cRun;
    int            nCount;
    int            nStep;
    int            nRows;
    int            nColumn;
    unsigned char* pDest;

    if (pSprite == 0)
    {
        return;
    }

    if (nX > g_nBackBufferWidth)
    {
        return;
    }

    if (nY > g_nBackBufferHeight)
    {
        return;
    }

    if (nHeight > pSprite->cy)
    {
        nHeight = pSprite->cy;
    }

    if (nWidth > pSprite->cx)
    {
        nWidth = pSprite->cx;
    }

    if (nX + nWidth > g_nBackBufferWidth)
    {
        nWidth = g_nBackBufferWidth - nX;
    }

    if (nY + nHeight > g_nBackBufferHeight)
    {
        nHeight = g_nBackBufferHeight - nY;
    }

    if (g_bBottomUpDIB)
    {
        pDest = (g_nBackBufferHeight - nY - 1) * g_nSurfacePitchCopy + g_pBackBufferBitsCopy + nX;
        nStep = -g_nSurfacePitchCopy;
    }
    else
    {
        pDest = g_nSurfacePitchCopy * nY + g_pBackBufferBitsCopy + nX;
        nStep = g_nSurfacePitchCopy;
    }

    if (nHeight <= 0)
    {
        return;
    }

    nRows  = nHeight;
    ppRows = pSprite->apRows;

    do
    {
        pRow    = pDest;
        nColumn = 0;
        pData   = (unsigned char*)*ppRows;

        while (nColumn < nWidth)
        {
            cRun = *pData;
            pData++;

            if ((cRun & 0x80) != 0)
            {
                cRun &= 0x7f;

                nCount = cRun;

                if (nCount + nColumn >= nWidth)
                {
                    nCount = nWidth - nColumn;
                }

                __asm
                    {
                    mov ecx, nCount
                    mov esi, pData
                    mov ebx, nCount
                    mov edi, pRow
                    and ecx, 1
                    sub ebx, ecx
                    rep movsb
                    mov ecx, ebx
                    and ecx, 2
                    sub ebx, ecx
                    shr ecx, 1
                    rep movsw
                    mov ecx, ebx
                    shr ecx, 2
                    rep movsd
                    }

                pData += nCount;
            }

            nColumn += cRun;
            pRow += cRun;
        }

        ppRows++;
        pDest += nStep;
        nRows--;
    } while (nRows != 0);
}

// FUNCTION: HYPERX 0x0044f7d0
void __cdecl SWRend::FillRect(unsigned int nX, unsigned int nY, unsigned int nWidth, int nHeight, int nColour)
{
    unsigned char* pRow;
    int            nStep;

    if (nX > (unsigned int)g_nBackBufferWidth)
    {
        return;
    }

    if (nY > (unsigned int)g_nBackBufferHeight)
    {
        return;
    }

    if ((int)(nWidth + nX) > g_nBackBufferWidth)
    {
        nWidth = g_nBackBufferWidth - nX;
    }

    if ((int)(nY + nHeight) > g_nBackBufferHeight)
    {
        nHeight = g_nBackBufferHeight - nY;
    }

    if (g_bBottomUpDIB)
    {
        pRow  = g_pBackBufferBitsCopy + ((g_nBackBufferHeight - nY) - 1) * g_nSurfacePitchCopy + nX;
        nStep = -g_nSurfacePitchCopy;
    }
    else
    {
        pRow  = g_pBackBufferBitsCopy + nY * g_nSurfacePitchCopy + nX;
        nStep = g_nSurfacePitchCopy;
    }

    if (nHeight > 0)
    {
        nX = nHeight;

        do
        {
            memset(pRow, nColour, nWidth);

            pRow += nStep;
        } while (--nX != 0);
    }
}

// With no second material the previous face's stays; the rasteriser reads it only when told to.
// FUNCTION: HYPERX 0x0046f970
void __fastcall SWRend::DrawFaceList(MeshBone* pThis, FaceRecord** ppFaces, short nCount)
{
    MeshMaterial* apMaterials[2];
    int*          pColours;

    int nShadeMode = pThis->HasColours() | 0xfffffffe;

    while (nCount-- > 0)
    {
        FaceRecord* const pFace = *ppFaces;

        MeshFace* pEntry = pThis->m_pMesh->Face(pFace->nFace);

        EngineObject* pRendOwner = pThis->m_pOwner;

        apMaterials[0] = &pRendOwner->m_pModelBlock->m_pMaterials[pEntry->cMaterial];

        const short nSecond = apMaterials[0]->nBackIndex;

        if (nSecond >= 0)
        {
            apMaterials[1] = &pRendOwner->m_pModelBlock->m_pMaterials[nSecond];
        }

        if (pFace->cFlip == 0 && nShadeMode == -1)
        {
            pColours = pThis->m_pFaceColours + pFace->nFace;
        }
        else
        {
            pColours = 0;
        }

        SWRend::DrawFace(pThis, pFace->cFlip, pEntry, apMaterials, pColours, g_pModelVertexBuf, &pEntry->vNormal, pFace->cClip);

        ppFaces--;
    }
}

// FUNCTION: HYPERX 0x0047fb70
void __fastcall SWRend::ClipPolyAgainstPlane(ClipPolyBuffer* pIn, ClipPolyBuffer* pOut, int nAxis, float flAxisScale, float flWScale)
{
    pOut->nCount           = 0;
    pOut->nMask            = pIn->nMask;
    const int   nRemaining = pIn->nCount;
    ClipVertex* pCurr      = &pIn->aVertices[0];
    ClipVertex* pPrev      = &pIn->aVertices[nRemaining - 1];
    double      dPrev      = flAxisScale * pPrev->af[nAxis] - flWScale * pPrev->pos.w;

    if (nRemaining > 0)
    {
        int nLeft = nRemaining;

        do
        {
            const double dCurr = flAxisScale * pCurr->af[nAxis] - flWScale * pCurr->pos.w;

            if ((dPrev <= 0.0) ^ (dCurr <= 0.0))
            {
                const double dBlend   = dPrev / (dPrev - dCurr);
                float*       pBlended = pOut->aVertices[pOut->nCount].af;
                int          nMask    = pIn->nMask;
                const float* pFrom    = pPrev->af;
                const float* pTo      = pCurr->af;

                while (nMask != 0)
                {
                    if ((nMask & 1) != 0)
                    {
                        *pBlended = (float)((*pTo - *pFrom) * dBlend + *pFrom);
                    }

                    nMask >>= 1;
                    pFrom++;
                    pTo++;
                    pBlended++;
                }

                pOut->nCount++;
            }

            if (dCurr <= 0.0)
            {
                pOut->aVertices[pOut->nCount] = *pCurr;
                pOut->nCount++;
            }

            pPrev = pCurr;
            dPrev = dCurr;
            pCurr++;
        } while (--nLeft != 0);
    }
}
