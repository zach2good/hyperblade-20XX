#ifndef HYPERBLADE_CUPDATESHOTPOWER_H
#define HYPERBLADE_CUPDATESHOTPOWER_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slots 0 and 2 are bodies many tables share, declared where they are defined.
// VTABLE: HYPERX 0x004b2114
class CUpdateShotPower : public ReleasingAgendaItem
{
public:
    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    // The engine tick at bind, which Process measures against.
    unsigned int m_nStarted; // 0x10
    int          m_nUnused;  // 0x14
    float        m_flPower;  // 0x18
    Vec3*        m_pOrigin;  // 0x1c
};

#endif
