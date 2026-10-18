#include "common/compat.h"
#include "graphics/transformedvertex.h"
#include "math/vector.h"
#include "meshblock.h"
#include "globals.h"
#include "projrender.h"
#include "ui/hudglobals.h"
#include <cstring>
#include "graphics/face.h"
#include "graphics/swrend.h"
#include "common/staticinit.h"

float g_flUnusedFaceA = g_flZeroFace;
float g_flUnusedFaceB = g_flZeroFace;
float g_flUnusedFaceC = g_flZeroFace;

// A double-sided face that faces away answers minus one so the caller swaps materials.

constexpr auto CLIP_ALL = 0xff;

namespace Face
{

// The z of the cross product of two screen-space edges.
inline float Cross2D(const Vec3* pA, const Vec3* pB)
{
    return pA->x * pB->y - pA->y * pB->x;
}

} // namespace Face

// FUNCTION: HYPERX 0x0044a9a0
int Face::BackfaceCullAndClipCheck(MeshFace* pFace, const TransformedVertex* pVertexBuf, char cFacing, char* pcClipOut)
{
    const char cClip = Face::ClipCodeAndCullCheck(pFace, pVertexBuf);

    *pcClipOut = cClip;

    if (static_cast<unsigned char>(cClip) == CLIP_ALL)
    {
        return 0;
    }

    if ((cClip & CLIP_NEAR) == 0)
    {
        Vec3 v0;
        Vec3 v1;
        Vec3 v2;

        const short* pIndices = pFace->aCorners;

        const TransformedVertex* p2 = &pVertexBuf[pIndices[2]];
        const TransformedVertex* p1 = &pVertexBuf[pIndices[1]];
        const TransformedVertex* p0 = &pVertexBuf[pIndices[0]];

        v0.x = (1.0f / p0->w) * p0->x;
        v0.y = (1.0f / p0->w) * p0->y;
        v1.x = (1.0f / p1->w) * p1->x;
        v1.y = (1.0f / p1->w) * p1->y;
        v2.x = (1.0f / p2->w) * p2->x;
        v2.y = (1.0f / p2->w) * p2->y;

        v1.x -= v0.x;
        v1.y -= v0.y;
        v2.x -= v0.x;
        v2.y -= v0.y;

        if (Cross2D(&v2, &v1) <= 0.0)
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
// FUNCTION: HYPERX 0x0047f7b0
char Face::ClipCodeAndCullCheck(const MeshFace* pFace, const TransformedVertex* pVertices)
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

constexpr auto AXIS_X = 0;
constexpr auto AXIS_Y = 1;
constexpr auto AXIS_Z = 2;

// FUNCTION: HYPERX 0x0047f8f0
unsigned char Face::ClipPolygon(ClipPolyBuffer* pPoly, unsigned char cClipFlags)
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
