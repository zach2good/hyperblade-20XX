// BehObj_GlideToObject, the one behaviour object that lives in planvm/.

#include "common/compat.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "engine/devicector.h"
#include "physics/physicsbody.h"
#include "common/staticinit.h"
#include "engine/objectclasses.h"
#include "input/keyboarddeviceobj.h"
#include "ui/screens/lobbyscreen.h"
#include "ui/saveload.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "ui/screens/keyconfigscreen.h"
#include "ui/screens/finalceremonyscreen.h"
#include "ui/screens/headtoheadsetupuiscreen.h"
#include "ui/screens/matchinfouiscreen.h"
#include "engine/rokobject.h"
#include "engine/playerobject.h"
#include "ui/screens/gameconfiguiscreen.h"
#include "ui/screens/tournamentsetupuiscreen.h"
#include "ui/screens/mainmenuscreen.h"
#include "ui/screens/jumbotronscreen.h"
#include "physics/collisionellipsoid.h"
#include "planvm/agendaitem.h"
#include "planvmglobals.h"
#include "engine/gameengine.h"
#include "planvm.h"
#include "collinteractor.h"
#include "engine/engineobject.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x0041d460
BehObj_GlideToObject::BehObj_GlideToObject() = default;

// FUNCTION: HYPERX 0x0041d4a0
BehObj_GlideToObject::~BehObj_GlideToObject() = default;

// FUNCTION: HYPERX 0x0041d4b0
void BehObj_GlideToObject::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForGlideToObject();

    short nSlot = 0;

    // STRING: HYPERX 0x004bc288
    m_pszName = "GlideToObject";

    m_wKind = nSlot;

    m_apSlots[0] = AllocAgendaItem(Agenda_CSteerToTarget);

    m_apSlots[1] = AllocAgendaItem(Agenda_CAdjustAcceleration);
    m_apSlots[2] = AllocAgendaItem(Agenda_CGlideOnSurface);
    m_apSlots[3] = AllocAgendaItem(Agenda_CStepAnimation);
    m_apSlots[4] = AllocAgendaItem(Agenda_CSetAttitude);
    m_apSlots[5] = nullptr;

    m_pOwner = pOwner;
    m_pBody  = pOwner->GetPhysicsBody();

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        auto* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}

// FUNCTION: HYPERX 0x0041d570
void BehObj_GlideToObject::SendTimedMessage()
{
    GameBehaviorObject::SendTimedMessage();

    m_pBody->flFacing = g_flZeroBehavior;
}
