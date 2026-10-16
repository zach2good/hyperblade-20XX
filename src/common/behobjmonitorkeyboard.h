#ifndef HYPERBLADE_BEHOBJMONITORKEYBOARD_H
#define HYPERBLADE_BEHOBJMONITORKEYBOARD_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b26e4
class BehObj_MonitorKeyboard : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_MonitorKeyboard();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    virtual void SendTimedMessage();
    BehObj_MonitorKeyboard();
};

// SYNTHETIC: HYPERX 0x0041df20
// BehObj_MonitorKeyboard::`scalar deleting destructor'

#endif
