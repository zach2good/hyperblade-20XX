#include "common/compat.h"
#include "common/clock.h"
#include "engine/messageids.h"
#include "physics/physicsbody.h"
#include "devicector.h"
#include "physics/boundbubble.h"
#include "physics/collisioncage.h"
#include "gameengine.h"
#include "globals.h"
#include "planvm/planvm.h"
#include "net/session.h"
#include "common/pooledclasses.h"
#include "objectclasses.h"
#include "graphics/meshbone.h"
#include "math/transformobject.h"
#include <cstring>
#include <cstdlib>
#include "rokobject.h"
#include "net/cplayer.h"
#include "app/gameapp.h"
#include "ui/hudglobals.h"
#include "engine/engineobject.h"
#include "engine/playerobject.h"
#include "engine/teamsblock.h"
#include "engine/objectphase.h"
#include "engine/possessionsyncword.h"
#include "engine/systemsyncword.h"
#include "common/staticinit.h"

extern const float g_flZeroRokObject    = 0.0f;
float              g_flUnusedRokObjectA = g_flZeroRokObject;
float              g_flUnusedRokObjectB = g_flZeroRokObject;
float              g_flUnusedRokObjectC = g_flZeroRokObject;

// Every mesh bone past the first is hidden.
// FUNCTION: HYPERX 0x00473a80
RokObject::RokObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    m_wRokValue           = 0;
    m_nKillState          = -1;
    m_nCarrierSlot        = 0;
    m_nPrevCarrierSlot    = 0;
    m_nOddCarrierSlot     = 1;
    m_nOddPrevCarrierSlot = 1;

    m_nFlags |= kEngineObjectNoCull;

    for (int nBone = 1; nBone < m_wBoneCount; nBone++)
    {
        m_pBones[nBone].SetFlags(kMeshboneHidden);
    }

    m_bAtSpawnSlot = 0;
}

// The scratch position the loop fills and hands to the slot's own transform.
// GLOBAL: HYPERX 0x004d06d8
extern float g_flRokBoneShiftX;
// GLOBAL: HYPERX 0x004d06dc
extern float g_flRokBoneShiftY;
// GLOBAL: HYPERX 0x004d06e0
extern float g_flRokBoneShiftZ;

// FUNCTION: HYPERX 0x00473b80
RokObject::~RokObject() = default;

// Every slot is hidden, then one is shown; an uncovered multiplier leaves everything hidden.
// FUNCTION: HYPERX 0x00473b90
void RokObject::SetRenderSlotFlags()
{
    int nSlot;

    const int nCount = m_wBoneCount;

    for (nSlot = 0; nSlot < nCount; nSlot++)
    {
        m_pBones[static_cast<short>(nSlot)].SetFlags(kMeshboneHidden);
    }

    if (m_bAtSpawnSlot != 0)
    {
        nSlot = 0;
    }
    else if (m_nKillState == -1)
    {
        switch (abs(m_wRokValue))
        {
            case 0:
            case 1:
                nSlot = 0;
                break;

            case 2:
                nSlot = 7;
                break;

            case 4:
                nSlot = 8;
                break;

            default:
                return;
        }
    }
    else
    {
        nSlot = 2;
    }

    if (nSlot < nCount)
    {
        m_pBones[static_cast<short>(nSlot)].ClearFlags(kMeshboneHidden);
    }
}

// FUNCTION: HYPERX 0x00473c50
void RokObject::ActivateAtSpawnSlot(unsigned int nSlot)
{
    ObjectDef* pDef;

    if (nSlot == kSpawnSlotAny)
    {
        pDef = g_pGameEngine->GetArenaRokDef();
    }
    else
    {
        pDef = g_pGameEngine->GetTeamRokDef(nSlot);
    }

    if (pDef != nullptr)
    {
        g_pGameEngine->ReplaceEntity(kSlotRok, pDef);

        ApplyRenderSlotPose();

        if (nSlot == kSpawnSlotAny)
        {
            m_bAtSpawnSlot = 0;

            SetRenderSlotFlags();
            return;
        }

        m_bAtSpawnSlot = 1;

        SetRenderSlotFlags();
    }
}

constexpr auto kBotSendInterval = Clock::Milliseconds{ 0x1f4 };
constexpr auto kBotSlotLimit    = 7;

// Sent at most every 500 ms; the phase compare reads two bytes that were never written.
// FUNCTION: HYPERX 0x00473cc0
void RokObject::Postpass()
{
    int              nMessage;
    int              nIndex;
    const TeamBlock* pTeam;
    float            flFirst;
    float            flSecond;

    if (g_bSessionActive == 0)
    {
        return;
    }

    if (g_pGameEngine == nullptr)
    {
        return;
    }

    if (GetPhase() != ObjectPhase::kLocal)
    {
        return;
    }

    if (m_lastSend + kBotSendInterval >= g_pGameEngine->m_clock.tick)
    {
        return;
    }

    auto nPayload = 0;

    if (m_sendCursor.GetPossessionTurn() == 0)
    {
        nMessage = kMsgSyncSystem;

        if (g_pGameEngine->m_clock.nPauseMark == 1)
        {
            nPayload = kSystemSyncPaused;
        }

        nIndex = static_cast<int>((g_pGameEngine->m_clock.pauseStart - g_pGameEngine->m_clock.periodStart).count() / 100) & 0x7fff;

        nPayload |= SystemSyncWord::Pack(nIndex, g_pGameEngine->GetHomeScore(), g_pGameEngine->GetAwayScore());

        m_sendCursor.SetPossessionTurn();
    }
    else
    {
        nMessage = kMsgSyncPossession;

        auto* const pCarrier = GetPhysicsBody()->pAttach;

        if (pCarrier != nullptr && pCarrier->nSlot < kPlayerObjectCount)
        {
            if (pCarrier->nSlot <= kSlotDromer3)
            {
                nPayload = kPossessionSyncDromerCarrier;
            }
        }

        nIndex = m_sendCursor.GetRow();

        if (nIndex > kBotSlotLimit)
        {
            nIndex = 0;
        }

        const auto nTeam = nIndex / kTeamRowCount;

        if (nTeam == 0)
        {
            pTeam = g_pGameEngine->m_pTeamA.get();
        }
        else
        {
            pTeam = g_pGameEngine->m_pTeamB.get();
        }

        const auto nRow = nIndex % kTeamRowCount;

        nPayload |= PossessionSyncWord::PackTeam(nTeam);

        if (nRow == pTeam->aSeat[0])
        {
            auto* const pOwner = (PlayerObject*)g_pGameEngine->m_ppSlots[static_cast<short>(nTeam)];

            flFirst  = pOwner->m_matchStats.GetAt(kMatchStatTimeOfPoss);
            flSecond = pOwner->m_matchStats.GetAt(kMatchStatShotsOnGoal);
        }
        else if (nRow == pTeam->aSeat[1])
        {
            auto* const pOwner = (PlayerObject*)
                                     g_pGameEngine->m_ppSlots[static_cast<short>(nTeam + 2)];

            flFirst  = pOwner->m_matchStats.GetAt(kMatchStatTimeOfPoss);
            flSecond = pOwner->m_matchStats.GetAt(kMatchStatShotsOnGoal);
        }
        else
        {
            flFirst  = pTeam->aStatRows[nRow].aStat[kMatchStatTimeOfPoss];
            flSecond = pTeam->aStatRows[nRow].aStat[kMatchStatShotsOnGoal];
        }

        nPayload |= PossessionSyncWord::Pack(nRow, static_cast<int>(flSecond), static_cast<unsigned int>(static_cast<int>(flFirst)) / 100);

        m_sendCursor.SetRow(nRow + nTeam * 4 + 1);
    }

    PostSceneMessage(nMessage, nPayload, GetSlot(), 0);

    m_lastSend = g_pGameEngine->m_clock.tick;
}

// The scratch transform is constructed on the first call and never read.
// FUNCTION: HYPERX 0x00473f40
void RokObject::ApplyRenderSlotPose()
{
    static TransformObject s_scratch;

    m_nFlags |= kEngineObjectPosed;

    if (m_bScaledBounds != 0)
    {
        g_flRokBoneShiftX = m_transformC.GetScaleX() * m_pBody->surfNormal.x * m_flBoundRadius + m_pBody->pos.x;
        g_flRokBoneShiftY = m_transformC.GetScaleY() * m_pBody->surfNormal.y * m_flBoundRadius + m_pBody->pos.y;
        g_flRokBoneShiftZ = m_transformC.GetScaleZ() * m_pBody->surfNormal.z * m_flBoundRadius + m_pBody->pos.z;
    }
    else
    {
        g_flRokBoneShiftX = m_pBody->surfNormal.x * m_flBoundRadius + m_pBody->pos.x;
        g_flRokBoneShiftY = m_pBody->surfNormal.y * m_flBoundRadius + m_pBody->pos.y;
        g_flRokBoneShiftZ = m_pBody->surfNormal.z * m_flBoundRadius + m_pBody->pos.z;
    }

    m_transformB.Multiply(&m_transformC, &m_transformA);

    for (int i = 0; i < m_wBoneCount; i++)
    {
        auto* const pSlot = &m_pBones[i];

        if (pSlot->IsHidden() == 0)
        {
            const auto flAlong = pSlot->m_flRadius;

            if (m_bScaledBounds != 0)
            {
                g_flRokBoneShiftX = pSlot->m_world.GetScaleX() * m_pBody->surfNormal.x * flAlong + m_pBody->pos.x;
                g_flRokBoneShiftY = pSlot->m_world.GetScaleY() * m_pBody->surfNormal.y * flAlong + m_pBody->pos.y;
                g_flRokBoneShiftZ = pSlot->m_world.GetScaleZ() * m_pBody->surfNormal.z * flAlong + m_pBody->pos.z;
            }
            else
            {
                g_flRokBoneShiftX = flAlong * m_pBody->surfNormal.x + m_pBody->pos.x;
                g_flRokBoneShiftY = flAlong * m_pBody->surfNormal.y + m_pBody->pos.y;
                g_flRokBoneShiftZ = flAlong * m_pBody->surfNormal.z + m_pBody->pos.z;
            }

            m_transformC.AddTranslation(g_flRokBoneShiftX, g_flRokBoneShiftY, g_flRokBoneShiftZ);

            pSlot->UpdateLeafTransform(static_cast<unsigned int>(i), &m_transformC);
        }
    }
}
