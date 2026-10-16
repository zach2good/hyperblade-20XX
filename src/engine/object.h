#ifndef HYPERBLADE_OBJECT_H
#define HYPERBLADE_OBJECT_H

#include "common/compat.h"

class EngineObject;
struct Vec3;

namespace Object
{

void  CopyPhysicsStateTo(EngineObject* pSrc, EngineObject* pDest, char bBoost);
void  SpawnAtObjectRotated(EngineObject* pSrc, EngineObject* pDest, char bMirrored);
Vec3* ComputeSteeringDirection(Vec3* pDirOut, EngineObject* pObject, const Vec3* pQueryPos);
void  HandleObjectCollisionBounce(EngineObject* pSelf, EngineObject* pOther, const Vec3* pSelfVelocity, const Vec3* pOtherVelocity, float flFacingDot, float flSelfWeight, float flOtherWeight, int nSelfStopped, int nOtherStopped, int nSelfSector, int nOtherSector, float flSpeed, float flOtherSpeed);
void  ScaleBones(int* pBoneIndices, int nCount, float flScaleX, float flScaleY, float flScaleZ);
Vec3* ComputePoleGateNormal(Vec3* pDirOut, Vec3* pQueryPos);

} // namespace Object

#endif
