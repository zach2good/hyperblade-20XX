#ifndef HYPERBLADE_PLANINTERACTORFACTORIES_H
#define HYPERBLADE_PLANINTERACTORFACTORIES_H

#include "common/compat.h"

class Interactor;
class CollInteractor;
class EngineObject;
class GameBehaviorObject;
class MsgInteractor;
struct PlanNode;

namespace PlanInteractorFactories
{

MsgInteractor*  CreateMsgInteractor(EngineObject* pObject, PlanNode* pInstr, GameBehaviorObject* pNode, Interactor* pPrevious);
CollInteractor* CreateCollInteractor(EngineObject* pObject, PlanNode* pInstr, GameBehaviorObject* pNode, Interactor* pPrevious);
void            ResetAll();
void            DestroyAll();

} // namespace PlanInteractorFactories

#endif
