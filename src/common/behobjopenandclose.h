#ifndef HYPERBLADE_BEHOBJOPENANDCLOSE_H
#define HYPERBLADE_BEHOBJOPENANDCLOSE_H

#include "engine/inplacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b278c
class BehObj_OpenAndClose : public InPlaceBehaviorObject
{
public:
    ~BehObj_OpenAndClose() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    void SendTimedMessage() override;
    BehObj_OpenAndClose();
};

// SYNTHETIC: HYPERX 0x0041e630
// BehObj_OpenAndClose::`scalar deleting destructor'

#endif
