#ifndef HYPERBLADE_CTRACK_H
#define HYPERBLADE_CTRACK_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// The four pointers are what its init fills; m_vLastDir is last frame's swung look.
// VTABLE: HYPERX 0x004b1f40
class CTrack : public ReleasingAgendaItem
{
    // m_pOwner is a camera for this kind.

public:
    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    Vec3* m_pPos;        // 0x10
    Vec3* m_pDir;        // 0x14
    Vec3* m_pVel;        // 0x18
    Vec3* m_pDesiredVel; // 0x1c
    // Bind clears it as a byte and nothing else in the image reads it.
    char m_bUnused; // 0x20
    // Not padding: on a Direct3D run these bytes hold data.
    short m_wCountdown;   // 0x22
    char  m_bSwingBack;   // 0x24
    char  m_bSwingChosen; // 0x25
    char  m_bOutOfView;   // 0x26
    Vec3  m_vLastDir;     // 0x28
};

#endif
