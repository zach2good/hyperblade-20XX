#ifndef HYPERBLADE_CTRACKPASSERRECEIVER_H
#define HYPERBLADE_CTRACKPASSERRECEIVER_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// The five pointers are what its init fills.
// VTABLE: HYPERX 0x004b1fa8
class CTrackPasser_Receiver : public ReleasingAgendaItem
{
    // m_pOwner is a camera for this kind.

public:
    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    Vec3* m_pReceiverPos; // 0x10
    Vec3* m_pCamPos;      // 0x14
    Vec3* m_pCamDir;      // 0x18
    Vec3* m_pCamVel;      // 0x1c
    Vec3* m_pDesiredVel;  // 0x20
    // OnListChanged seeds it with the same constant six other kinds seed a field with.
    float m_flSeed; // 0x24
};

#endif
