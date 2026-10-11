#ifndef HYPERBLADE_COLLISIONCHECK_H
#define HYPERBLADE_COLLISIONCHECK_H

#include "common/compat.h"

class Interactor;
struct PlanNode;
struct Vec3;

namespace CollisionCheck
{

void __cdecl  DromerLaserHurdleCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
Vec3* __cdecl ComputeNearestNormal(Vec3* pNormalOut, const struct MeshBoundingBox* pBox, Vec3* pPoint);
void __cdecl  DromerRampCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void __cdecl  DromerLimboCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void __cdecl  BouncedInGoal(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl  DromerHurdleCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void __cdecl  RokHurdleCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void __cdecl  DromerRandomizerCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl  DromerMultiplierCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);

} // namespace CollisionCheck

#endif
