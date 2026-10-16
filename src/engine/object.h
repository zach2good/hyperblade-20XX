#ifndef HYPERBLADE_OBJECT_H
#define HYPERBLADE_OBJECT_H

#include "common/compat.h"

class EngineObject;
struct Vec3;

namespace Object
{

void __cdecl  CopyPhysicsStateTo(EngineObject* pSrc, EngineObject* pDest, char bBoost);
void __cdecl  SpawnAtObjectRotated(EngineObject* pSrc, EngineObject* pDest, char bMirrored);
Vec3* __cdecl ComputeSteeringDirection(Vec3* pDirOut, EngineObject* pObject, const Vec3* pQueryPos);
void __cdecl  HandleObjectCollisionBounce(EngineObject* pSelf, EngineObject* pOther, const Vec3* pSelfVelocity, const Vec3* pOtherVelocity, float flFacingDot, float flSelfWeight, float flOtherWeight, int nSelfStopped, int nOtherStopped, int nSelfSector, int nOtherSector, float flSpeed, float flOtherSpeed);
void          ScaleBones(int* pBoneIndices, int nCount, float flScaleX, float flScaleY, float flScaleZ);
Vec3* __cdecl ComputePoleGateNormal(Vec3* pDirOut, Vec3* pQueryPos);

} // namespace Object

#endif
