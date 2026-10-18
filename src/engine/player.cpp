#include "common/compat.h"
#include "engine/objectclasses.h"
#include "gameengine.h"
#include "physics/physicsbody.h"
#include "globals.h"
#include "devicector.h"
#include "math/vector.h"
#include "planvm/damagegate.h"
#include <cmath>
#include "engine/engineobject.h"
#include "engine/player.h"
#include "engine/messageids.h"

constexpr auto kTrickHoldMs = 100;

// The trick and boost-grab steps, and the tick the hold started at; minus one means untimed.
// GLOBAL: HYPERX 0x004cb4da
extern short g_nTrickSeqStep;
// GLOBAL: HYPERX 0x004cb4dc
extern short g_nBoostGrabStep;
// GLOBAL: HYPERX 0x004bb668
extern int g_nTrickHoldStamp;

// The forward key picks trick or boost-grab; a hundred milliseconds of quiet fires the neutral trick.
// FUNCTION: HYPERX 0x004110f0
void Player::ProcessTrickInput(EngineObject* pPlayer, short* pnInput)
{
    if (pPlayer->GetPhysicsBody()->flFriction >= 1.0)
    {
        pPlayer->GetPhysicsBody()->UpdateBanking();
    }

    pPlayer->GetPhysicsBody()->UpdateMovementAxes();

    if ((*pnInput & HELD_FORWARD) == 0)
    {
        if (((*pnInput & HELD_MOVEMENT) != 0) || (g_nTrickSeqStep != 0) || (pPlayer->GetPhysicsBody()->flSteerScale > 1.0001))
        {
            const short          nStep  = g_nTrickSeqStep;
            const unsigned short nInput = *pnInput;

            if (((nInput & HELD_STOP) == 0) && ((nInput & HELD_ROTATE) != 0))
            {
                if (nStep == 0)
                {
                    g_nTrickSeqStep  = 1;
                    g_nBoostGrabStep = 0;

                    if ((*pnInput & HELD_ROTATE_LEFT) != 0)
                    {
                        pPlayer->PostSceneEvent(kMsgNoInputForSec, 1);

                        return;
                    }

                    pPlayer->PostSceneEvent(kMsgNoInputForSec, 2);

                    return;
                }
            }
            else if ((nStep == 0) && ((nInput & HELD_ROTATE) != 0))
            {
                g_nTrickSeqStep  = 1;
                g_nBoostGrabStep = 0;

                if ((*pnInput & HELD_ROTATE_LEFT) != 0)
                {
                    pPlayer->PostSceneEvent(kMsgNoInputForSec, 3);

                    return;
                }

                pPlayer->PostSceneEvent(kMsgNoInputForSec, 4);

                return;
            }

            g_nTrickSeqStep  = static_cast<short>((nStep + 1) % 8);
            g_nBoostGrabStep = 0;

            return;
        }

        if (g_nTrickHoldStamp < 0)
        {
            g_nTrickHoldStamp = g_pGameEngine->m_clock.nTickMs;

            return;
        }

        if (g_pGameEngine->m_clock.nTickMs - static_cast<unsigned int>(g_nTrickHoldStamp) > kTrickHoldMs)
        {
            g_nTrickSeqStep   = 1;
            g_nTrickHoldStamp = -1;
            g_nBoostGrabStep  = 0;

            pPlayer->PostSceneEvent(kMsgNoInputForSec, 0);

            return;
        }
    }
    else
    {
        const short nGrab = g_nBoostGrabStep;

        if (nGrab == 0)
        {
            pPlayer->PostSceneEvent(kMsgNewInput, 0);

            g_nBoostGrabStep = 1;

            g_pGameEngine->m_ppSlots[kSlotKeyboard]->PostSceneEvent(kMsgDragOver, 0);

            return;
        }

        g_nBoostGrabStep = static_cast<short>((nGrab + 1) % 8);
        g_nTrickSeqStep  = 0;
    }
}

// Four.
extern const float g_flFourPlayer;

// GLOBAL: HYPERX 0x004b17b4
extern const float g_flPassAngleStep;

// The two reaction-table steps, published for src/planvm/planactionrecoil.cpp.
extern short g_nCheckImpulseStep;
extern short g_nCheckImpulseFacing;

// The opponents one and three slots on; a lead under -4 or a difference over 18 finds none.
// FUNCTION: HYPERX 0x00411550
EngineObject* Player::FindBungeeVictim(EngineObject* pActor, float* pflOut)
{
    EngineObject* pTarget;
    float         flDxA = 0.0f;
    float         flDyB = 0.0f;
    float         flDxB = 0.0f;
    float         flCross;
    float         flDet;
    short         nFacing;
    short         nStep;

    auto                nSlot  = static_cast<short>((pActor->GetSlot() + 1) % 4);
    EngineObject* const pCandA = g_pGameEngine->m_ppSlots[nSlot];
    nSlot                      = static_cast<short>((pActor->GetSlot() + 3) % 4);
    EngineObject* const pCandB = g_pGameEngine->m_ppSlots[nSlot];

    const auto flDyA = static_cast<float>(fabs(static_cast<double>(pCandA->GetPositionPtr()->y - pActor->GetPositionPtr()->y)));

    if (flDyA > 30.0f)
    {
        flDyB = static_cast<float>(fabs(static_cast<double>(pCandB->GetPositionPtr()->y - pActor->GetPositionPtr()->y)));

        if (flDyB > 30.0f)
        {
            return nullptr;
        }

        flDxB = static_cast<float>(fabs(static_cast<double>(pCandB->GetPositionPtr()->x - pActor->GetPositionPtr()->x)));

        if (flDxB + flDyB > 30.0f)
        {
            return nullptr;
        }

        if (pActor->GetDistIfFacingDefault(pCandB) == 0.0f)
        {
            return nullptr;
        }

        pTarget = pCandB;
    }
    else
    {
        flDxA = static_cast<float>(fabs(static_cast<double>(pCandA->GetPositionPtr()->x - pActor->GetPositionPtr()->x)));

        if (flDxA + flDyA > 30.0f)
        {
            flDyB = static_cast<float>(fabs(static_cast<double>(pCandB->GetPositionPtr()->y - pActor->GetPositionPtr()->y)));

            if (flDyB > 30.0f)
            {
                return nullptr;
            }

            flDxB = static_cast<float>(fabs(static_cast<double>(pCandB->GetPositionPtr()->x - pActor->GetPositionPtr()->x)));

            if (flDxB + flDyB > 30.0f)
            {
                return nullptr;
            }

            if (flDxA + flDyA < flDxB + flDyB)
            {
                if (pActor->GetDistIfFacingDefault(pCandA) == 0.0f)
                {
                    return nullptr;
                }

                pTarget = pCandA;
            }
            else
            {
                if (pActor->GetDistIfFacingDefault(pCandB) == 0.0f)
                {
                    return nullptr;
                }

                pTarget = pCandB;
            }
        }
        else
        {
            if (pActor->GetDistIfFacingDefault(pCandA) == 0.0f)
            {
                return nullptr;
            }

            pTarget = pCandA;
        }
    }

    const float flSelfGrad   = pActor->GetPhysicsBody()->velDir.y / pActor->GetPhysicsBody()->velDir.x;
    const float flTargetGrad = pTarget->GetPhysicsBody()->velDir.y / pTarget->GetPhysicsBody()->velDir.x;

    const float flSelfBase = pActor->GetPositionPtr()->y - flSelfGrad * pActor->GetPositionPtr()->x;
    const float flTargetY  = pTarget->GetPositionPtr()->y;
    flCross                = ((flTargetY - flTargetGrad * pTarget->GetPositionPtr()->x) - flSelfBase) / (flSelfGrad - flTargetGrad) * flSelfGrad + flSelfBase;

    const PhysicsBody* pSelfBodyA   = pActor->GetPhysicsBody();
    const PhysicsBody* pTargetBodyA = pTarget->GetPhysicsBody();
    const PhysicsBody* pSelfBodyB   = pActor->GetPhysicsBody();
    const PhysicsBody* pTargetBodyB = pTarget->GetPhysicsBody();
    flDet                           = pTargetBodyA->velDir.y * pSelfBodyA->velDir.x - pTargetBodyB->velDir.x * pSelfBodyB->velDir.y;

    const float  flLead  = (flCross - pTarget->GetPositionPtr()->y) / pTarget->m_pBody->GetVel().y;
    const double dAbsDet = fabs(static_cast<double>(flDet));

    if (flLead * dAbsDet < -4.0)
    {
        return nullptr;
    }

    const float flDiff = static_cast<float>(((flCross - pActor->GetPositionPtr()->y) / pActor->m_pBody->GetVel().y - flLead) * dAbsDet);

    if (fabs(static_cast<double>(flDiff)) > 18.0)
    {
        return nullptr;
    }

    // A negative determinant adds the offset and a non-negative one subtracts it.
    if (flDet < g_flZeroAgendaItem)
    {
        nFacing = static_cast<short>((flDiff + 9.0f) / g_flPassAngleStep);
    }
    else
    {
        nFacing = static_cast<short>((9.0f - flDiff) / g_flPassAngleStep);
    }

    g_nCheckImpulseFacing = nFacing;

    nStep = static_cast<short>(5 - static_cast<int>((pActor->m_pBody->GetVel().x * pTarget->m_pBody->GetVel().x + pActor->m_pBody->GetVel().y * pTarget->m_pBody->GetVel().y + 9.0f) / g_flFourPlayer));

    if (nStep < 0)
    {
        g_nCheckImpulseStep = 0;
    }
    else
    {
        g_nCheckImpulseStep = 4;

        if (nStep <= 4)
        {
            g_nCheckImpulseStep = nStep;
        }
    }

    if (nFacing < 0)
    {
        g_nCheckImpulseFacing = 0;
    }
    else
    {
        g_nCheckImpulseFacing = 6;

        if (nFacing <= 6)
        {
            g_nCheckImpulseFacing = nFacing;
        }
    }

    if (pTarget == pCandA)
    {
        *pflOut = static_cast<float>(sqrt(static_cast<double>(flDxA * flDxA + flDyA * flDyA)));

        return pTarget;
    }

    *pflOut = static_cast<float>(sqrt(static_cast<double>(flDxB * flDxB + flDyB * flDyB)));

    return pTarget;
}

int g_nTrickHoldStamp = -1;
