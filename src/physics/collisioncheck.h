#ifndef HYPERBLADE_COLLISIONCHECK_H
#define HYPERBLADE_COLLISIONCHECK_H

#include "common/compat.h"

class Interactor;
struct PlanNode;
struct Vec3;

namespace CollisionCheck
{

void  DromerLaserHurdleCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
Vec3* ComputeNearestNormal(Vec3* pNormalOut, const struct MeshBoundingBox* pBox, Vec3* pPoint);
void  DromerRampCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void  DromerLimboCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void  BouncedInGoal(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void  DromerHurdleCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void  RokHurdleCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void  DromerRandomizerCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void  DromerMultiplierCollision(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);

} // namespace CollisionCheck

#endif
