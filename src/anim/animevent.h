#ifndef HYPERBLADE_ANIMEVENT_H
#define HYPERBLADE_ANIMEVENT_H

#include "common/compat.h"

class Interactor;
struct PlanNode;

namespace AnimEvent
{

void PlaySnd(Interactor* pContext, PlanNode* pEvent, short* pState, int nArg);
void PlaySndType(Interactor* pContext, PlanNode* pArgs, short* pState, int nGroup);
void PanSndType(Interactor* pContext, PlanNode* pEvent, short* pState, int nGroup);
void DopplerSound(Interactor* pContext, PlanNode* pArgs, short* pState, int nArg);
void PlaySoundByDistance(Interactor* pContext, PlanNode* pEvent, short* pState, int nArg);
void IsAmbient(Interactor* pContext, PlanNode* pArgs, short* pResult, int nEntry);

} // namespace AnimEvent

#endif
