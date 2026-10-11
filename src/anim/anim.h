#ifndef HYPERBLADE_ANIM_H
#define HYPERBLADE_ANIM_H

#include "common/compat.h"

class TransformObject;
struct AnimClipHeader;
struct PackedRotation;

namespace Anim
{

void __cdecl EulerToMatrix(const float* pInputEulers, PackedRotation* pOutput, short nFrames, const AnimClipHeader* pHeader, TransformObject* pTempXform);
void __cdecl UnpackFixed16ToFloat(const PackedRotation* pIn, float* pOut);

} // namespace Anim

#endif
