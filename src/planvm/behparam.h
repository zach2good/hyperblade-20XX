#ifndef HYPERBLADE_BEHPARAM_H
#define HYPERBLADE_BEHPARAM_H

struct PlanNode;
class GameBehaviorObject;

class PlayerObject;
class EngineObject;

namespace BehParam
{

void SetInteractorLatency(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pTarget);
void SetDigressive(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pTarget);
void SetPurgeFlag(EngineObject* pActor, PlanNode* pItem, GameBehaviorObject* pTarget);
void SetTargetGoal(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetTarget(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetRelativePosition(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetModelImRelativeTo(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetAnimation(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetAnimationEvent(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetObjectToFollow(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetObjectToAvoid(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetModelImFacing(EngineObject* pActor, PlanNode* pNotify, GameBehaviorObject* pTrajectory);
void SetTargetPosition(EngineObject* pActor, PlanNode* pNotify, GameBehaviorObject* pReceiver);
void SetModsTweenNFacing(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour);
void SetPersistentAnimation(EngineObject* pActor, PlanNode* pRef, GameBehaviorObject* pOut);

// The linker folded this empty body onto Stub::Assert, which carries the marker.
inline void SetKeyMonitorMode(EngineObject*, PlanNode*, GameBehaviorObject*)
{
}

} // namespace BehParam

struct PlanNode;
class GameBehaviorObject;

#endif
