#ifndef HYPERBLADE_CCHECKAI_H
#define HYPERBLADE_CCHECKAI_H

#include "planvm/baseresetagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slots 2 and 9 are shared too.
// VTABLE: HYPERX 0x004b21b0
class CCheckAI : public BaseResetAgendaItem
{
public:
    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
};

#endif
