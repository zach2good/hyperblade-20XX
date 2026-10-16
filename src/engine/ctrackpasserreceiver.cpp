// 0x004cb3d0 is the trackers' own collision ellipsoid, separate from the arena's.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "physics/physicsbody.h"
#include "cameraobject.h"
#include "objectclasses.h"
#include "planvm/agendaitem.h"
#include "engine/ctrackpasserreceiver.h"
#include "devicector.h"
#include "engine/objectstats.h"
#include "gameengine.h"
#include "globals.h"
#include "common/staticinit.h"
#include "math/vector.h"
#include "planvm/damagegate.h"

#include <math.h>
#include "physics/collisionellipsoid.h"
#include "app/gameapp.h"
#include "engine/engineobject.h"
#include "graphics/meshbone.h"
#include "common/pooledownerroot.h"
#include "engine/messageids.h"

// The aim point the camera springs at, rebuilt every frame.
// GLOBAL: HYPERX 0x004cb3b8
extern Vec3 g_vCamPasserAim;

// Two: the aim point's added height and the fresh acceleration's factor. Marker in ctracktraj.cpp.
extern const float g_flCamTrackVelocityGain;

// Four: the square component's factor and the head camera's lift.
extern const float g_flFourTracking;

// FUNCTION: HYPERX 0x0040aec0
void CTrackPasser_Receiver::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext        = 0;
    m_wKind        = Agenda_CTrackPasser_Receiver;
    m_bReady       = 0;
    m_pOwner       = pOwner;
    m_pReceiverPos = 0;

    m_pCamPos     = &pOwner->GetPhysicsBody()->pos;
    m_pCamVel     = &pOwner->GetPhysicsBody()->vel;
    m_pCamDir     = &pOwner->GetPhysicsBody()->velDir;
    m_pDesiredVel = &pOwner->GetPhysicsBody()->smoothVel;
    m_flSeed      = g_flZeroTracking;
}

// FUNCTION: HYPERX 0x0040af20
void CTrackPasser_Receiver::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pOwner       = 0;
        m_pReceiverPos = 0;
        m_pCamPos      = 0;
        m_pCamVel      = 0;
        m_pCamDir      = 0;
        m_pDesiredVel  = 0;

        m_flSeed = g_flZeroTracking;
        m_pNext  = 0;
        m_bReady = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// A carrier turning across the camera drags the aim point sideways.
// FUNCTION: HYPERX 0x0040af60
void CTrackPasser_Receiver::Process(int nArg)
{
    Vec3 vAxis;

    const float flStep   = g_pGameEngine->m_clock.GetDelta() * 4.0f;
    const float flFollow = GetCamera()->GetBungeeLength();

    static Vec3 s_vLastVel = { g_flZeroTracking, g_flZeroTracking, g_flZeroTracking };

    EngineObject* pTracked = GetCamera()->GetTrackedObject();

    if (pTracked == 0)
    {
        // STRING: HYPERX 0x004bb55c
        g_gameApp.Fatal("CTrackPasser_Receiver::Process(): Null pointer for model to track\n");
    }

    if (pTracked->GetSlot() < PLAYER_OBJECT_COUNT)
    {
        const TransformObject* const pWorld = &pTracked->m_pBones->m_world;

        g_vCamPasserAim.x = pWorld->GetElement(0, 3);

        g_vCamPasserAim.y = pWorld->GetElement(1, 3);

        g_vCamPasserAim.z = (float)(pWorld->GetElement(2, 3) + g_flCamTrackVelocityGain);
    }
    else
    {
        g_vCamPasserAim.x = pTracked->GetPositionPtr()->x;
        g_vCamPasserAim.y = pTracked->GetPositionPtr()->y;
        g_vCamPasserAim.z = pTracked->GetPositionPtr()->z;
    }

    vAxis = s_vLastVel;

    s_vLastVel = pTracked->GetPhysicsBody()->vel;

    g_vCamPasserAim.x = (float)(g_flCamTrackVelocityGain * s_vLastVel.x + g_vCamPasserAim.x);

    g_vCamPasserAim.y = (float)(g_flCamTrackVelocityGain * s_vLastVel.y + g_vCamPasserAim.y);

    g_vCamPasserAim.z = (float)(g_flCamTrackVelocityGain * s_vLastVel.z + g_vCamPasserAim.z);

    const Vec3* pDir = &pTracked->GetPhysicsBody()->velDir;

    const float flCrossX = vAxis.y * pDir->z - vAxis.z * pDir->y;
    const float flCrossY = vAxis.z * pDir->x - vAxis.x * pDir->z;
    const float flCrossZ = vAxis.x * pDir->y - vAxis.y * pDir->x;

    vAxis.x = flCrossY * pDir->z - flCrossZ * pDir->y;
    vAxis.y = flCrossZ * pDir->x - flCrossX * pDir->z;
    vAxis.z = flCrossX * pDir->y - flCrossY * pDir->x;

    g_vCamPasserAim.x = g_flFourTracking * vAxis.x + g_vCamPasserAim.x;
    g_vCamPasserAim.y = g_flFourTracking * vAxis.y + g_vCamPasserAim.y;
    g_vCamPasserAim.z = g_flFourTracking * vAxis.z + g_vCamPasserAim.z;

    EngineObject* pReceiver = GetCamera()->GetSecondaryTrackedObject();

    if (pReceiver == 0 || pReceiver == pTracked)
    {
        m_pOwner->PostSceneEvent(MSG_END_PASS_MODE, 0);

        if (m_pNext != 0)
        {
            m_pNext->Process(nArg);
        }
    }
    else
    {
        short      wCarrierSlot = 0;
        char       bCarried;
        AttachPak* pHolder;
        bool       bCarrierElsewhere;
        float      flLength;
        float      flLead;
        float      flFloor;
        Vec3       vToReceiver;
        Vec3       vToAim;
        Vec3       vLead;
        Vec3       vHeld;
        Vec3       vCamDir;
        Vec3       vSecondRow;
        Vec3       vTorque;

        pHolder = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pHolder != 0 && pHolder->nSlot < PLAYER_OBJECT_COUNT)
        {
            wCarrierSlot = pHolder->nSlot;
            bCarried     = 1;
        }
        else
        {
            bCarried = 0;
        }

        bCarrierElsewhere = (wCarrierSlot != pTracked->GetSlot());

        m_pReceiverPos = &pReceiver->GetPhysicsBody()->pos;

        if (bCarried != 0 && !bCarrierElsewhere && pReceiver->GetSlot() == SLOT_ROK)
        {
            m_pOwner->PostSceneEvent(MSG_END_PASS_MODE, 0);
        }

        vToReceiver.x = m_pReceiverPos->x - g_vCamPasserAim.x;
        vToReceiver.y = m_pReceiverPos->y - g_vCamPasserAim.y;
        vToReceiver.z = (m_pReceiverPos->z - g_vCamPasserAim.z) + g_flFourTracking;

        vToAim.x = g_vCamPasserAim.x - m_pCamPos->x;
        vToAim.y = g_vCamPasserAim.y - m_pCamPos->y;
        vToAim.z = g_vCamPasserAim.z - m_pCamPos->z;

        flLength = (float)sqrt(vToReceiver.x * vToReceiver.x + (vToReceiver.y * vToReceiver.y + vToReceiver.z * vToReceiver.z));

        Math::Vec3Normalize(&vToReceiver);

        Vec3::ScaleTo(&vToReceiver, flFollow, &vHeld);

        vLead.x = vToAim.x - vHeld.x;
        vLead.y = vToAim.y - vHeld.y;
        vLead.z = vToAim.z - vHeld.z;

        Math::Vec3Normalize(&vToAim);

        if (flLength > 50.0 || vToAim.x * vToReceiver.x + (vToAim.y * vToReceiver.y + vToAim.z * vToReceiver.z) < 0.3f)
        {
            m_pOwner->PostSceneEvent(MSG_HEAD_CAM_OFF, 0);
        }

        flLead = (float)sqrt(vLead.x * vLead.x + (vLead.y * vLead.y + vLead.z * vLead.z));

        Math::Vec3Normalize(&vLead);

        Vec3::ScaleTo(&vLead, flLead * flStep, m_pDesiredVel);

        m_pCamVel->x = (m_pDesiredVel->x - m_pCamVel->x * 0.4f) + m_pCamVel->x;
        m_pCamVel->y = (m_pDesiredVel->y - m_pCamVel->y * 0.4f) + m_pCamVel->y;
        m_pCamVel->z = (m_pDesiredVel->z - m_pCamVel->z * 0.4f) + m_pCamVel->z;

        m_pCamPos->x = m_pCamVel->x + m_pCamPos->x;
        m_pCamPos->y = m_pCamVel->y + m_pCamPos->y;
        m_pCamPos->z = m_pCamVel->z + m_pCamPos->z;

        if (m_pOwner->GetSlot() == CAMERA_HEAD_SLOT)
        {
            const Vec3* pSum = &GetCamera()->m_posOut;

            m_pCamDir->x = m_pReceiverPos->x - pSum->x;
            m_pCamDir->z = (m_pReceiverPos->z - pSum->z) + g_flFourTracking;
            m_pCamDir->y = m_pReceiverPos->y - pSum->y;
        }
        else
        {
            m_pCamDir->x = g_vCamPasserAim.x - m_pCamPos->x;
            m_pCamDir->y = g_vCamPasserAim.y - m_pCamPos->y;
            m_pCamDir->z = g_vCamPasserAim.z - m_pCamPos->z;
        }

        Math::Vec3Normalize(m_pCamDir);

        vCamDir = *m_pCamDir;

        vSecondRow = pTracked->GetPhysicsBody()->surfNormal;

        const Vec3* pVel = GetCamera()->m_pSettings;

        vTorque.x = vSecondRow.z * vCamDir.y - vCamDir.z * vSecondRow.y;
        vTorque.y = vSecondRow.x * vCamDir.GetZ() - vSecondRow.z * vCamDir.x;
        vTorque.z = vCamDir.x * vSecondRow.GetY() - vCamDir.y * vSecondRow.x;

        vTorque.ScaleInPlace(pVel->x);
        vCamDir.ScaleInPlace(pVel->y);
        vSecondRow.ScaleInPlace(pVel->z);

        flFloor = g_camTrackEllipsoid.GetZ((vTorque.x + m_pCamPos->x) + vCamDir.x,
                                           vTorque.y + m_pCamPos->y + vCamDir.y) +
                  g_flCameraMinHeight;

        if (flFloor > vSecondRow.z + m_pCamPos->z)
        {
            float flGap;
            Vec3  vBack;

            m_pCamPos->z = flFloor - vSecondRow.z;

            vBack.x = g_vCamPasserAim.x - m_pCamPos->x;
            vBack.y = g_vCamPasserAim.y - m_pCamPos->y;
            vBack.z = g_vCamPasserAim.z - m_pCamPos->z;

            flGap = (float)sqrt(vBack.x * vBack.x + (vBack.y * vBack.y + vBack.z * vBack.z));

            Math::Vec3Normalize(&vBack);

            vBack.ScaleInPlace(flGap - flFollow);

            m_pCamPos->x = vBack.x + m_pCamPos->x;
            m_pCamPos->y = vBack.y + m_pCamPos->y;
            m_pCamPos->z = vBack.z + m_pCamPos->z;
        }

        GetCamera()->UpdateSmoothing();

        if (m_pOwner->GetSlot() == CAMERA_HEAD_SLOT)
        {
            const Vec3* pSum = &GetCamera()->m_posOut;

            m_pCamDir->x = m_pReceiverPos->x - pSum->x;
            m_pCamDir->z = (m_pReceiverPos->z - pSum->z) + g_flFourTracking;
            m_pCamDir->y = m_pReceiverPos->y - pSum->y;
        }
        else
        {
            const Vec3* pSum = &GetCamera()->m_posOut;

            m_pCamDir->x = g_vCamPasserAim.x - pSum->x;
            m_pCamDir->z = g_vCamPasserAim.z - pSum->z;
            m_pCamDir->y = g_vCamPasserAim.y - pSum->y;
        }

        Math::Vec3Normalize(m_pCamDir);

        GetCamera()->UpdateDirSmoothing();

        if (m_pNext != 0)
        {
            m_pNext->Process(nArg);
        }
    }
}
