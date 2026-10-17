#ifndef HYPERBLADE_CDISPLAYSHOTPOWER_H
#define HYPERBLADE_CDISPLAYSHOTPOWER_H

#include "planvm/baseresetagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slots 2 and 9 are shared too.
// VTABLE: HYPERX 0x004b20e0
class CDisplayShotPower : public BaseResetAgendaItem
{
public:
    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
};

#endif
