#ifndef HYPERBLADE_CSPEEDBURST_H
#define HYPERBLADE_CSPEEDBURST_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slot 2 is a shared Release.
// VTABLE: HYPERX 0x004b1b40
class CSpeedBurst : public ReleasingAgendaItem
{
    // m_bReady is this kind's finished flag; m_pOwner is a player.

public:
    CSpeedBurst() throw();

    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    float GetBoost() const
    {
        return *m_pBoost;
    }

    // The players either side of the owner in slot order, which are on the other team.
    class EngineObject* m_pOpponentA; // 0x10
    class EngineObject* m_pOpponentB; // 0x14
    // The body's steering scale, which the burst raises while it runs.
    float* m_pBoost;   // 0x18
    float  m_flTarget; // 0x1c
};

#endif
