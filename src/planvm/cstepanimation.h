#ifndef HYPERBLADE_CSTEPANIMATION_H
#define HYPERBLADE_CSTEPANIMATION_H

#include "planvm/baseresetagendaitem.h"

class EngineObject;

// Slots 0 and 9 are bodies many tables share, declared where they are defined.
// VTABLE: HYPERX 0x004b1e3c
class CStepAnimation : public BaseResetAgendaItem
{
public:
    void Process(int nArg) override;
    void Release(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
};

#endif
