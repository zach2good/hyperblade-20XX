// The same routine twice, chaining through slot one or two; a dying spin still turns last frame.

#include "common/compat.h"
#include "engine/messageids.h"
#include "planvm/agendaitemids.h"
#include "math/transformobject.h"
#include "globals.h"
#include "common/stub.h"
#include <cmath>
#include "devicector.h"
#include "planvm/agendaitem.h"
#include "engine/crelaxyaw.h"
#include "planvm/plannode.h"
#include "common/pooledownerroot.h"

// The band's upper bound is the tenth at 0x004b1440.
#define SPIN_BAND 0.1

// FUNCTION: HYPERX 0x00410920
void CRelaxYaw::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind   = Agenda_CRelaxYaw;
    m_pNext   = nullptr;
    m_bReady  = 0;
    m_pOwner  = pSubject;
    m_flAngle = g_flZeroAgendaItem;
    m_flDecay = 0.92f;
}

// FUNCTION: HYPERX 0x00410950
void CRelaxYaw::BindWithNode(EngineObject* pObject, GameBehaviorObject*, PlanNode* pNode, int)
{
    m_wKind  = Agenda_CRelaxYaw;
    m_pNext  = nullptr;
    m_bReady = 0;
    m_pOwner = pObject;

    if (pNode->m_nArgCount > 1)
    {
        m_flAngle = pNode->m_pArgs[1].fl;
    }
    else
    {
        m_flAngle = g_flZeroAgendaItem;
    }

    if (pNode->m_nArgCount > 2)
    {
        m_flDecay = pNode->m_pArgs[2].fl;
        return;
    }

    m_flDecay = 0.92f;
}

// FUNCTION: HYPERX 0x004109a0
void CRelaxYaw::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext   = nullptr;
        m_pOwner  = nullptr;
        m_flAngle = g_flZeroAgendaItem;
        m_bReady  = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// FUNCTION: HYPERX 0x004109d0
void CRelaxYaw::Process(int nArg)
{
    if (m_flAngle != g_flZeroAgendaItem)
    {
        const auto flAngle = m_flAngle;

        auto* const pTransform = m_pOwner->GetLocalTransformPtr();

        pTransform->m_rotZ.flCos = static_cast<float>(cos(static_cast<double>(flAngle)));
        pTransform->m_rotZ.flSin = static_cast<float>(sin(static_cast<double>(flAngle)));

        pTransform->ApplyRotationZ();

        m_flAngle = m_flDecay * m_flAngle;
    }

    auto* const pNext = m_pNext;

    if (pNext != nullptr && m_bReady == 0)
    {
        pNext->Process(nArg);
    }

    if (m_flAngle > -SPIN_BAND && m_flAngle < 0.1)
    {
        m_flAngle = g_flZeroAgendaItem;

        m_pOwner->PostSceneEvent(kMsgSpunDown, 0);

        Detach();
    }
}

// FUNCTION: HYPERX 0x00410a70
void CRelaxYaw::Release(int nArg)
{
    if (m_flAngle != g_flZeroAgendaItem)
    {
        const auto flAngle = m_flAngle;

        auto* const pTransform = m_pOwner->GetLocalTransformPtr();

        pTransform->m_rotZ.flCos = static_cast<float>(cos(static_cast<double>(flAngle)));
        pTransform->m_rotZ.flSin = static_cast<float>(sin(static_cast<double>(flAngle)));

        pTransform->ApplyRotationZ();

        m_flAngle = m_flDecay * m_flAngle;
    }

    auto* const pNext = m_pNext;

    if (pNext != nullptr && m_bReady == 0)
    {
        pNext->Release(nArg);
    }

    if (m_flAngle > -SPIN_BAND && m_flAngle < 0.1)
    {
        m_flAngle = g_flZeroAgendaItem;

        m_pOwner->PostSceneEvent(kMsgSpunDown, 0);

        Detach();
    }
}
