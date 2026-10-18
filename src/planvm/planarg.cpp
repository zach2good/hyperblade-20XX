// Plan argument resolvers: most resolve an ENUMIDS.H pronoun from the actor's slot.

#include "engine/devicector.h"
#include "common/compat.h"
#include "engine/objectclasses.h"
#include "planarg.h"
#include "planvm.h"
#include "plannode.h"
#include "engine/collisionobjectids.h"
#include "common/staticinit.h"
#include "engine/engineobject.h"

float g_flUnusedPlanArgA = g_flZeroPlanArg;
float g_flUnusedPlanArgB = g_flZeroPlanArg;
float g_flUnusedPlanArgC = g_flZeroPlanArg;

// ENUMIDS.H id_POS_ positions whose north and south pair a team parity selects.
constexpr auto kPosNGoalDefender    = 0x02;
constexpr auto kPosHomeDragStartPos = 0x04;
#define POS_N_GOAL_CENTER 0x08

// Each skater owns three collision objects: its bubble, its baton zone and its body.
#define COB_PER_DROMER (kCOBDromer1 - kCOBDromer0)

// Under a kPlanArgLocalReal high half the low half indexes the actor's local reals; any other word is its own value.

// FUNCTION: HYPERX 0x0045e390
float PlanArg::ResolveFloat(Interactor* pContext, PlanNode* pNode, int nIndex)
{
    PlanWord* const pSlot = pNode->m_pArgs + static_cast<short>(nIndex);

    if (pSlot->IsLocalReal())
    {
        const auto nSlot = static_cast<short>(pSlot->n);

        PlanState* const pStats = pContext->GetActor()->GetPlanState();

        float flValue;

        if (nSlot < pStats->m_nLocalRealCount)
        {
            flValue = pStats->m_pflLocalReals[nSlot];
        }
        else
        {
            flValue = g_flPlanRealDefault;
        }

        return flValue;
    }

    return static_cast<float>(pSlot->n);
}

// FUNCTION: HYPERX 0x0047c780
short PlanArg::GetMe(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = nMe);
}

// FUNCTION: HYPERX 0x0047c7a0
short PlanArg::GetMyTeammate(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    const int nTeammate = TeammateSlot(nMe);

    *pResult = nTeammate;

    return static_cast<short>(nTeammate);
}

// FUNCTION: HYPERX 0x0047c7d0
short PlanArg::GetMyLeadOpponent(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (static_cast<short>(~nMe) & 1));
}

// FUNCTION: HYPERX 0x0047c7f0
short PlanArg::GetMyOtherOpponent(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((static_cast<short>(~nMe) & 1) | 2));
}

// FUNCTION: HYPERX 0x0047c810
short PlanArg::GetMyGoal(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    if ((nMe & 1) != 0)
    {
        *pResult = kSlotSgoal;
    }
    else
    {
        *pResult = kSlotNgoal;
    }

    return static_cast<short>(*pResult);
}

// FUNCTION: HYPERX 0x0047c840
short PlanArg::GetMyLeadTeammate(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe & 1));
}

// FUNCTION: HYPERX 0x0047c860
short PlanArg::GetMyGoalSide(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    if ((nMe & 1) != 0)
    {
        *pResult = kCOBSgoalSide;
    }
    else
    {
        *pResult = kCOBNgoalSide;
    }

    return static_cast<short>(*pResult);
}

// FUNCTION: HYPERX 0x0047c890
short PlanArg::GetMyGoalZone(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (((nMe & 1) << 2) | kCOBNgoalZone));
}

// FUNCTION: HYPERX 0x0047c8c0
short PlanArg::GetOpponentsGoal(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    if ((nMe & 1) != 0)
    {
        *pResult = kSlotNgoal;
    }
    else
    {
        *pResult = kSlotSgoal;
    }

    return static_cast<short>(*pResult);
}

// FUNCTION: HYPERX 0x0047c8f0
short PlanArg::GetOppnGoalSide(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    if ((nMe & 1) != 0)
    {
        *pResult = kCOBNgoalSide;
    }
    else
    {
        *pResult = kCOBSgoalSide;
    }

    return static_cast<short>(*pResult);
}

// FUNCTION: HYPERX 0x0047c920
short PlanArg::GetOppnGoalZone(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (((static_cast<short>(~nMe) & 1) << 2) | kCOBNgoalZone));
}

// FUNCTION: HYPERX 0x0047c950
short PlanArg::GetMyCollisionBub(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe * COB_PER_DROMER));
}

// FUNCTION: HYPERX 0x0047c970
short PlanArg::GetMyTeammatesCOB(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    const int nTeammate = TeammateSlot(nMe);

    return static_cast<short>(*pResult = (nTeammate * COB_PER_DROMER));
}

// FUNCTION: HYPERX 0x0047c9a0
short PlanArg::GetMyAwarenessBub(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + kCOBDr0Awareness));
}

// FUNCTION: HYPERX 0x0047c9c0
short PlanArg::GetMyLeadOppnCollBub(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((static_cast<short>(~nMe) & 1) * COB_PER_DROMER));
}

// FUNCTION: HYPERX 0x0047c9e0
short PlanArg::GetMyOtherOppnCollBub(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (((static_cast<short>(~nMe) & 1) | 2) * COB_PER_DROMER));
}

// FUNCTION: HYPERX 0x0047ca00
short PlanArg::GetMyBodyBubble(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe * COB_PER_DROMER + kCOBDromerBody0));
}

// FUNCTION: HYPERX 0x0047ca20
short PlanArg::GetLeadOppnBodyBub(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((static_cast<short>(~nMe) & 1) * COB_PER_DROMER + kCOBDromerBody0));
}

// FUNCTION: HYPERX 0x0047ca40
short PlanArg::GetOtherOppnBodyBub(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((((static_cast<short>(~nMe) & 1) | 2) * COB_PER_DROMER) + kCOBDromerBody0));
}

// FUNCTION: HYPERX 0x0047ca60
short PlanArg::GetRampCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (kCOBRamp - kSlotRamp)));
}

// FUNCTION: HYPERX 0x0047ca80
short PlanArg::GetMultiplierCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (kCOBMultiplier - kSlotMultiplier)));
}

// FUNCTION: HYPERX 0x0047caa0
short PlanArg::GetRedRandomizerCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (kCOBRedRandomizer - kSlotRedRandomizer)));
}

// FUNCTION: HYPERX 0x0047cac0
short PlanArg::GetMyBackDefWgPos(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((nMe & 1) | kPosNGoalDefender));
}

// FUNCTION: HYPERX 0x0047cae0
short PlanArg::GetMyGoalPos(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((nMe & 1) | POS_N_GOAL_CENTER));
}

// FUNCTION: HYPERX 0x0047cb00
short PlanArg::GetMyDragStartPos(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = ((nMe & 1) | kPosHomeDragStartPos));
}

// The view and Rok pronouns resolve at run time: each resolver keeps the first word of the argument block.
// FUNCTION: HYPERX 0x0047cb20
short PlanArg::GetViewOwner(EngineObject**, const PlanWord* pStream, int* pResult)
{
    return static_cast<short>(*pResult = pStream->n);
}

// FUNCTION: HYPERX 0x0047d050
short PlanArg::GetBodyPartCollisionObject(EngineObject** ppActor, const PlanWord*, int* pResult)
{
    const short nMe = (*ppActor)->GetSlot();

    return static_cast<short>(*pResult = (nMe + (kCOBBodyPart0 - kSlotBodyPart)));
}
