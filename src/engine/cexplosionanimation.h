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
    virtual void Process(int nArg)
    {
        ParticleSystem* const pSystem = (ParticleSystem*)m_pOwner;

        pSystem->Update(g_pGameEngine->m_clock.GetDelta());

        AgendaItem* const pNext = m_pNext;

        if (pNext != 0)
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
    virtual void Release(int nArg)
    {
        ParticleSystem* const pSystem = (ParticleSystem*)m_pOwner;

        pSystem->Update(g_pGameEngine->m_clock.GetDelta());

        AgendaItem* const pNext = m_pNext;

        if (pNext != 0)
        {
            pNext->Process(nArg);
        }

        if (pSystem->m_bAnyAlive == 0)
        {
            pSystem->Deactivate();

            this->Detach();
        }
    }

    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
};

#endif
