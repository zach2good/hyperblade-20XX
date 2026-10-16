#ifndef HYPERBLADE_SWREND_H
#define HYPERBLADE_SWREND_H

#include "common/compat.h"

struct FaceRecord;
class MeshBone;
struct MeshFace;
struct MeshMaterial;
struct RLESprite;
struct TransformedVertex;

namespace SWRend
{

void          BlitRLESprite(int nX, int nY, int nWidth, int nHeight, RLESprite* pSprite);
unsigned char ScanEdge(unsigned short nX1, unsigned short nY1, unsigned short nX2, unsigned short nY2);
void          FillSpans(int nColour);
void          FillPolygon(const POINT* pPoints, int nPoints, unsigned int nColour);
void          FillRect(unsigned int nX, unsigned int nY, unsigned int nWidth, int nHeight, int nColour);
int           RefreshBltDestRect(HWND hWnd, WPARAM wParam, int nWidth, int nHeight);
BOOL          SetBltDestRect(int nLeft, int nTop, int nRight, int nBottom);
void          SortFaces(short nCount);
void          ClipPolyAgainstPlane(struct ClipPolyBuffer* pIn, struct ClipPolyBuffer* pOut, int nAxis, float flAxisScale, float flWScale);
void          DrawLine(int nX1, int nY1, int nX2, int nY2, int nColour);
void          DrawFace(MeshBone* pRend, char cFlip, MeshFace* pFaceData, MeshMaterial** ppMaterials, int* pVertexColour, struct TransformedVertex* pVertexBufBase, struct Vec3* pFaceNormal, char cClipFlags);
void          DrawFaceList(MeshBone* pThis, FaceRecord** ppFaces, short nCount);

} // namespace SWRend

#endif
