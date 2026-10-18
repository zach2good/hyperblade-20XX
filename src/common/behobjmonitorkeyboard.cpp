#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "engine/objectclasses.h"
#include "common/pooledclasses.h"
#include "engine/engineobject.h"
#include "engine/devicector.h"
#include "globals.h"
#include "engine/gameengine.h"
#include "input/keyboarddeviceobj.h"
#include "pooledownerroot.h"
#include "engine/objectphase.h"

// The button the rok throw comes in on.
constexpr auto BUTTON_ROK = 4;

// Never written in this image; its three readers treat it as a two-player flag.
// GLOBAL: HYPERX 0x004cb5e4
extern int g_bTwoPlayers;

// FUNCTION: HYPERX 0x0041def0
BehObj_MonitorKeyboard::BehObj_MonitorKeyboard() = default;

// FUNCTION: HYPERX 0x0041df40
BehObj_MonitorKeyboard::~BehObj_MonitorKeyboard() = default;

// FUNCTION: HYPERX 0x0041df50
void BehObj_MonitorKeyboard::SendTimedMessage()
{
    int nSlot;

    GameBehaviorObject::SendTimedMessage();

    if (m_pOwner->GetPhase() == OBJECT_PHASE_LOCAL)
    {
        const short nOwnerSlot = m_pOwner->GetSlot();

        g_pGameEngine->GetInputDevice(SLOT_KEYBOARD)->OnButtonDown(BUTTON_ROK);

        if (g_bTwoPlayers != 0)
        {
            g_pGameEngine->m_nViewedSlot = nOwnerSlot;

            if ((nOwnerSlot & 1) != 0)
            {
                nSlot = g_nJoystickSlotB;
            }
            else
            {
                nSlot = g_nJoystickSlotA;
            }

            g_pGameEngine->m_apTracked[nSlot] = g_pGameEngine->m_ppSlots[nOwnerSlot];

            return;
        }

        g_pGameEngine->m_nViewedSlot = nOwnerSlot;

        g_pGameEngine->m_apTracked[0] = g_pGameEngine->m_ppSlots[nOwnerSlot];
        g_pGameEngine->m_apTracked[1] = g_pGameEngine->m_ppSlots[nOwnerSlot];

        g_pGameEngine->m_bFollowingOther = 0;
    }
}

// FUNCTION: HYPERX 0x0041e010
void BehObj_MonitorKeyboard::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc314
    m_pszName = "MonitorKeyboard";

    m_wKind = BEH_MONITOR_KEYBOARD;

    m_apSlots[0] = AllocAgendaItem(Agenda_CMonitorKeys);

    short nSlot  = 0;
    m_apSlots[1] = nullptr;

    m_pOwner = pOwner;
    m_pBody  = pOwner->GetPhysicsBody();

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}
