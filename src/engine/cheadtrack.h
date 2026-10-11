#ifndef HYPERBLADE_CHEADTRACK_H
#define HYPERBLADE_CHEADTRACK_H

#include "common/fieldat.h"
#include "planvm/cameraliteagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slots 2 and 9 are shared too.
// VTABLE: HYPERX 0x004b2078
class CHeadTrack : public CameraLiteAgendaItem
{
    // m_pOwner is a camera for this kind.

public:
    CHeadTrack()
    : CameraLiteAgendaItem(0, 0)
    {
    }

    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);

    char m_bAttached; // 0x18
};

HYPERBLADE_FIELD_AT(CHeadTrack, m_pWatchPos, 0x10);

#endif
