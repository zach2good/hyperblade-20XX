#ifndef HYPERBLADE_BEHOBJOPENANDCLOSE_H
#define HYPERBLADE_BEHOBJOPENANDCLOSE_H

#include "engine/inplacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b278c
class BehObj_OpenAndClose : public InPlaceBehaviorObject
{
public:
    virtual ~BehObj_OpenAndClose();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    virtual void SendTimedMessage();
    BehObj_OpenAndClose();
};

// SYNTHETIC: HYPERX 0x0041e630
// BehObj_OpenAndClose::`scalar deleting destructor'

#endif
