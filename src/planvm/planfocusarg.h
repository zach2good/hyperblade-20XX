// The runtime bank of plan argument resolvers, which answer from the engine's focus slot.

#ifndef HYPERBLADE_PLANFOCUSARG_H
#define HYPERBLADE_PLANFOCUSARG_H

class EngineObject;
struct PlanNode;

namespace PlanFocusArg
{

short         GetViewOwner(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetViewOwnerCOB(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetVwOwnrAwarnsBub(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetViewOnrsOppn1COB(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetViewOnrsOppn2COB(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetWatchedObject(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetTrackedObject(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetTrackedObjsTeammate(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetViewOwnersGoal(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetVwOwnrOppnGoalie(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetViewOwnersTeammate(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetViewOnrsOppn1(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetViewOnrsOppn2(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short         GetLastRokCarrier(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short __cdecl GetRokCarrier(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short __cdecl GetRokCarrierCOB(EngineObject* pUnusedActor, PlanNode* pUnusedArgs);
short __cdecl GetNonRokCarryingOppn(EngineObject* pActor, PlanNode* pUnusedArgs);

} // namespace PlanFocusArg

#endif
