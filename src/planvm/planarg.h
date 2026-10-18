// The plan argument resolvers that resolve a pronoun from the actor's slot.

#ifndef HYPERBLADE_PLANARG_H
#define HYPERBLADE_PLANARG_H

#include "engine/devicector.h"
#include "engine/collisionobjectids.h"
#include "engine/engineobject.h"
#include "engine/objectclasses.h"
#include "planvm/plannode.h"

class EngineObject;

class Interactor;
struct PlanNode;
union PlanWord;

namespace PlanArg
{

short GetMe(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyTeammate(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyLeadOpponent(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyOtherOpponent(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyGoal(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyLeadTeammate(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyGoalSide(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyGoalZone(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetOpponentsGoal(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetOppnGoalSide(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetOppnGoalZone(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyCollisionBub(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyTeammatesCOB(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyAwarenessBub(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyLeadOppnCollBub(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyOtherOppnCollBub(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyBodyBubble(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetLeadOppnBodyBub(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetOtherOppnBodyBub(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetRampCollisionObject(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMultiplierCollisionObject(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetRedRandomizerCollisionObject(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyBackDefWgPos(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyGoalPos(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetMyDragStartPos(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetBodyPartCollisionObject(EngineObject** ppActor, const PlanWord* pStream, int* pResult);
short GetViewOwner(EngineObject** ppUnused, const PlanWord* pStream, int* pResult);
float ResolveFloat(Interactor* pContext, PlanNode* pNode, int nIndex);

// The linker folded these bodies onto GetMultiplierCollisionObject, which carries the marker.
inline short GetTurnstileCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_TURNSTILE - SLOT_TURNSTILE)));
}

inline short GetLimboPoleCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_LIMBO_POLE - SLOT_LIMBO_POLE)));
}

inline short GetLaserHurdleCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_LASER_HURDLE - SLOT_LASER_HURDLE)));
}

inline short GetFastLaneCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_FAST_LANE - SLOT_FAST_LANE)));
}

inline short GetSlowLaneCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_SLOW_LANE - SLOT_SLOW_LANE)));
}

inline short GetMineCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_MINE - SLOT_MINE)));
}

inline short GetSwarfCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_SWARF - SLOT_SWARF)));
}

inline short GetExplosionCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_EXPLOSION - SLOT_EXPLOSION)));
}

inline short GetArmoryCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_ARMORY - SLOT_ARMORY)));
}

inline short GetHurdleCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_HURDLE - SLOT_HURDLE)));
}

// The linker folded this body onto GetRedRandomizerCollisionObject, which carries the marker.
inline short GetGreenRandomizerCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (COB_GREEN_RANDOMIZER - SLOT_GREEN_RANDOMIZER)));
}

// The linker folded this body onto GetMyDragStartPos, which carries the marker.
inline short GetMyGoalie(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((nMe & 1) | GOALIE_SLOT_A));
}

// The linker folded these bodies onto GetViewOwner, which carries the marker.
inline short GetViewOwnerCOB(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetVwOwnrAwarnsBub(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetViewOnrsOppn1COB(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetViewOnrsOppn2COB(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetWatchedObject(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetTrackedObject(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetTrackedObjsTeammate(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetViewOwnersGoal(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetVwOwnrOppnGoalie(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetViewOwnersTeammate(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetViewOnrsOppn1(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetViewOnrsOppn2(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetVwOwnrsOppnGoal(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetRokCarrier(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetLastRokCarrier(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetPrevRokCarrier(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetRokCarrierCOB(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

inline short GetNonRokCarryingOppn(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

} // namespace PlanArg

#endif
