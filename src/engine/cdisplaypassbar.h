#ifndef HYPERBLADE_CDISPLAYPASSBAR_H
#define HYPERBLADE_CDISPLAYPASSBAR_H

#include "planvm/baseresetagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slots 2 and 9 are shared too.
// VTABLE: HYPERX 0x004b2148
class CDisplayPassBar : public BaseResetAgendaItem
{
    // m_bReady is this kind's inactive flag.

public:
    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
};

#endif
