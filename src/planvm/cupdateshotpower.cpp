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

    g_pGameEngine->GetCamera(kCameraHeadSlot)->m_nShotPowerLevel = 1;

    auto* pShooter = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

    m_flPower = ((PlayerObject*)pShooter)->m_stats.GetScalar(kRatingShotPower);

    pShooter = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

    m_pOrigin = pShooter->GetPositionPtr();
}

constexpr auto kShotPowerMax = 100;

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

    auto* const pCamera = g_pGameEngine->GetCamera(kCameraHeadSlot);

    if (pCamera->m_bShotPowerShown != 0)
    {
        nAmount = static_cast<int>(static_cast<float>(g_pGameEngine->m_clock.nTickMs - m_nStarted) * (m_flPower + 12.0f) * 0.004f);

        if (nAmount > kShotPowerMax)
        {
            nAmount = kShotPowerMax;
        }

        pCamera->m_nShotPowerLevel = nAmount;

        pCamera->SetReticleState(kReticleCommit);
    }

    auto* const pNext = m_pNext;

    if (pNext != nullptr && m_bReady == 0)
    {
        pNext->Process(nArg);
    }
}
