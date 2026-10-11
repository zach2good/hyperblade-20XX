#ifndef HYPERBLADE_CCIRCLETRACK_H
#define HYPERBLADE_CCIRCLETRACK_H

#include "planvm/cameraliteagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor.
// VTABLE: HYPERX 0x004b21e4
class CCircleTrack : public CameraLiteAgendaItem
{
    // m_pOwner is a camera for this kind.

public:
    virtual void Process(int nArg);

    // The original's linker folded this onto EngineObject::CheckObjectAI, which carries the marker.
    virtual void Release(int)
    {
    }

    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);

    // Bind stores the engine clock; nothing reads it, as the circle's angle is a constant zero.
    int m_nUnused; // 0x18
};

#endif
