#include "common/compat.h"
#include "engine/devicector.h"
#include "engine/objectclasses.h"
#include "planvm/agendaitem.h"
#include "planvm/cupdateshotpower.h"
#include "engine/objectstats.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "planvm.h"
#include "engine/cameraobject.h"
#include "engine/playerobject.h"
#include "engine/engineobject.h"
#include "common/pooledownerroot.h"
#include "engine/statids.h"

// Raises the head camera's m_nShotPowerLevel.

// FUNCTION: HYPERX 0x004123f0
void CUpdateShotPower::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_bReady = 0;
    m_wKind  = Agenda_CUpdateShotPower;
    m_pOwner = pSubject;

    m_nStarted = g_pGameEngine->m_clock.nTickMs;

    g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT)->m_nShotPowerLevel = 1;

    EngineObject* pShooter = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

    m_flPower = ((PlayerObject*)pShooter)->m_stats.GetScalar(RATING_SHOT_POWER);

    pShooter = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

    m_pOrigin = pShooter->GetPositionPtr();
}

#define SHOT_POWER_MAX 100

// FUNCTION: HYPERX 0x00412470
void CUpdateShotPower::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner   = nullptr;
        m_nStarted = 0;
        m_flPower  = 0;
        m_pOrigin  = nullptr;
        m_pNext    = nullptr;
        m_bReady   = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// The fog ramp is only ever raised here; the head camera's m_bShotPowerShown stops the whole block.
// FUNCTION: HYPERX 0x004124a0
void CUpdateShotPower::Process(int nArg)
{
    int nAmount;

    CameraObject* const pCamera = g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT);

    if (pCamera->m_bShotPowerShown != 0)
    {
        nAmount = (int)((float)(g_pGameEngine->m_clock.nTickMs - m_nStarted) * (m_flPower + 12.0f) * 0.004f);

        if (nAmount > SHOT_POWER_MAX)
        {
            nAmount = SHOT_POWER_MAX;
        }

        pCamera->m_nShotPowerLevel = nAmount;

        pCamera->SetReticleState(RETICLE_COMMIT);
    }

    AgendaItem* const pNext = m_pNext;

    if (pNext != nullptr && m_bReady == 0)
    {
        pNext->Process(nArg);
    }
}
