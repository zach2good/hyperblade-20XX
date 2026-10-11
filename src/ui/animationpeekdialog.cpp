#include "common/compat.h"
#include "engine/objectclasses.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "dialogids.h"
#include "globals.h"
#include "engine/gameengine.h"
#include "engine/cameraobject.h"
#include "engine/engineobject.h"
#include "anim/animplayer.h"
#include "anim/animdb.h"
#include "ui/animationpeekdialog.h"
#include "ui/dialog.h"
#include "app/win32.h"

// GLOBAL: HYPERX 0x004b22ac
extern char* const g_pszEmptyDefaultName;

// The marker is in src/planvm/ctracktraj.cpp.
extern char g_bCamTrackLookEnabled;

// GLOBAL: HYPERX 0x004cb42c
extern unsigned char g_bSmartCamEnabled;

namespace AnimationPeekDialog
{

static inline const char* NameOrDefault(const AnimDB* pSet);

}

const char* AnimationPeekDialog::NameOrDefault(const AnimDB* pSet)
{
    if (pSet != 0)
    {
        return pSet->GetName();
    }

    return g_pszEmptyDefaultName;
}

// FUNCTION: HYPERX 0x00417a40
unsigned int __cdecl AnimationPeekDialog::OnInit(HWND hDlg, unsigned int, long)
{
    Win32::CenterWindow(hDlg, GetParent(hDlg));

    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_A0),
                   NameOrDefault(g_pGameEngine->m_ppSlots[0]->m_pAnimPlayer->m_pSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_A1),
                   NameOrDefault(g_pGameEngine->m_ppSlots[1]->m_pAnimPlayer->m_pSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_A2),
                   NameOrDefault(g_pGameEngine->m_ppSlots[2]->m_pAnimPlayer->m_pSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_A3),
                   NameOrDefault(g_pGameEngine->m_ppSlots[3]->m_pAnimPlayer->m_pSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_A4),
                   NameOrDefault(g_pGameEngine->m_ppSlots[4]->m_pAnimPlayer->m_pSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_A5),
                   NameOrDefault(g_pGameEngine->m_ppSlots[5]->m_pAnimPlayer->m_pSet));

    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_B0),
                   NameOrDefault(g_pGameEngine->m_ppSlots[0]->m_pAnimPlayer->m_pPrevSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_B1),
                   NameOrDefault(g_pGameEngine->m_ppSlots[1]->m_pAnimPlayer->m_pPrevSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_B2),
                   NameOrDefault(g_pGameEngine->m_ppSlots[2]->m_pAnimPlayer->m_pPrevSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_B3),
                   NameOrDefault(g_pGameEngine->m_ppSlots[3]->m_pAnimPlayer->m_pPrevSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_B4),
                   NameOrDefault(g_pGameEngine->m_ppSlots[4]->m_pAnimPlayer->m_pPrevSet));
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME_B5),
                   NameOrDefault(g_pGameEngine->m_ppSlots[5]->m_pAnimPlayer->m_pPrevSet));

    Dialog::SetItemText(hDlg, IDC_POSITION_X, g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->GetPositionPtr()->x);
    Dialog::SetItemText(hDlg, IDC_POSITION_Y, g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->GetPositionPtr()->y);
    Dialog::SetItemText(hDlg, IDC_POSITION_Z, g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->GetPositionPtr()->z);

    Dialog::SetItemText(
        hDlg, IDC_OFFSET_X, g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->m_pSettings->x);
    Dialog::SetItemText(
        hDlg, IDC_OFFSET_Y, g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->m_pSettings->y);
    Dialog::SetItemText(
        hDlg, IDC_OFFSET_Z, g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->m_pSettings->z);

    Dialog::SetItemText(
        hDlg, IDC_DIRECTION_X, g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->GetPhysicsBody()->velDir.x);
    Dialog::SetItemText(
        hDlg, IDC_DIRECTION_Y, g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->GetPhysicsBody()->velDir.y);
    Dialog::SetItemText(
        hDlg, IDC_DIRECTION_Z, g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->GetPhysicsBody()->velDir.z);

    Dialog::SetItemText(hDlg, IDC_LOOK_HEIGHT, g_flCameraLookHeight);

    Dialog::SetItemText(hDlg, IDC_BUNGEE_LENGTH, g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->GetBungeeLength());

    Dialog::SetItemText(hDlg, IDC_TRACKING_HEIGHT, g_flCameraTrackingHeight);
    Dialog::SetItemText(hDlg, IDC_STEADY_CAM, g_flSteadyCamBuffer);
    Dialog::SetItemText(hDlg, IDC_MIN_HEIGHT, g_flCameraMinHeight);
    Dialog::SetItemText(hDlg, IDC_SOLAR_WIND, g_flSolarWindFactor);

    SendMessageA(GetDlgItem(hDlg, IDC_WATCH_ROK), BM_SETCHECK, g_bCamTrackLookEnabled != 0, 0);
    SendMessageA(GetDlgItem(hDlg, IDC_SMART_CAM), BM_SETCHECK, g_bSmartCamEnabled != 0, 0);
    SendMessageA(GetDlgItem(hDlg, IDC_SCANNING_MODES), BM_SETCHECK, g_bScanningModesEnabled != 0, 0);

    return 1;
}

// One in the image; ObjectStats::UpdateMomentumAndEnergy reads it before any of its energy work.
// GLOBAL: HYPERX 0x004c4b88
extern unsigned char g_bEnergyOn;

// OK stores the second direction field over the first, and clears two physics-sheet flags.
// 0x40e and 0x41e belong to the physics sheet; GetDlgItem answers null for both here.
// FUNCTION: HYPERX 0x00417e80
unsigned int __cdecl AnimationPeekDialog::OnCommand(HWND hDlg, int nCommand, HWND, int nNotify)
{
    switch (nCommand)
    {
        case IDCANCEL:
            if (nNotify == BN_CLICKED)
            {
                EndDialog(hDlg, 0);
            }

            break;

        case IDOK:
            if (nNotify == BN_CLICKED)
            {
                CameraObject* const pCamera = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);

                if (pCamera != 0)
                {
                    pCamera->GetPositionPtr()->x = Dialog::GetItemFloat(hDlg, IDC_POSITION_X, 0);
                    pCamera->GetPositionPtr()->y = Dialog::GetItemFloat(hDlg, IDC_POSITION_Y, 0);
                    pCamera->GetPositionPtr()->z = Dialog::GetItemFloat(hDlg, IDC_POSITION_Z, 0);

                    Vec3* pOffset = pCamera->m_pSettings;
                    pOffset->x    = Dialog::GetItemFloat(hDlg, IDC_OFFSET_X, 0);

                    pOffset    = pCamera->m_pSettings;
                    pOffset->y = Dialog::GetItemFloat(hDlg, IDC_OFFSET_Y, 0);

                    pOffset    = pCamera->m_pSettings;
                    pOffset->z = Dialog::GetItemFloat(hDlg, IDC_OFFSET_Z, 0);

                    PhysicsBody* pBody = pCamera->GetPhysicsBody();
                    pBody->velDir.x    = Dialog::GetItemFloat(hDlg, IDC_DIRECTION_X, 0);

                    // The original's own fault: this stores over the field above it.
                    pBody           = pCamera->GetPhysicsBody();
                    pBody->velDir.x = Dialog::GetItemFloat(hDlg, IDC_DIRECTION_Y, 0);

                    pBody           = pCamera->GetPhysicsBody();
                    pBody->velDir.z = Dialog::GetItemFloat(hDlg, IDC_DIRECTION_Z, 0);
                }

                g_bCamTrackLookEnabled =
                    (char)(SendMessageA(GetDlgItem(hDlg, IDC_WATCH_ROK), BM_GETCHECK, 0, 0) != 0);

                g_flCameraLookHeight = Dialog::GetItemFloat(hDlg, IDC_LOOK_HEIGHT, 0);

                CameraObject* const pObject  = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT);
                const float         flBungee = Dialog::GetItemFloat(hDlg, IDC_BUNGEE_LENGTH, 0);
                pObject->m_flBungeeLength    = flBungee;

                g_flCameraTrackingHeight = Dialog::GetItemFloat(hDlg, IDC_TRACKING_HEIGHT, 0);
                g_flCameraMinHeight      = Dialog::GetItemFloat(hDlg, IDC_MIN_HEIGHT, 0);

                g_flSolarWindFactor = Dialog::GetItemFloat(hDlg, IDC_SOLAR_WIND, 0);

                if (Dialog::GetItemFloat(hDlg, IDC_STEADY_CAM, 0) != g_flSteadyCamBuffer)
                {
                    g_flSteadyCamBuffer = Dialog::GetItemFloat(hDlg, IDC_STEADY_CAM, 0);

                    float flRate = g_pGameEngine->m_clock.flInvDelta;

                    short nWindow = (short)(flRate * g_flSteadyCamBuffer * 0.5);

                    if (nWindow > CAMERA_HISTORY_CAPACITY)
                    {
                        nWindow = CAMERA_HISTORY_CAPACITY;
                    }
                    else if (nWindow < 1)
                    {
                        nWindow = 1;
                    }

                    g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->SetPosHistorySize(nWindow);

                    flRate = g_pGameEngine->m_clock.flInvDelta;

                    nWindow = (short)(flRate * g_flSteadyCamBuffer * 0.4);

                    if (nWindow > CAMERA_HISTORY_CAPACITY)
                    {
                        nWindow = CAMERA_HISTORY_CAPACITY;
                    }
                    else if (nWindow < 1)
                    {
                        nWindow = 1;
                    }

                    g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->SetDirHistorySize(nWindow);
                }

                g_bEnergyOn = (unsigned char)(SendMessageA(GetDlgItem(hDlg, IDC_ENERGY_ON),
                                                           BM_GETCHECK,
                                                           0,
                                                           0) != 0);

                g_bVictimCam = SendMessageA(GetDlgItem(hDlg, IDC_VICTIM_CAM), BM_GETCHECK, 0, 0) != 0;

                g_bSmartCamEnabled = (unsigned char)(SendMessageA(GetDlgItem(hDlg, IDC_SMART_CAM),
                                                                  BM_GETCHECK,
                                                                  0,
                                                                  0) != 0);

                g_bScanningModesEnabled =
                    (unsigned char)(SendMessageA(GetDlgItem(hDlg, IDC_SCANNING_MODES),
                                                 BM_GETCHECK,
                                                 0,
                                                 0) != 0);

                EndDialog(hDlg, 1);
            }

            break;

        default:
            return 0;
    }

    return 1;
}
