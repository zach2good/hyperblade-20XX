// The coach: thirteen match conditions, the per-player orders they drive, and the pass hints.

#include "common/compat.h"
#include "engine/messageids.h"
#include "planvm/gamebehaviorobject.h"
#include "objectclasses.h"
#include "teamsblock.h"
#include "devicector.h"
#include "engine/objectstats.h"
#include "globals.h"
#include "gameengine.h"
#include "physics/physicsbody.h"
#include "math/vector.h"
#include "planvm/planvm.h"
#include "objectdef.h"
#include "anim/animbinding.h"
#include "math/transformobject.h"
#include "graphics/meshbone.h"
#include "common/staticinit.h"
#include "physics/boundbubble.h"
#include "graphics/cd3dtexturemanager.h"
#include "graphics/modeldatabase.h"
#include "graphics/colorpalette.h"
#include "common/pooledclasses.h"
#include "net/session.h"
#include "cameraprojectionstate.h"
#include "cameraobject.h"
#include "graphics/meshmaterial.h"
#include "physics/collisioncage.h"
#include "planvm/agendaitem.h"
#include "statids.h"
#include "playerobject.h"
#include "anim/animplayer.h"
#include "app/gameapp.h"
#include "net/cplayer.h"
#include "planvm/msginteractor.h"
#include "rokobject.h"
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <new>
#include <cstdio>
#include "physics/collisionplane.h"
#include "engine/engineobject.h"
#include "net/netrecords.h"
#include "planvm/damagegate.h"
#include "anim/animset.h"
#include "graphics/face.h"
#include "math/mathops.h"
#include "engine/msgevent.h"
#include "planvm/plannode.h"
#include "common/stub.h"
#include "graphics/modelcomponent.h"
#include "engine/objectphase.h"
#include "engine/modelids.h"
#include "planvm/planvmglobals.h"
#include "engine/dromeobject.h"
#include "engine/goalobject.h"
#include "engine/weaponobject.h"
#include "engineglobals.h"
#include "gameoptions.h"
#include "arenastate.h"
#include "teamplayerrow.h"
#include "ui/uiglobals.h"
#include "ui/torchbitmaps.h"
#include "ui/saveload.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "objectstats.h"
#include "messagequeue.h"
#include "graphics/texcacheentry.h"
#include "common/fileobj.h"
#include "common/pathstring.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "teamblock.h"
#include "input/keyboarddeviceobj.h"
#include "rampobject.h"
#include "planvm/planopcodetable.h"
#include "saveblock.h"
#include "sound/audiomgr.h"
#include "common/stringpool.h"
#include "net/gamesession.h"
#include "particlesystem.h"
#include "planvm/collinteractor.h"
#include "engine/coachobject.h"
#include "ui/infobar.h"
#include "planvm/behaviorregistry.h"
#include "common/parseutil.h"
#include "planvm/planinteractorfactories.h"
#include "app/window.h"
#include "math/mathglobals.h"
#include "math/randroll.h"
#include <SDL3/SDL.h>

float  g_flUnusedDistanceA = g_flZeroDistance;
float  g_flUnusedDistanceB = g_flZeroDistance;
float  g_flUnusedDistanceC = g_flZeroDistance;
double g_dMsPerMinute      = 60.0 * g_dMsPerSecond;

// FUNCTION: HYPERX 0x00428010
float EngineObject::ComputeDistance(EngineObject* pFirst, EngineObject* pSecond)
{
    const auto posFirst  = pFirst->GetPhysicsBody()->pos;
    const auto posSecond = pSecond->GetPhysicsBody()->pos;

    const auto flDz = posFirst.z - posSecond.z;
    const auto flDy = posFirst.y - posSecond.y;
    const auto flDx = posFirst.x - posSecond.x;

    return static_cast<float>(sqrt(static_cast<double>(flDx * flDx + flDy * flDy + flDz * flDz)));
}

// FUNCTION: HYPERX 0x00428090
float Math::Vec3Distance(const Vec3* pA, const Vec3* pB)
{
    const auto a = *pA;
    const auto b = *pB;

    return static_cast<float>(sqrt(static_cast<double>((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z))));
}

// Spent markers read "Spin" or "ScoreSpin"; with none left the answer is 102, id_MOD_NOMODEL.
// FUNCTION: HYPERX 0x00428100
int GameEngine::FindNearestLiveMultiplier(EngineObject* pFrom)
{
    int i;

    auto flNearest = 2e8f;
    auto nNearest  = kModelIDNone;

    for (i = kSlotMultiplier; i <= kSlotMultiplierLast; i++)
    {
        auto* const pObject = g_pGameEngine->m_ppSlots[static_cast<short>(i)];

        if (pObject != nullptr)
        {
            const auto* const pszName = pObject->GetPlanState()->m_pCurrent->m_pszName;

            // STRING: HYPERX 0x004bcac8
            if (strcmp(pszName, "Spin") != 0)
            {
                // STRING: HYPERX 0x004bcabc
                if (strcmp(pszName, "ScoreSpin") != 0)
                {
                    const auto flDistance = EngineObject::ComputeDistance(pFrom, pObject);

                    if (flDistance < flNearest)
                    {
                        flNearest = flDistance;
                        nNearest  = i;
                    }
                }
            }
        }
    }

    return nNearest;
}

using CoachConditionFn = int (*)();

// The thirteen conditions the coach evaluates, in the image's order.
// GLOBAL: HYPERX 0x004bca88
extern CoachConditionFn g_apfnCoachConditions[13];

// The first of the thirteen answers, and the base the rest are reached from.
extern int g_anCoachConditions[13];

constexpr auto kCoachConditions = 13;
constexpr auto kCoachSlotLast   = 5;

constexpr auto kTeamHome = 0;
constexpr auto kTeamAway = 1;

// FUNCTION: HYPERX 0x00428190
int EngineObject::IsRokCarrier(EngineObject* pActor)
{
    auto* const pBlock = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody();

    auto* const pTracked = pBlock->pAttach;

    if (pTracked != nullptr)
    {
        const auto nTracked = pTracked->nSlot;

        if (nTracked < 6 && nTracked == pActor->GetSlot())
        {
            return 1;
        }
    }

    return 0;
}

// Neither the slot nor its teammate is a human plan, and the slot is not the viewed one.
// FUNCTION: HYPERX 0x004281d0
int GameEngine::IsComputerTeam(int nSlot)
{
    const int nWide = static_cast<short>(nSlot);

    auto* pObject = g_pGameEngine->m_ppSlots[nWide];

    // STRING: HYPERX 0x004bb340
    if (strcmp(pObject->GetPlanState()->GetName(), "Human") == 0)
    {
        return 0;
    }

    pObject = g_pGameEngine->m_ppSlots[TeammateSlot(static_cast<short>(nSlot))];

    if (strcmp(pObject->GetPlanState()->GetName(), "Human") == 0)
    {
        return 0;
    }

    const auto nLocal = g_pGameEngine->m_nViewedSlot;

    if (nLocal == nSlot)
    {
        return 0;
    }

    if (nLocal == static_cast<short>(TeammateSlot(static_cast<short>(nSlot))))
    {
        return 0;
    }

    return 1;
}

// Three, the period a match is played over.
// GLOBAL: HYPERX 0x004b3f34
extern const short g_nFinalPeriod;

// The trailing team earns the handicap on a late lead, a weak opponent, or a wide multi spread.
// FUNCTION: HYPERX 0x004282c0
int GameEngine::CheckHandicapCondition()
{
    PhysicsBody*  pBlock;
    PlayerObject* pSlot;
    int           nElapsed;
    int           i;

    pBlock = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody();

    auto* const pnCarrierSlot = pBlock->pAttach;

    if (pnCarrierSlot == nullptr || pnCarrierSlot->nSlot >= kPlayerObjectCount)
    {
        return 0;
    }

    const int nCarrier = pnCarrierSlot->nSlot;

    if (nCarrier == -1)
    {
        return 0;
    }

    if (g_pGameEngine->m_clock.nPeriod == g_nFinalPeriod)
    {
        nElapsed = g_pGameEngine->m_clock.nPauseStartMs - g_pGameEngine->m_clock.nPeriodStartMs;

        if (static_cast<double>(nElapsed) >= static_cast<double>(g_pGameEngine->m_nPeriodLengthMs) - (g_dMsPerMinute + g_dMsPerMinute))
        {
            if (abs(g_pGameEngine->GetHomeScore() - g_pGameEngine->GetAwayScore()) >= 2)
            {
                if (g_pGameEngine->GetHomeScore() < g_pGameEngine->GetAwayScore() && (nCarrier & 1) == 0)
                {
                    return 1;
                }

                if (g_pGameEngine->GetAwayScore() < g_pGameEngine->GetHomeScore() && (nCarrier & 1) == 1)
                {
                    return 1;
                }
            }
        }
    }

    for (i = 0; i < kPlayerSlotCount; i++)
    {
        if (((i ^ nCarrier) & 1) != 0)
        {
            pSlot = g_pGameEngine->GetPlayer(static_cast<short>(i));

            const auto flDamage = pSlot->m_stats.GetScalar(kRatingDamage);

            if (flDamage <= DamageGate())
            {
                return 1;
            }
        }
    }

    pSlot = g_pGameEngine->GetPlayer(static_cast<short>(nCarrier));

    const auto nSpread = static_cast<short>(pSlot->m_stats.GetScalar(kRatingMultiPointSpread));

    if ((nCarrier & 1) != 0)
    {
        if (g_pGameEngine->GetHomeScore() > g_pGameEngine->GetAwayScore() + nSpread)
        {
            return 1;
        }
    }
    else
    {
        if (g_pGameEngine->GetAwayScore() > g_pGameEngine->GetHomeScore() + nSpread)
        {
            return 1;
        }
    }

    return 0;
}

// DamageGate's third arm asks the difficulty and discards the answer.

// FUNCTION: HYPERX 0x004284c0
int GameEngine::IsAnyPlayerInjured()
{
    int i;

    for (i = 0; i < kPlayerSlotCount; i++)
    {
        auto* const pPlayer = g_pGameEngine->GetPlayer(static_cast<short>(i));

        const auto flDamage = pPlayer->m_stats.GetScalar(kRatingDamage);

        if (flDamage <= DamageGate())
        {
            return 1;
        }
    }

    return 0;
}

// id_GRT_Intelligence, used as the goalie's reach whatever the developers' name says.
constexpr auto kGoalieRatingIntelligence = 6;

// The swarf and rok carriers must be on opposite sides; both are measured to the rok side's goal.
// FUNCTION: HYPERX 0x00428550
int GameEngine::CheckOpposedCarriersRace()
{
    short       nSlotA;
    short       nSlotB;
    const Vec3* pGoal;
    double      flToGoal;

    auto* const pCarrier = g_pGameEngine->m_ppSlots[kSlotSwarf];
    auto* const pRok     = g_pGameEngine->m_ppSlots[kSlotRok];

    if (pCarrier != nullptr)
    {
        auto* const pSlotA = pCarrier->GetPhysicsBody()->pAttach;

        if (pSlotA != nullptr && (nSlotA = pSlotA->nSlot) < kPlayerObjectCount)
        {
            auto* const pSlotB = pRok->GetPhysicsBody()->pAttach;

            if (pSlotB != nullptr && (nSlotB = pSlotB->nSlot) < kPlayerObjectCount && ((nSlotA ^ nSlotB) & 1) != 0)
            {
                const Vec3* const pPosB = &g_pGameEngine->m_ppSlots[nSlotB]->GetPhysicsBody()->pos;
                const Vec3* const pPosA = &g_pGameEngine->m_ppSlots[nSlotA]->GetPhysicsBody()->pos;

                if ((nSlotB & 1) != 0)
                {
                    pGoal = &g_pGameEngine->m_ppSlots[kSlotNgoal]->GetPhysicsBody()->pos;
                }
                else
                {
                    pGoal = &g_pGameEngine->m_ppSlots[kSlotSgoal]->GetPhysicsBody()->pos;
                }

                flToGoal = Math::Vec3Distance(pPosB, pGoal);

                if (flToGoal < Math::Vec3Distance(pPosA, pGoal))
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

// Yes only when slot 55's name is "DisplayOverArmory" and a player carries the rok.

// FUNCTION: HYPERX 0x00428650
int GameEngine::CheckWeaponOutWhileRokCarried()
{
    short nSlot;

    auto* const pDisplay = g_pGameEngine->m_ppSlots[kSlotSwarf];

    if (pDisplay != nullptr)
    {
        auto* const pDefinition = pDisplay->GetPlanState();

        if (strcmp(pDefinition->m_pCurrent->m_pszName,
                   // STRING: HYPERX 0x004bcad0
                   "DisplayOverArmory") == 0)
        {
            for (nSlot = 0; nSlot < kPlayerSlotCount; nSlot++)
            {
                if (EngineObject::IsRokCarrier(g_pGameEngine->m_ppSlots[nSlot]) != 0)
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

// The coach condition that never fires; six of the thirteen entries point here.

// FUNCTION: HYPERX 0x004286e0
int GameEngine::CoachConditionNever()
{
    return 0;
}

// FUNCTION: HYPERX 0x004286f0
int GameEngine::CheckGoalieOutOfPosition()
{
    PhysicsBody* pNetA;
    PhysicsBody* pNetB;

    EngineObject** const pSlots = g_pGameEngine->m_ppSlots;

    auto* const pGoalieA = (PlayerObject*)pSlots[kGoalieSlotA];
    auto* const pGoalieB = (PlayerObject*)pSlots[kGoalieSlotB];

    auto* const pPosA = pGoalieA->GetPhysicsBody();
    auto* const pPosB = pGoalieB->GetPhysicsBody();

    pNetA = g_pGameEngine->m_ppSlots[kSlotNgoal]->GetPhysicsBody();
    pNetB = g_pGameEngine->m_ppSlots[kSlotSgoal]->GetPhysicsBody();

    const auto flReachA = pGoalieA->m_stats.GetScalar(kGoalieRatingIntelligence);
    const auto flReachB = pGoalieB->m_stats.GetScalar(kGoalieRatingIntelligence);

    if (flReachA < Math::Vec3Distance(&pPosA->pos, &pNetA->pos))
    {
        return 1;
    }

    if (flReachB < Math::Vec3Distance(&pPosB->pos, &pNetB->pos))
    {
        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x004287d0
int GameEngine::CheckCarrierBreakaway()
{
    AttachPak*  pSlot;
    int         nSlot;
    const Vec3* pGoal;

    pSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pSlot != nullptr && pSlot->nSlot < kPlayerObjectCount)
    {
        nSlot            = pSlot->nSlot;
        const int nIndex = static_cast<short>(nSlot);

        auto* const pCarrier   = g_pGameEngine->GetSlot(nIndex);
        auto* const pOpponentA = g_pGameEngine->GetSlot(nIndex ^ 1);
        auto* const pOpponentB = g_pGameEngine->GetSlot(nIndex ^ 3);

        const Vec3*       pCarrierPos = &pCarrier->GetPhysicsBody()->pos;
        const Vec3* const pPosA       = &pOpponentA->GetPhysicsBody()->pos;
        const Vec3* const pPosB       = &pOpponentB->GetPhysicsBody()->pos;

        if ((nSlot & 1) != 0)
        {
            pGoal = &g_pGameEngine->m_ppSlots[kSlotNgoal]->GetPhysicsBody()->pos;
        }
        else
        {
            pGoal = &g_pGameEngine->m_ppSlots[kSlotSgoal]->GetPhysicsBody()->pos;
        }

        // Computed twice: the original calls it twice.
        double flCarrier = Math::Vec3Distance(pCarrierPos, pGoal);

        if (flCarrier < Math::Vec3Distance(pPosA, pGoal))
        {
            flCarrier = Math::Vec3Distance(pCarrierPos, pGoal);

            if (flCarrier < Math::Vec3Distance(pPosB, pGoal))
            {
                return 1;
            }
        }
    }

    return 0;
}

// Nothing checks there are four players, so fewer reads whatever is in the table.

// FUNCTION: HYPERX 0x004288c0
int GameEngine::CheckAnyPlayerNeedsSub()
{
    short iPlayer;

    for (iPlayer = 0; iPlayer < kPlayerSlotCount; iPlayer++)
    {
        auto* const pObject = g_pGameEngine->GetPlayer(iPlayer);

        const auto flLimit = pObject->m_stats.GetScalar(kRatingSubOutDamage);

        if (flLimit > pObject->m_stats.GetScalar(kRatingDamage))
        {
            return 1;
        }
    }

    return 0;
}

constexpr auto kReserveCount = 2;

// A harder game recovers faster: elapsed ms / 13500 over the difficulty gate.
// FUNCTION: HYPERX 0x00428920
void GameEngine::SubstituteRecoveredReserve(unsigned int nTeam)
{
    TeamBlock* pTeam;
    int        i;

    if (nTeam != 0)
    {
        pTeam = g_pGameEngine->m_pTeamB;
    }
    else
    {
        pTeam = g_pGameEngine->m_pTeamA;
    }

    for (i = 0; i < kReserveCount; i++)
    {
        if (g_pGameEngine->IsFitSubstitute(nTeam + i * 2, i) != 0)
        {
            auto flLevel = pTeam->aRows[i].aStat[kRatingDamage];

            if (pTeam->aReserveSince[i] != 0)
            {
                flLevel = static_cast<float>(g_pGameEngine->m_clock.nTickMs - pTeam->aReserveSince[i]) / (13500.0f / DamageGate()) + flLevel;
            }

            if (flLevel > pTeam->aRows[i].aStat[kRatingSubInDamage])
            {
                g_pGameEngine->m_ppSlots[static_cast<short>(nTeam + i * 2)]
                    ->PostSceneEvent(kMsgSubstituteDromer, i);
                return;
            }
        }
    }
}

#define SLOT_FAR_HALF 2

// A twentieth, against the fifth 0x00463ef0 adds.
#define NEAR_SLOT_BOOST 0.05f

// Near-half slots get a twentieth; the "limp" threshold at 0x004b1408 is a plain speed test here.
// FUNCTION: HYPERX 0x00428a40
int GameEngine::SelectSubstitute(EngineObject* pActor)
{
    short            sIndex;
    int              nWhich;
    const TeamBlock* pTeam;
    float            flBoost;

    const int nSlot = pActor->GetSlot();

    short sCandidate  = kSlotNone;
    short sChallenger = kSlotNone;

    for (sIndex = 0, nWhich = 0; sIndex < kPlayerSlotCount; sIndex++, nWhich++)
    {
        if (g_pGameEngine->IsFitSubstitute(nSlot, nWhich) != 0)
        {
            if (sCandidate == kSlotNone)
            {
                sCandidate = sIndex;
            }
            else
            {
                sChallenger = sIndex;
            }
        }
    }

    if (nSlot % 2 != 0)
    {
        pTeam = g_pGameEngine->m_pTeamB;
    }
    else
    {
        pTeam = g_pGameEngine->m_pTeamA;
    }

    const int nFirst = sCandidate;

    const auto sCount = static_cast<short>(pTeam->aSeat[(nSlot >> 1) & 1]);

    const int nSecond = sChallenger;

    auto       flFirst          = pTeam->aRows[nFirst].aStat[kRatingDamage];
    auto       flSecond         = pTeam->aRows[nSecond].aStat[kRatingDamage];
    const auto flFirstMinSubIn  = pTeam->aRows[nFirst].aStat[kRatingMinSubIn];
    const auto flSecondMinSubIn = pTeam->aRows[nSecond].aStat[kRatingMinSubIn];
    const auto flFirstSubOut    = pTeam->aRows[nFirst].aStat[kRatingSubOutDamage];
    // The second candidate's limit reads the first candidate's row, as in the original.
    const auto flSecondSubOut = pTeam->aRows[nFirst].aStat[kRatingSubOutDamage];

    if (pTeam->aReserveSince[nFirst] != 0)
    {
        const auto flGate = DamageGate();

        flFirst = static_cast<float>(g_pGameEngine->m_clock.nTickMs - pTeam->aReserveSince[nFirst]) / (13500.0f / flGate) + flFirst;
    }

    if (pTeam->aReserveSince[nSecond] != 0)
    {
        flSecond = static_cast<float>(g_pGameEngine->m_clock.nTickMs - pTeam->aReserveSince[nSecond]) / (13500.0f / DamageGate()) + flSecond;
    }

    if (sCandidate == kSlotNone)
    {
        return kSlotNone;
    }

    if (sChallenger == kSlotNone)
    {
        if ((sCount >= SLOT_FAR_HALF || sCandidate < SLOT_FAR_HALF) && (flFirst <= flFirstMinSubIn || flFirst <= flFirstSubOut))
        {
            return kSlotNone;
        }

        return nFirst;
    }

    if (sCount < SLOT_FAR_HALF)
    {
        if (sCandidate >= SLOT_FAR_HALF && flFirst > flSecond)
        {
            return nFirst;
        }

        return nSecond;
    }

    if (flSecond <= flSecondMinSubIn || flSecond <= flSecondSubOut)
    {
        sChallenger = kSlotNone;
    }

    if (flFirst <= flFirstMinSubIn || flFirst <= flFirstSubOut)
    {
        sCandidate  = sChallenger;
        sChallenger = kSlotNone;
    }

    if (sCandidate == kSlotNone)
    {
        return kSlotNone;
    }

    if (sChallenger == kSlotNone)
    {
        return sCandidate;
    }

    if (sCandidate < SLOT_FAR_HALF)
    {
        if (sChallenger >= SLOT_FAR_HALF && flFirst < 30.0f)
        {
            return sChallenger;
        }

        flBoost = flFirst;
        flFirst = flBoost * NEAR_SLOT_BOOST + flFirst;
    }

    if (sChallenger < SLOT_FAR_HALF)
    {
        flBoost  = flSecond;
        flSecond = flBoost * NEAR_SLOT_BOOST + flSecond;
    }

    if (flFirst >= flSecond)
    {
        return sCandidate;
    }

    return sChallenger;
}

// The six conditions with a body run in the order 0, 3, 4, 1, 9, 10; case ten discards a strcmp.
// FUNCTION: HYPERX 0x00428d80
void EngineObject::CoachUpdate(EngineObject* pActor)
{
    int        nSlot;
    AttachPak* pHeld;
    char       szState[128];
    char       szRole[128];

    short nHeld   = -1;
    short nGoalie = kSlotSwarf;
    auto  nCount  = 0;

    if (pActor->GetPhase() == OBJECT_PHASE_NETWORK)
    {
        return;
    }

    auto* const pszName = pActor->GetPlanState()->m_pszName;

    if (SDL_strcasecmp(pszName,
                       // STRING: HYPERX 0x004bb340
                       "Human") == 0)
    {
        return;
    }

    EngineObject** const ppEngineSlots = g_pGameEngine->m_ppSlots;
    auto* const          pSwarf        = ppEngineSlots[kSlotSwarf];
    auto* const          pRok          = ppEngineSlots[kSlotRok];

    if (pSwarf != nullptr)
    {
        pHeld = pSwarf->GetPhysicsBody()->pAttach;

        if (pHeld != nullptr && pHeld->nSlot < kPlayerObjectCount)
        {
            nGoalie = pHeld->nSlot;
        }
    }

    pHeld = pRok->GetPhysicsBody()->pAttach;

    if (pHeld != nullptr && pHeld->nSlot < kPlayerObjectCount)
    {
        nHeld = pHeld->nSlot;
    }

    nSlot                  = pActor->GetSlot();
    const int  nHeldSlot   = nHeld;
    const auto nSelfParity = nSlot & 1;
    const auto nHeldParity = nHeldSlot & 1;
    const auto nOpposite   = nSelfParity ^ 1;

    strcpy(szState, pActor->GetPlanState()->m_pCurrent->m_pszName);

    strcpy(szRole, pActor->GetPlanState()->m_pszName);

    for (int nCondition = 0; nCondition < kCoachConditions; nCondition++)
    {
        if (g_anCoachConditions[nCondition] != 0)
        {
            switch (nCondition)
            {
                case 0:
                    if (nSlot <= 3 && EngineObject::IsRokCarrier(pActor) != 0
                        // STRING: HYPERX 0x004bcb94
                        && strcmp(szState, "RunThroughMultiplier") != 0
                        // STRING: HYPERX 0x004bcb80
                        && strcmp(szState, "CompleatAnimation") != 0)
                    {
                        const auto nMarker = GameEngine::FindNearestLiveMultiplier(pActor);

                        if (nMarker != kModelIDNone)
                        {
                            pActor->PostSceneEvent(kMsgWatchOutMultiplier, nMarker);
                        }
                    }
                    break;

                case 3:
                    if (nSlot <= 3 && nGoalie != -1 && nGoalie == nSlot)
                    {
                        pActor->PostSceneEvent(kMsgTest, 0);
                        pActor->PostSceneEvent(kMsgAttack, 0);
                    }
                    break;

                case 4:
                    if (pSwarf != nullptr && nSlot <= 3 && strcmp(pRok->GetPlanState()->m_pCurrent->m_pszName,
                                                                  // STRING: HYPERX 0x004bcb74
                                                                  "SitInGoal") != 0 &&
                        strcmp(pRok->GetPlanState()->m_pCurrent->m_pszName,
                               // STRING: HYPERX 0x004bcb64
                               "SitInStartgun") != 0
                        // STRING: HYPERX 0x004bcb50
                        && strcmp(szState, "RunThroughArmory") != 0
                        // STRING: HYPERX 0x004bcb40
                        && strcmp(szState, "GettingThere") != 0
                        // STRING: HYPERX 0x004bcb30
                        && strcmp(szState, "SkateToFaceOff") != 0 && strcmp(szState, "CompleatAnimation") != 0 && nHeld != -1 && nGoalie != 0 && nGoalie != 1 && nGoalie != 2 && nGoalie != 3 && (nHeldParity ^ nSelfParity))
                    {
                        const Vec3*    pHeldPos;
                        const Vec3*    pGoaliePos;
                        const Vec3*    pOppositePos;
                        Vec3*          pSelfPos;
                        Vec3*          pGoalPos;
                        EngineObject*  pGoal;
                        EngineObject** ppSlots;
                        double         dDistance;

                        pHeldPos = &g_pGameEngine->m_ppSlots[nHeldSlot]->GetPhysicsBody()->pos;

                        pGoaliePos = &g_pGameEngine->m_ppSlots[nGoalie]->GetPhysicsBody()->pos;

                        pOppositePos = &g_pGameEngine->m_ppSlots[nHeldSlot ^ 2]->GetPhysicsBody()->pos;

                        pSelfPos = &pActor->GetPhysicsBody()->pos;

                        ppSlots = g_pGameEngine->m_ppSlots;

                        if (nHeld & 1)
                        {
                            pGoal = ppSlots[kSlotNgoal];
                        }
                        else
                        {
                            pGoal = ppSlots[kSlotSgoal];
                        }

                        pGoalPos = &pGoal->GetPhysicsBody()->pos;

                        dDistance = Math::Vec3Distance(pSelfPos, pGoaliePos);

                        if (dDistance <= ((PlayerObject*)pActor)->m_stats.GetScalar(kRatingArmoryRange))
                        {
                            dDistance = Math::Vec3Distance(pHeldPos, pSelfPos);

                            if (Math::Vec3Distance(pHeldPos, pOppositePos) <= dDistance)
                            {
                                dDistance = Math::Vec3Distance(pHeldPos, pGoalPos);

                                if (dDistance <= Math::Vec3Distance(pSelfPos, pGoalPos))
                                {
                                    dDistance = Math::Vec3Distance(pSelfPos,
                                                                   pGoaliePos);

                                    if (dDistance < Math::Vec3Distance(pSelfPos, pHeldPos))
                                    {
                                        pActor->PostSceneEvent(kMsgWatchOutArmory, nGoalie);
                                    }
                                }
                            }
                        }
                    }
                    break;

                case 1:
                    if (nSlot <= 3)
                    {
                        float         flDamage;
                        PlayerObject* pOther;

                        pOther = g_pGameEngine->GetPlayer(static_cast<short>(nOpposite));

                        flDamage = pOther->m_stats.GetScalar(kRatingDamage);

                        if (flDamage <= DamageGate())
                        {
                            nCount++;
                        }

                        switch (nCount)
                        {
                            case 1:
                                if (EngineObject::IsRokCarrier(pActor) == 0)
                                {
                                    // STRING: HYPERX 0x004bb38c
                                    if (strcmp(szRole, "OffWing") == 0)
                                    {
                                        pActor->PostSceneEvent(kMsgTest, 0);
                                    }
                                    // STRING: HYPERX 0x004bb384
                                    else if (strcmp(szRole, "Defense") == 0)
                                    {
                                        pActor->PostSceneEvent(kMsgTest, 0);
                                    }
                                }
                                break;
                        }
                    }
                    break;

                case 9:
                    if (nSlot == kGoalieSlotA || nSlot == kGoalieSlotB)
                    {
                        // STRING: HYPERX 0x004bc4e4
                        if (strcmp(szState, "Attack") == 0
                            // STRING: HYPERX 0x004bcb20
                            || strcmp(szState, "SecondaryAttack") == 0
                            // STRING: HYPERX 0x004bcb0c
                            || strcmp(szState, "SecondaryLeadAttack") == 0
                            // STRING: HYPERX 0x004bcaf4
                            || strcmp(szState, "SecondaryOtherAttack") == 0)
                        {
                            pActor->PostSceneEvent(kMsgTimeOut, 0);
                        }
                    }
                    break;

                case 10:
                    if (nSlot < 4 && nSelfParity != nHeldParity)
                    {
                    }
                    break;
            }
        }
    }
}

// Thirteen match conditions, six coach updates, six substitution checks, then two goal notifications.

// FUNCTION: HYPERX 0x00429620
void EngineObject::UpdateCoachAI()
{
    int nSlot;

    for (int nCondition = 0; nCondition < kCoachConditions; nCondition++)
    {
        g_anCoachConditions[nCondition] = g_apfnCoachConditions[nCondition]();
    }

    for (nSlot = 0; nSlot <= kCoachSlotLast; nSlot++)
    {
        EngineObject::CoachUpdate(g_pGameEngine->m_ppSlots[static_cast<short>(nSlot)]);
    }

    for (nSlot = 0; nSlot <= kCoachSlotLast; nSlot++)
    {
        auto* const pActor = g_pGameEngine->m_ppSlots[static_cast<short>(nSlot)];

        if (GameEngine::IsComputerTeam(nSlot) != 0)
        {
            auto* const pObject = (PlayerObject*)pActor;
            float       flLimit;

            flLimit = pObject->m_stats.GetScalar(kRatingSubOutDamage);

            if (pObject->m_stats.GetScalar(kRatingDamage) < flLimit)
            {
                const auto nTarget = GameEngine::SelectSubstitute(pActor);

                if (nTarget != -1)
                {
                    auto* const pszName = pActor->GetPlanState()->m_pszName;

                    // STRING: HYPERX 0x004bb37c
                    if (strcmp(pszName, "Sideln") != 0)
                    {
                        pActor->PostSceneEvent(kMsgSubstituteDromer, nTarget);
                    }
                }
            }
        }
    }

    if (GameEngine::IsComputerTeam(kTeamHome) != 0)
    {
        GameEngine::SubstituteRecoveredReserve(kTeamHome);
    }

    if (GameEngine::IsComputerTeam(kTeamAway) != 0)
    {
        GameEngine::SubstituteRecoveredReserve(kTeamAway);
    }
}

// Six of the thirteen share one answer-nothing entry.
CoachConditionFn g_apfnCoachConditions[13] = {
    GameEngine::CheckHandicapCondition,
    GameEngine::IsAnyPlayerInjured,
    GameEngine::CoachConditionNever,
    GameEngine::CheckOpposedCarriersRace,
    GameEngine::CheckWeaponOutWhileRokCarried,
    GameEngine::CoachConditionNever,
    GameEngine::CoachConditionNever,
    GameEngine::CoachConditionNever,
    GameEngine::CoachConditionNever,
    GameEngine::CheckGoalieOutOfPosition,
    GameEngine::CheckCarrierBreakaway,
    GameEngine::CoachConditionUnused,
    GameEngine::CheckAnyPlayerNeedsSub
};
