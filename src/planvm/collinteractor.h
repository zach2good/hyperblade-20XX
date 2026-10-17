#ifndef HYPERBLADE_COLLINTERACTOR_H
#define HYPERBLADE_COLLINTERACTOR_H

#include "common/compat.h"
#include "planvm/interactor.h"

// VTABLE: HYPERX 0x004b37bc
class CollInteractor : public Interactor
{
public:
    CollInteractor()
    : Interactor(-1, -1, -1, 0)
    {
    }

    // The four slots this class's table differs from its base's on.
    ~CollInteractor() override;
    // The discard src/planvm/planstate.cpp walks both interactor lists with.
    void  Discard() override;
    short GetOtherSlot() override;
    short GetModelSlot() override;

    CollInteractor* Setup(class EngineObject* pObject, struct PlanNode* pInstr);
    void            Process(int nArg, short* pOutcome, int nMode);
};

// SYNTHETIC: HYPERX 0x0046bd30
// CollInteractor::`scalar deleting destructor'

#endif
