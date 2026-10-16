#ifndef HYPERBLADE_FACE_H
#define HYPERBLADE_FACE_H

#include "common/compat.h"

// Face flags both rasterisers test, and the clip code bits, one per frustum plane.
#define FACE_FLAG_HAS_UV  0x20
#define FACE_FLAG_UV_SKIP 0x10
#define CLIP_LEFT         0x01
#define CLIP_RIGHT        0x02
#define CLIP_TOP          0x04
#define CLIP_BOTTOM       0x08
#define CLIP_NEAR         0x10
#define CLIP_FAR          0x20

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
