// Pushing the head camera's shot power level at the renderer each tick.

#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "engine/objectclasses.h"
#include "cameraobject.h"
#include "planvm/agendaitem.h"
#include "engine/cdisplayshotpower.h"
#include "gameengine.h"
#include "globals.h"
#include "ui/widget.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "ui/screens/gameconfiguiscreen.h"
#include "graphics/gdi.h"

// FUNCTION: HYPERX 0x00412330
void CDisplayShotPower::Bind(EngineObject* pSubject, GameBehaviorObject*)
{
    m_wKind  = Agenda_CDisplayShotPower;
    m_pNext  = 0;
    m_bReady = 0;
    m_pOwner = pSubject;

    g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT)->m_nShotPowerLevel = 1;
}

// FUNCTION: HYPERX 0x00412390
void CDisplayShotPower::Process(int nArg)
{
    if (g_pRenderer != 0)
    {
        CameraObject* const pSource = g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT);

        if (pSource != 0)
        {
            g_pRenderer->SetShotPower((float)pSource->m_nShotPowerLevel);
            g_pRenderer->ShowShotPower();
        }
    }

    if (m_pNext != 0 && m_bReady == 0)
    {
        m_pNext->Process(nArg);
    }
}
