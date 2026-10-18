#ifndef HYPERBLADE_FACE_H
#define HYPERBLADE_FACE_H

#include "common/compat.h"

// Face flags both rasterisers test, and the clip code bits, one per frustum plane.
constexpr auto kFaceFlagHasUV  = 0x20;
constexpr auto kFaceFlagUVSkip = 0x10;
constexpr auto kClipLeft       = 0x01;
constexpr auto kClipRight      = 0x02;
constexpr auto kClipTop        = 0x04;
constexpr auto kClipBottom     = 0x08;
constexpr auto kClipNear       = 0x10;
constexpr auto kClipFar        = 0x20;

struct ClipPolyBuffer;
struct MeshFace;

namespace Face
{

int           BackfaceCullAndClipCheck(MeshFace* pFace, const struct TransformedVertex* pVertexBuf, char cFacing, char* pcClipOut);
char          ClipCodeAndCullCheck(const MeshFace* pFace, const struct TransformedVertex* pVertices);
unsigned char ClipPolygon(ClipPolyBuffer* pPoly, unsigned char cClipFlags);

// The clip mask of nCount vertex floats from nFirst on.
inline int VertexMask(int nFirst, int nCount)
{
    return ((1 << nCount) - 1) << nFirst;
}

} // namespace Face

#endif
