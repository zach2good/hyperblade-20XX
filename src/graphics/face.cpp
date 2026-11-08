#include "common/compat.h"
#include "graphics/transformedvertex.h"
#include "math/vector.h"
#include "meshblock.h"
#include "globals.h"
#include "projrender.h"
#include "ui/hudglobals.h"
#include <string.h>
#include "graphics/face.h"
#include "graphics/swrend.h"
#include "common/staticinit.h"

float g_flUnusedFaceA = g_flZeroFace;
float g_flUnusedFaceB = g_flZeroFace;
float g_flUnusedFaceC = g_flZeroFace;

// A double-sided face that faces away answers minus one so the caller swaps materials.

#define CLIP_ALL 0xff

// FUNCTION: HYPERX100 0x0045fa50
// FUNCTION: HYPERX 0x0044a9a0
int __fastcall Face::BackfaceCullAndClipCheck(MeshFace* pFace, const TransformedVertex* pVertexBuf, char cFacing, char* pcClipOut)
{
    const char cClip = Face::ClipCodeAndCullCheck(pFace, pVertexBuf);

    *pcClipOut = cClip;

    if (cClip == (char)CLIP_ALL)
    {
        return 0;
    }

    if ((cClip & CLIP_NEAR) == 0)
    {
        // One record, so the corners keep the original's frame offsets.
        struct
        {
            Vec3 v2;
            Vec3 v1;
            Vec3 v0;
        } t;

        const short* pIndices = pFace->aCorners;

        const TransformedVertex* p2 = &pVertexBuf[pIndices[2]];
        const TransformedVertex* p1 = &pVertexBuf[pIndices[1]];
        const TransformedVertex* p0 = &pVertexBuf[pIndices[0]];

        t.v0.x = (1.0f / p0->w) * p0->x;
        t.v0.y = (1.0f / p0->w) * p0->y;
        t.v1.x = (1.0f / p1->w) * p1->x;
        t.v1.y = (1.0f / p1->w) * p1->y;
        t.v2.x = (1.0f / p2->w) * p2->x;
        t.v2.y = (1.0f / p2->w) * p2->y;

        t.v1.x -= t.v0.x;
        t.v1.y -= t.v0.y;
        t.v0.x = t.v2.x - t.v0.x;
        t.v2.y -= t.v0.y;

        const float flX = t.v0.x;

        if (flX * t.v1.y - t.v2.y * t.v1.x <= 0.0)
        {
            if (cFacing != 0)
            {
                return -1;
            }

            return 0;
        }
    }

    return 1;
}

// Screen y grows down, so y below zero is above the top edge. Near is strict and far is not.
// FUNCTION: HYPERX100 0x00499520
// FUNCTION: HYPERX 0x0047f7b0
char __fastcall Face::ClipCodeAndCullCheck(const MeshFace* pFace, const TransformedVertex* pVertices)
{
    const short* pIndices = pFace->aCorners;

    const short nCount = pFace->nCorners;

    unsigned char cCode = 0;

    int cLeft   = 0;
    int cRight  = 0;
    int cTop    = 0;
    int cBottom = 0;
    int cNear   = 0;
    int cFar    = 0;

    const float flXScale = g_projRenderParams.flMaxX;
    const float flYScale = g_projRenderParams.flMaxY;

    for (int i = nCount; i > 0; i--)
    {
        const TransformedVertex* p = &pVertices[*pIndices];

        const float  flW     = p->w;
        const float* pflAxis = p->af;

        if (*pflAxis < 0.0)
        {
            cLeft++;
            cCode |= CLIP_LEFT;
        }

        if (flW * flXScale < *pflAxis)
        {
            cRight++;
            cCode |= CLIP_RIGHT;
        }

        if (*++pflAxis < 0.0)
        {
            cTop++;
            cCode |= CLIP_TOP;
        }

        if (flW * flYScale < *pflAxis)
        {
            cBottom++;
            cCode |= CLIP_BOTTOM;
        }

        if (*++pflAxis < 0.0)
        {
            cNear++;
            cCode |= CLIP_NEAR;
        }

        if (flW <= *pflAxis)
        {
            cFar++;
            cCode |= CLIP_FAR;
        }

        pIndices++;
    }

    if (cCode == 0)
    {
        return 0;
    }

    if (cLeft == nCount || cRight == nCount || cTop == nCount || cBottom == nCount || cNear == nCount || cFar == nCount)
    {
        return -1;
    }

    return cCode;
}

// One Sutherland-Hodgman pass per plane in the clip code, alternating with a scratch array.

#pragma intrinsic(memcpy)

#define AXIS_X 0
#define AXIS_Y 1
#define AXIS_Z 2

// FUNCTION: HYPERX100 0x004996e0
// FUNCTION: HYPERX 0x0047f8f0
unsigned char __fastcall Face::ClipPolygon(ClipPolyBuffer* pPoly, unsigned char cClipFlags)
{
    ClipPolyBuffer* pSwap;

    ClipPolyBuffer* pSrc = pPoly;
    ClipPolyBuffer* pDst = &g_clipPolyStaging;

    if ((cClipFlags & CLIP_LEFT) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_X, -1.0f, g_projRenderParams.flMinX * -1.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & CLIP_RIGHT) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_X, 1.0f, g_projRenderParams.flMaxX);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & CLIP_TOP) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Y, -1.0f, g_projRenderParams.flMinY * -1.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & CLIP_BOTTOM) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Y, 1.0f, g_projRenderParams.flMaxY);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & CLIP_NEAR) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Z, -1.0f, g_projRenderParams.flMinZ * -1.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & CLIP_FAR) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Z, 0.0f, 0.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if (pSrc == &g_clipPolyStaging)
    {
        memcpy(pPoly, &g_clipPolyStaging, g_clipPolyStaging.nCount * sizeof(ClipVertex) + offsetof(ClipPolyBuffer, aVertices));
    }

    return 0;
}

#ifdef HYPERX_V100
// Two when every vertex is inside, nought when the polygon is rejected or clipped away, one when clipped.
// The outcodes test against nought, but the clip planes use the minimum extents.
// FUNCTION: HYPERX100 0x004998f0
unsigned char __fastcall Face::ClassifyAndClipPolygon(ClipPolyBuffer* pPoly)
{
    ClipPolyBuffer* pSwap;

    int nLeft   = 0;
    int nRight  = 0;
    int nTop    = 0;
    int nBottom = 0;
    int nNear   = 0;
    int nFar    = 0;

    const ClipVertex* pVertex = pPoly->aVertices;

    for (int i = pPoly->nCount; i != 0; i--)
    {
        const float* const pfl = pVertex->af;

        if (pfl[0] < 0.0f)
        {
            nLeft++;
        }

        if (pfl[3] * g_projRenderParams.flMaxX < pfl[0])
        {
            nRight++;
        }

        if (pfl[1] < 0.0f)
        {
            nTop++;
        }

        if (pfl[3] * g_projRenderParams.flMaxY < pfl[1])
        {
            nBottom++;
        }

        if (pfl[2] < 0.0f)
        {
            nNear++;
        }

        if (pfl[3] <= pfl[2])
        {
            nFar++;
        }

        pVertex++;
    }

    if (nNear + nBottom + nFar + nLeft + nRight + nTop == 0)
    {
        return 2;
    }

    const int nCount = pPoly->nCount;

    if (nCount == nLeft || nCount == nRight || nCount == nTop || nCount == nBottom || nCount == nNear || nCount == nFar)
    {
        pPoly->nCount = 0;

        return 0;
    }

    ClipPolyBuffer* pSrc = pPoly;
    ClipPolyBuffer* pDst = &g_clipPolyStaging;

    if (nLeft != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_X, -1.0f, -g_projRenderParams.flMinX);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 0;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if (nRight != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_X, 1.0f, g_projRenderParams.flMaxX);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 0;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if (nTop != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Y, -1.0f, -g_projRenderParams.flMinY);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 0;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if (nBottom != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Y, 1.0f, g_projRenderParams.flMaxY);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 0;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if (nNear != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Z, -1.0f, -g_projRenderParams.flMinZ);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 0;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if (nFar != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, AXIS_Z, 0.0f, 0.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 0;
        }

        pSrc = pDst;
    }

    if (pSrc == &g_clipPolyStaging)
    {
        memcpy(pPoly, &g_clipPolyStaging, g_clipPolyStaging.nCount * sizeof(ClipVertex) + offsetof(ClipPolyBuffer, aVertices));
    }

    return 1;
}
#endif
