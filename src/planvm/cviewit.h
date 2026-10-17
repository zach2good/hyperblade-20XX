#ifndef HYPERBLADE_CVIEWIT_H
#define HYPERBLADE_CVIEWIT_H

#include "planvm/baseresetagendaitem.h"

class EngineObject;

// Slots 0, 2 and 9 are bodies many tables share, declared where they are defined.
// VTABLE: HYPERX 0x004b1fdc
class CViewIt : public BaseResetAgendaItem
{
public:
    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
};

#endif
