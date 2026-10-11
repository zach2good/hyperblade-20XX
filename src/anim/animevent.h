#ifndef HYPERBLADE_ANIMEVENT_H
#define HYPERBLADE_ANIMEVENT_H

#include "common/compat.h"

class Interactor;
struct PlanNode;

namespace AnimEvent
{

void __cdecl PlaySnd(Interactor* pContext, PlanNode* pEvent, short* pState, int nArg);
void         PlaySndType(Interactor* pContext, PlanNode* pArgs, short* pState, int nGroup);
void __cdecl PanSndType(Interactor* pContext, PlanNode* pEvent, short* pState, int nGroup);
void         DopplerSound(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void __cdecl PlaySoundByDistance(Interactor* pContext, PlanNode* pEvent, short* pState, int nArg);
void __cdecl IsAmbient(Interactor* pContext, PlanNode* pArgs, short* pResult, int nEntry);

} // namespace AnimEvent

#endif
