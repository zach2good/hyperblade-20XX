#ifndef HYPERBLADE_CEXPLOSIONANIMATION_H
#define HYPERBLADE_CEXPLOSIONANIMATION_H

#include "globals.h"
#include "engine/gameengine.h"
#include "engine/particlesystem.h"
#include "planvm/baseresetagendaitem.h"

class EngineObject;

// VTABLE: HYPERX 0x004b217c
class CExplosionAnimation : public BaseResetAgendaItem
{
public:
    // FUNCTION: HYPERX 0x004127a0
    void Process(int nArg) override
    {
        auto* const pSystem = (ParticleSystem*)m_pOwner;

        pSystem->Update(g_pGameEngine->m_clock.GetDelta());

        AgendaItem* const pNext = m_pNext;

        if (pNext != nullptr)
        {
            pNext->Process(nArg);
        }

        if (pSystem->m_bAnyAlive == 0)
        {
            pSystem->Deactivate();

            this->Detach();
        }
    }

    // The original's linker folded this body onto Process, which carries the marker.
    void Release(int nArg) override
    {
        auto* const pSystem = (ParticleSystem*)m_pOwner;

        pSystem->Update(g_pGameEngine->m_clock.GetDelta());

        AgendaItem* const pNext = m_pNext;

        if (pNext != nullptr)
        {
            pNext->Process(nArg);
        }

        if (pSystem->m_bAnyAlive == 0)
        {
            pSystem->Deactivate();

            this->Detach();
        }
    }

    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
};

#endif
