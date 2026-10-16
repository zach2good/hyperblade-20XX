#include "math/floatbits.h"
#include "common/compat.h"
#include <string.h>
#include <stdlib.h>
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

namespace
{

// Fewer than three vertices, or one at or behind the eye.
int FaceUnprojectable()
{
    if (g_swFaceBuffer.nCount < 3)
    {
        return 1;
    }

    for (int v = 0; v < g_swFaceBuffer.nCount; v++)
    {
        if (g_swFaceBuffer.aVertices[v].pos.w <= 0.0001f)
        {
            return 1;
        }
    }

    return 0;
}

// Takes the two rounded row coordinates, whose low bits are the row once the bias is added. The
// edge stacks hold one entry per back-buffer row, so that is the bound.
int EdgeOffScreen(float flRoundY1, float flRoundY2)
{
    const int nRow1 = (short)((int)FloatBits(flRoundY1) + g_nViewportOriginY);
    const int nRow2 = (short)((int)FloatBits(flRoundY2) + g_nViewportOriginY);

    if (nRow1 < 0 || nRow1 >= g_nBackBufferHeight || nRow2 < 0 || nRow2 >= g_nBackBufferHeight)
    {
        return 1;
    }

    return 0;
}

} // namespace

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

    short* pUVIndex = 0;

    // Only an MDM-textured face that carries UV indices takes texture coordinates.
    if (g_cMdmTexLoaded != 0 && (pMatTexture = ppMaterials[0]->pTexture) != 0 && (pFaceData->cFlags & FACE_FLAG_HAS_UV) != 0)
    {
        pUVIndex = pFaceData->UVIndices();
    }

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

        g_swFaceBuffer.nMask = g_nSWVertexUVMask | g_nSWVertexPosMask;
        pTexture             = pMatTexture->m_pSoftTexture;

        for (i = 0; i < pFaceData->nCorners; i++)
        {
            const TexCoord* pUV;

            pVert = &g_swFaceBuffer.aVertices[i];

            const TransformedVertex* const pSource = &pVertexBufBase[*pIndex++];

            pVert->pos.x = pSource->x;
            pVert->pos.y = pSource->y;
            pVert->pos.z = pSource->z;
            pVert->pos.w = pSource->w;

            pUV = &pUVTable[*pUVIndex++];

            pVert->uv = *pUV;
        }
    }
    else
    {
        for (i = 0; i < pFaceData->nCorners; i++)
        {
            pVert = &g_swFaceBuffer.aVertices[i];

            const TransformedVertex* const pSource = &pVertexBufBase[*pIndex++];

            pVert->pos.x = pSource->x;
            pVert->pos.y = pSource->y;
            pVert->pos.z = pSource->z;
            pVert->pos.w = pSource->w;
        }
    }

    if (cClipFlags != 0 && Face::ClipPolygon(&g_swFaceBuffer, cClipFlags) != 0)
    {
        return;
    }

    // A clipped face can come back degenerate, and its projection divides by zero.
    if (FaceUnprojectable() != 0)
    {
        return;
    }

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

                g_nSWFaceColour =
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
                g_nSWFaceColour = (unsigned int)pMaterial->nColour;
            }
        }
        else
        {
            flLevel = g_flZeroSWRend;

            nKind = MeshMaterial::CalcLightIntensity(&flLevel, pMaterial, &vNormal) & 0xff;

            switch (nKind)
            {
                case SHADE_WHITE:
                    g_nSWFaceColour = (unsigned int)pMaterial->nColour;
                    break;

                case SHADE_SCALED:
                    g_nSWFaceColour =
                        ((((unsigned int)(int)(pMaterial->acChannel[2] * flLevel) | COLOUR_ALPHA_BITS)
                              << 8 |
                          (unsigned int)(int)(pMaterial->acChannel[1] * flLevel))
                         << 8) |
                        (unsigned int)(int)(pMaterial->acChannel[0] * flLevel);

                    break;

                default:
                    g_nSWFaceColour = COLOUR_OPAQUE;
                    break;
            }
        }
    }
    else
    {
        if (pVertexColour != 0)
        {
            g_nSWFaceColour = (unsigned int)*pVertexColour;
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

                    g_nSWFaceColour =
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
                    g_nSWFaceColour = (unsigned int)pMaterial->nColour;
                }
            }
            else
            {
                flLevel2 = g_flZeroSWRend;

                nKind = MeshMaterial::CalcLightIntensity(&flLevel2, pMaterial, pFaceNormal) & 0xff;

                switch (nKind)
                {
                    case SHADE_WHITE:
                        g_nSWFaceColour = (unsigned int)pMaterial->nColour;
                        break;

                    case SHADE_SCALED:
                        g_nSWFaceColour =
                            ((((unsigned int)(int)(pMaterial->acChannel[2] * flLevel2) | COLOUR_ALPHA_BITS)
                                  << 8 |
                              (unsigned int)(int)(pMaterial->acChannel[1] * flLevel2))
                             << 8) |
                            (unsigned int)(int)(pMaterial->acChannel[0] * flLevel2);

                        break;

                    default:
                        g_nSWFaceColour = COLOUR_OPAQUE;
                        break;
                }
            }
        }
    }

    i          = 0;
    int nIndex = 0;

    if ((g_bBottomUpDIB != 0) == (cBackFace != 0))
    {
        nFirst = nIndex;
        nStep  = 1;
        nEnd   = g_swFaceBuffer.nCount;
    }
    else
    {
        nStep  = -1;
        nFirst = g_swFaceBuffer.nCount - 1;
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

        // An edge outside the frame's rows would scan past the edge tables.
        if (EdgeOffScreen(flRoundY1, flRoundY2) != 0)
        {
            continue;
        }

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

// Fills the spans between the two edge stacks, one row at a time, from g_wSpanYMin to
// g_wSpanYMax. A row whose right edge is left of its left edge is skipped.
// FUNCTION: HYPERX 0x0044ab40
void __cdecl SWRend::FillSpans(int nColour)
{
    const auto nTop    = (int)(unsigned short)g_wSpanYMin;
    const auto nBottom = (int)(unsigned short)g_wSpanYMax;

    unsigned char* pRow = g_pBackBufferBitsCopy + nTop * g_nSurfacePitchCopy;

    for (int y = nTop; y <= nBottom; y++)
    {
        const int nLeft  = g_pLeftStack[y];
        const int nCount = (int)g_pRightStack[y] - nLeft;

        if (nCount >= 0)
        {
            memset(pRow + nLeft, nColour & 0xff, nCount + 1);
        }

        pRow += g_nSurfacePitchCopy;
    }
}

// Walks one polygon edge from its top row to its bottom with Bresenham's method, writing the x of
// each row into the left stack for a falling edge and the right stack for a rising one, and
// widens the span's rows. A flat edge walks nothing. The rising arm widens both bounds by its top
// row only, as the original does.
// FUNCTION: HYPERX 0x0044ad00
unsigned char __cdecl SWRend::ScanEdge(unsigned short nX1, unsigned short nY1, unsigned short nX2, unsigned short nY2)
{
    if (nY1 == nY2)
    {
        g_bEdgeWalked = 0;

        return g_bEdgeWalked;
    }

    const auto bFalling = nY2 > nY1;

    const auto nTop = [&]
    {
        if (bFalling)
        {
            return (int)nY1;
        }

        return (int)nY2;
    }();

    const auto nDY = std::abs((int)nY2 - (int)nY1);

    auto nX = [&]
    {
        if (bFalling)
        {
            return (int)nX1;
        }

        return (int)nX2;
    }();

    const auto nSignedDX = [&]
    {
        if (bFalling)
        {
            return (int)nX2 - (int)nX1;
        }

        return (int)nX1 - (int)nX2;
    }();

    const auto nDX   = std::abs(nSignedDX);
    const auto nStep = [&]
    {
        if (nSignedDX < 0)
        {
            return -1;
        }

        return 1;
    }();

    unsigned short* pOut = [&]
    {
        if (bFalling)
        {
            g_bLeftEdgeUsed = 1;

            return g_pLeftStack + nTop;
        }

        g_bRightEdgeUsed = 1;

        return g_pRightStack + nTop;
    }();

    if ((short)nY2 > g_wSpanYMax)
    {
        g_wSpanYMax = (short)nY2;
    }

    if (g_wSpanYMin > (short)nTop)
    {
        g_wSpanYMin = (short)nTop;
    }

    *pOut = (unsigned short)nX;

    if (nDX > nDY)
    {
        const auto nDiagonal = 2 * nDY - 2 * nDX;
        const auto nStraight = 2 * nDY;

        auto nError = 2 * nDY - nDX;
        auto nLeft  = nDX;

        if (nStep > 0)
        {
            while (true)
            {
                if (nError > 0)
                {
                    nError += nDiagonal;
                    nX++;
                    pOut++;
                    *pOut = (unsigned short)nX;

                    if (--nLeft <= 0)
                    {
                        break;
                    }
                }
                else
                {
                    nError += nStraight;
                    nX++;

                    if (--nLeft <= 0)
                    {
                        *pOut = (unsigned short)nX;
                        break;
                    }
                }
            }
        }
        else
        {
            while (true)
            {
                if (nError > 0)
                {
                    *pOut = (unsigned short)nX;
                    nError += nDiagonal;
                    nX--;
                    pOut++;
                }
                else
                {
                    nError += nStraight;
                    nX--;
                }

                if (--nLeft <= 0)
                {
                    *pOut = (unsigned short)nX;
                    break;
                }
            }
        }
    }
    else
    {
        const auto nDiagonal = 2 * nDX - 2 * nDY;
        const auto nStraight = 2 * nDX;

        auto nError = 2 * nDX - nDY;
        auto nLeft  = nDY;

        while (true)
        {
            if (nError > 0)
            {
                nError += nDiagonal;
                nX += nStep;
            }
            else
            {
                nError += nStraight;
            }

            pOut++;
            *pOut = (unsigned short)nX;

            if (--nLeft <= 0)
            {
                break;
            }
        }
    }

    g_bEdgeWalked = 1;

    return g_bEdgeWalked;
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

// Draws a line into the locked back buffer, one byte per pixel, with Bresenham's method. A line
// whose height is at least its width and at most one row draws only its first pixel.
// FUNCTION: HYPERX 0x0044af70
void __cdecl SWRend::DrawLine(int nX1, int nY1, int nX2, int nY2, int nColour)
{
    const auto nPitch = g_nSurfacePitchCopy;
    const auto cPixel = (unsigned char)nColour;

    const auto nDX = std::abs(nX2 - nX1);
    const auto nDY = std::abs(nY2 - nY1);

    const auto nAcross = [&]
    {
        if (nX2 < nX1)
        {
            return -1;
        }

        return 1;
    }();

    const auto nAlong = [&]
    {
        if (nY2 < nY1)
        {
            return -nPitch;
        }

        return nPitch;
    }();

    unsigned char* p = g_pBackBufferBitsCopy + nY1 * nPitch + nX1;

    *p = cPixel;

    if (nDX > nDY)
    {
        const auto nDiagonal = 2 * nDY - 2 * nDX;
        const auto nStraight = 2 * nDY;

        auto nError = 2 * nDY - nDX;

        for (int nLeft = nDX; nLeft > 0; nLeft--)
        {
            if (nError > 0)
            {
                nError += nDiagonal;
                p += nAlong;
            }
            else
            {
                nError += nStraight;
            }

            p += nAcross;
            *p = cPixel;
        }

        return;
    }

    if (nDY <= 1)
    {
        return;
    }

    const auto nDiagonal = 2 * nDX - 2 * nDY;
    const auto nStraight = 2 * nDX;

    auto nError = 2 * nDX - nDY;

    for (int nLeft = nDY; nLeft > 0; nLeft--)
    {
        if (nError > 0)
        {
            nError += nDiagonal;
            p += nAcross;
        }
        else
        {
            nError += nStraight;
        }

        p += nAlong;
        *p = cPixel;
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

                memcpy(pRow, pData, nCount);

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
