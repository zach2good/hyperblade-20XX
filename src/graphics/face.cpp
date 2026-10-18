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

constexpr auto kClipAll = 0xff;

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
    const auto cClip = Face::ClipCodeAndCullCheck(pFace, pVertexBuf);

    *pcClipOut = cClip;

    if (static_cast<unsigned char>(cClip) == kClipAll)
    {
        return 0;
    }

    if ((cClip & kClipNear) == 0)
    {
        Vec3 v0;
        Vec3 v1;
        Vec3 v2;

        const short* pIndices = pFace->aCorners;

        const auto* p2 = &pVertexBuf[pIndices[2]];
        const auto* p1 = &pVertexBuf[pIndices[1]];
        const auto* p0 = &pVertexBuf[pIndices[0]];

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

    const auto nCount = pFace->nCorners;

    unsigned char cCode = 0;

    auto cLeft   = 0;
    auto cRight  = 0;
    auto cTop    = 0;
    auto cBottom = 0;
    auto cNear   = 0;
    auto cFar    = 0;

    const auto flXScale = g_projRenderParams.flMaxX;
    const auto flYScale = g_projRenderParams.flMaxY;

    for (int i = nCount; i > 0; i--)
    {
        const auto* p = &pVertices[*pIndices];

        const auto   flW     = p->w;
        const float* pflAxis = p->af;

        if (*pflAxis < 0.0)
        {
            cLeft++;
            cCode |= kClipLeft;
        }

        if (flW * flXScale < *pflAxis)
        {
            cRight++;
            cCode |= kClipRight;
        }

        if (*++pflAxis < 0.0)
        {
            cTop++;
            cCode |= kClipTop;
        }

        if (flW * flYScale < *pflAxis)
        {
            cBottom++;
            cCode |= kClipBottom;
        }

        if (*++pflAxis < 0.0)
        {
            cNear++;
            cCode |= kClipNear;
        }

        if (flW <= *pflAxis)
        {
            cFar++;
            cCode |= kClipFar;
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

constexpr auto kAxisX = 0;
constexpr auto kAxisY = 1;
constexpr auto kAxisZ = 2;

// FUNCTION: HYPERX 0x0047f8f0
unsigned char Face::ClipPolygon(ClipPolyBuffer* pPoly, unsigned char cClipFlags)
{
    ClipPolyBuffer* pSwap;

    auto*           pSrc = pPoly;
    ClipPolyBuffer* pDst = &g_clipPolyStaging;

    if ((cClipFlags & kClipLeft) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, kAxisX, -1.0f, g_projRenderParams.flMinX * -1.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & kClipRight) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, kAxisX, 1.0f, g_projRenderParams.flMaxX);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & kClipTop) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, kAxisY, -1.0f, g_projRenderParams.flMinY * -1.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & kClipBottom) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, kAxisY, 1.0f, g_projRenderParams.flMaxY);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & kClipNear) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, kAxisZ, -1.0f, g_projRenderParams.flMinZ * -1.0f);

        if (pDst->nCount == 0)
        {
            pPoly->nCount = 0;

            return 1;
        }

        pSwap = pSrc;
        pSrc  = pDst;
        pDst  = pSwap;
    }

    if ((cClipFlags & kClipFar) != 0)
    {
        SWRend::ClipPolyAgainstPlane(pSrc, pDst, kAxisZ, 0.0f, 0.0f);

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
