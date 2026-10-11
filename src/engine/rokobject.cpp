#include "common/compat.h"
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
#include <string.h>
#include <stdlib.h>
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

    m_nFlags |= ENGINEOBJECT_NO_CULL;

    for (int nBone = 1; nBone < m_wBoneCount; nBone++)
    {
        m_pBones[nBone].SetFlags(MESHBONE_HIDDEN);
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
RokObject::~RokObject()
{
}

// Every slot is hidden, then one is shown; an uncovered multiplier leaves everything hidden.
// FUNCTION: HYPERX 0x00473b90
void RokObject::SetRenderSlotFlags()
{
    int nSlot;

    const int nCount = m_wBoneCount;

    for (nSlot = 0; nSlot < nCount; nSlot++)
    {
        m_pBones[(short)nSlot].SetFlags(MESHBONE_HIDDEN);
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
        m_pBones[(short)nSlot].ClearFlags(MESHBONE_HIDDEN);
    }
}

// FUNCTION: HYPERX 0x00473c50
void RokObject::ActivateAtSpawnSlot(unsigned int nSlot)
{
    ObjectDef* pDef;

    if (nSlot == SPAWN_SLOT_ANY)
    {
        pDef = g_pGameEngine->GetArenaRokDef();
    }
    else
    {
        pDef = g_pGameEngine->GetTeamRokDef(nSlot);
    }

    if (pDef != 0)
    {
        g_pGameEngine->ReplaceEntity(SLOT_ROK, pDef);

        ApplyRenderSlotPose();

        if (nSlot == SPAWN_SLOT_ANY)
        {
            m_bAtSpawnSlot = 0;

            SetRenderSlotFlags();
            return;
        }

        m_bAtSpawnSlot = 1;

        SetRenderSlotFlags();
    }
}

#define BOT_SEND_INTERVAL 0x1f4
#define BOT_SLOT_LIMIT    7

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

    if (g_pGameEngine == 0)
    {
        return;
    }

    if (GetPhase() != OBJECT_PHASE_LOCAL)
    {
        return;
    }

    if ((unsigned int)(m_nLastSendMs + BOT_SEND_INTERVAL) >= g_pGameEngine->m_clock.nTickMs)
    {
        return;
    }

    int nPayload = 0;

    if (m_sendCursor.GetPossessionTurn() == 0)
    {
        nMessage = MSG_SYNC_SYSTEM;

        if (g_pGameEngine->m_clock.nPauseMark == 1)
        {
            nPayload = SYSTEM_SYNC_PAUSED;
        }

        nIndex = (int)((g_pGameEngine->m_clock.nPauseStartMs - g_pGameEngine->m_clock.nPeriodStartMs) / 100) & 0x7fff;

        nPayload |= SystemSyncWord::Pack(nIndex, g_pGameEngine->GetHomeScore(), g_pGameEngine->GetAwayScore());

        m_sendCursor.SetPossessionTurn();
    }
    else
    {
        nMessage = MSG_SYNC_POSSESSION;

        AttachPak* const pCarrier = GetPhysicsBody()->pAttach;

        if (pCarrier != 0 && pCarrier->nSlot < PLAYER_OBJECT_COUNT)
        {
            if (pCarrier->nSlot <= SLOT_DROMER3)
            {
                nPayload = POSSESSION_SYNC_DROMER_CARRIER;
            }
        }

        nIndex = m_sendCursor.GetRow();

        if (nIndex > BOT_SLOT_LIMIT)
        {
            nIndex = 0;
        }

        const int nTeam = nIndex / TEAM_ROW_COUNT;

        if (nTeam == 0)
        {
            pTeam = g_pGameEngine->m_pTeamA;
        }
        else
        {
            pTeam = g_pGameEngine->m_pTeamB;
        }

        const int nRow = nIndex % TEAM_ROW_COUNT;

        nPayload |= PossessionSyncWord::PackTeam(nTeam);

        if (nRow == pTeam->aSeat[0])
        {
            PlayerObject* pOwner = (PlayerObject*)g_pGameEngine->m_ppSlots[(short)nTeam];

            flFirst  = pOwner->m_matchStats.GetAt(MATCH_STAT_TIME_OF_POSS);
            flSecond = pOwner->m_matchStats.GetAt(MATCH_STAT_SHOTS_ON_GOAL);
        }
        else if (nRow == pTeam->aSeat[1])
        {
            PlayerObject* const pOwner = (PlayerObject*)
                                             g_pGameEngine->m_ppSlots[(short)(nTeam + 2)];

            flFirst  = pOwner->m_matchStats.GetAt(MATCH_STAT_TIME_OF_POSS);
            flSecond = pOwner->m_matchStats.GetAt(MATCH_STAT_SHOTS_ON_GOAL);
        }
        else
        {
            flFirst  = pTeam->aStatRows[nRow].aStat[MATCH_STAT_TIME_OF_POSS];
            flSecond = pTeam->aStatRows[nRow].aStat[MATCH_STAT_SHOTS_ON_GOAL];
        }

        nPayload |= PossessionSyncWord::Pack(nRow, (int)flSecond, (unsigned int)(int)flFirst / 100);

        m_sendCursor.SetRow(nRow + nTeam * 4 + 1);
    }

    PostSceneMessage(nMessage, nPayload, GetSlot(), 0);

    m_nLastSendMs = g_pGameEngine->m_clock.nTickMs;
}

// The scratch transform is constructed on the first call and never read.
// FUNCTION: HYPERX 0x00473f40
void RokObject::ApplyRenderSlotPose()
{
    static TransformObject s_scratch;

    m_nFlags |= ENGINEOBJECT_POSED;

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
        MeshBone* const pSlot = &m_pBones[i];

        if (pSlot->IsHidden() == 0)
        {
            const float flAlong = pSlot->m_flRadius;

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

            pSlot->UpdateLeafTransform((unsigned int)i, &m_transformC);
        }
    }
}
