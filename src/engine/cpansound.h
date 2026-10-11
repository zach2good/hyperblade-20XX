#ifndef HYPERBLADE_CPANSOUND_H
#define HYPERBLADE_CPANSOUND_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slot 2 is a shared Release.
// VTABLE: HYPERX 0x004b2010
class CPanSound : public ReleasingAgendaItem
{
public:
    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    // Nothing stores one; a SoundEntry is inferred from the pan it steps.
    struct SoundEntry* m_pSound; // 0x10
    // Process stops once m_nPan is within one m_nPanStep of m_nPanTarget.
    int m_nPan;       // 0x14
    int m_nPanTarget; // 0x18
    int m_nPanStep;   // 0x1c
};

#endif
