// Uses the five-frame mean timestep, so particle motion depends on frame rate.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "gameengine.h"
#include "particle.h"
#include "globals.h"
#include "common/stub.h"
#include "planvm/agendaitem.h"
#include "engine/cexplosionanimation.h"
#include "engine/particlesystem.h"

// FUNCTION: HYPERX 0x00412780
void CExplosionAnimation::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CExplosionAnimation;
    m_pNext  = nullptr;
    m_bReady = 0;
    m_pOwner = pSubject;
}
