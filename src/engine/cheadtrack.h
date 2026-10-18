#ifndef HYPERBLADE_CHEADTRACK_H
#define HYPERBLADE_CHEADTRACK_H

#include "planvm/cameraliteagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slots 2 and 9 are shared too.
// VTABLE: HYPERX 0x004b2078
class CHeadTrack : public CameraLiteAgendaItem
{
    // m_pOwner is a camera for this kind.

public:
    CHeadTrack()
    : CameraLiteAgendaItem(nullptr, nullptr)
    {
    }

    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;

    char m_bAttached; // 0x18
};

#endif
