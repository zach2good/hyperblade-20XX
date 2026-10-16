#include "common/compat.h"
#include "engine/messageids.h"
#include "globals.h"
#include "app/gameapp.h"
#include "engine/gameengine.h"
#include "sound/audiomgr.h"
#include "app/frametimer.h"
#include "graphics/colorpalette.h"
#include "uiglobals.h"
#include "engine/engineobject.h"
#include "input/joystickmgr.h"
#include "engine/objectclasses.h"
#include "anim/animset.h"
#include "planvm/planvm.h"
#include "ui/dialogids.h"
#include "ui/dialog.h"
#include "ui/keyconfigdialog.h"
#include "ui/mainwindow.h"
#include "engine/object.h"
#include "ui/ratingsdialog.h"
#include "planvm/planeditordialog.h"
#include "ui/splashdialog.h"
#include "app/win32.h"
#include "app/wndproc.h"
#include "app/menuids.h"
#include "app/dialogresources.h"

extern unsigned char g_bEnergyOn;

// Resource 0x13e: OK writes nine live physics constants. "Save As" writes nothing.
// FUNCTION: HYPERX 0x004197e0
INT_PTR CALLBACK MainWindow::PhysicsConfigProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
            Win32::CenterWindow(hDlg, GetParent(hDlg));

            Dialog::SetItemText(hDlg, IDC_STOP_FORCE_FACTOR, g_flStopForceFactor);
            Dialog::SetItemText(hDlg, IDC_SPEED_BURST_FACTOR, g_flSpeedBurstFactor);
            Dialog::SetItemText(hDlg, IDC_VIEW_STEEPNESS, g_vViewSteepness.z);
            Dialog::SetItemText(hDlg, IDC_FORCE_INCREMENT, g_flForceIncrement);
            Dialog::SetItemText(hDlg, IDC_MAX_ACCELERATION, g_flMaxAcceleration);
            Dialog::SetItemText(hDlg, IDC_MAX_SPEED, g_flMaxSpeed);
            Dialog::SetItemText(hDlg, IDC_FORCE_RELAXATION, g_flForceRelaxation);
            Dialog::SetItemText(hDlg, IDC_BANDWIDTH, g_flBandwidth);
            Dialog::SetItemText(hDlg, IDC_NET_TRACKING, g_flNetTracking);

            SendMessageA(GetDlgItem(hDlg, IDC_ENERGY_ON), BM_SETCHECK, g_bEnergyOn != 0, 0);
            SendMessageA(GetDlgItem(hDlg, IDC_VICTIM_CAM), BM_SETCHECK, g_bVictimCam != 0, 0);

            return 1;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        g_flStopForceFactor  = Dialog::GetItemFloat(hDlg, IDC_STOP_FORCE_FACTOR, 0);
                        g_flSpeedBurstFactor = Dialog::GetItemFloat(hDlg, IDC_SPEED_BURST_FACTOR, 0);
                        g_vViewSteepness.z   = Dialog::GetItemFloat(hDlg, IDC_VIEW_STEEPNESS, 0);
                        g_flForceIncrement   = Dialog::GetItemFloat(hDlg, IDC_FORCE_INCREMENT, 0);
                        g_flMaxAcceleration  = Dialog::GetItemFloat(hDlg, IDC_MAX_ACCELERATION, 0);
                        g_flMaxSpeed         = Dialog::GetItemFloat(hDlg, IDC_MAX_SPEED, 0);
                        g_flForceRelaxation  = Dialog::GetItemFloat(hDlg, IDC_FORCE_RELAXATION, 0);
                        g_flBandwidth        = Dialog::GetItemFloat(hDlg, IDC_BANDWIDTH, 0);
                        g_flNetTracking      = Dialog::GetItemFloat(hDlg, IDC_NET_TRACKING, 0);

                        EndDialog(hDlg, 1);
                    }

                    return 1;

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        EndDialog(hDlg, 0);
                    }

                    return 1;

                case IDC_SAVE_AS:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        KeyConfigDialog::SaveKeysAs(hDlg, g_aKeyRows);
                    }

                    return 1;
            }

            return 0;
    }

    return 0;
}

// The Ratings dialog, which sets one rating of one player.

// FUNCTION: HYPERX 0x00419a40
INT_PTR CALLBACK MainWindow::RatingsDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
            return (BOOL)RatingsDialog::OnInit(hDlg, (HWND)wParam, lParam);

        case WM_COMMAND:
            RatingsDialog::OnCommand(hDlg, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));

            return 0;
    }

    return 0;
}

// Both buttons answer only the plain click, and both return one.
// FUNCTION: HYPERX 0x0043ff70
INT_PTR CALLBACK MainWindow::DebugParamsDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
            Win32::CenterWindow(hDlg, GetParent(hDlg));

            return 1;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        EndDialog(hDlg, 1);
                    }

                    return 1;

                case IDCANCEL:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        EndDialog(hDlg, 0);
                    }

                    return 1;
            }

            return 0;
    }

    return 0;
}

// Cases 4 and 0x66 share a resize-check tail: the original jumps into the other case's body.
// FUNCTION: HYPERX 0x0044cac0
LRESULT __cdecl MainWindow::OnCommand(HWND hWnd, unsigned int nCommand, LPARAM lParam, int nNotify)
{
    RECT rect;

    switch (nCommand)
    {
        case ID_MENU_ABOUT:
            SplashDialog::ShowAbout(g_gameApp.m_hInstance, hWnd);
            // Falls through

        case 0x7a:
        case 0x9f:
        case ID_MENU_MIDI_OFF:
            return 0;

        case ID_MENU_CONFIGURE_KEYS:
            DialogBoxParamA(g_gameApp.m_hInstance, MAKEINTRESOURCEA(DIALOG_CONFIGURE_KEYBOARD), hWnd, MainWindow::KeyboardDialogProc, 0);
            return 0;

        case ID_MENU_PHYSICS_CONFIG:
            DialogBoxParamA(g_gameApp.m_hInstance, MAKEINTRESOURCEA(DIALOG_CONFIGURE_PHYSICS), hWnd, MainWindow::PhysicsConfigProc, 0);
            return 0;

        case ID_MENU_PLAN_EDITOR:
            MainWindow::OpenPlanEditor(g_gameApp.m_hInstance, hWnd);
            return 0;

        case ID_MENU_RATINGS:
            DialogBoxParamA(g_gameApp.m_hInstance, MAKEINTRESOURCEA(DIALOG_RATINGS), hWnd, MainWindow::RatingsDialogProc, 0);
            return 0;

        case ID_MENU_DEBUG_DIALOG:
            DialogBoxParamA(g_gameApp.m_hInstance, MAKEINTRESOURCEA(DIALOG_DEBUG_PARAMETERS), hWnd, MainWindow::DebugParamsDialogProc, 0);
            return 0;

        case ID_MENU_EXIT:
            g_pAudioMgr->StopCD();
            PostMessageA(hWnd, WM_CLOSE, 0, 0);
            return 0;

        case ID_MENU_NEW_GAME:
            g_pGameEngine->RestartMatch();
            return 0;

        case ID_MENU_FRAME_TIMES:
            if (g_frameTimer.bFrameTimesOn)
            {
                g_frameTimer.bFrameTimesOn = 0;
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_FRAME_TIMES, MF_UNCHECKED);
            }
            else
            {
                g_frameTimer.bFrameTimesOn = 1;
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_FRAME_TIMES, MF_CHECKED);
            }

            g_frameTimer.dwFrameStartTime = g_frameTimer.Restart();
            return 0;

        case ID_MENU_DISPLAY_HUD_ART:
        {
            unsigned char bFlag;

            bFlag                        = g_renderFlags.bDisplayHUDArt == 0;
            g_renderFlags.bDisplayHUDArt = bFlag;

            if (bFlag != 0)
            {
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_DISPLAY_HUD_ART, MF_CHECKED);
            }
            else
            {
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_DISPLAY_HUD_ART, MF_UNCHECKED);
            }

            return 0;
        }

        case ID_MENU_SHOW_GOALIES:
            if (CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SHOW_GOALIES, MF_CHECKED) == 0)
            {
                g_pGameEngine->InsertObjectSorted(4);
                g_pGameEngine->InsertObjectSorted(5);
                return 0;
            }

            CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SHOW_GOALIES, MF_UNCHECKED);
            g_pGameEngine->RemoveFromSortedList(4);
            g_pGameEngine->RemoveFromSortedList(5);
            return 0;

        case ID_MENU_SIZE_LARGE:
            GetWindowRect(hWnd, &rect);

            if (MoveWindow(hWnd, rect.left, rect.top, 0x288, 0x20e, TRUE) != 0)
            {
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_LARGE, MF_CHECKED);
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_MEDIUM, MF_UNCHECKED);
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_SMALL, MF_UNCHECKED);
            }

            break;

        case ID_MENU_SIZE_MEDIUM:
            GetWindowRect(hWnd, &rect);

            if (MoveWindow(hWnd, rect.left, rect.top, 0x1e8, 0x196, TRUE) != 0)
            {
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_MEDIUM, MF_CHECKED);
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_LARGE, MF_UNCHECKED);
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_SMALL, MF_UNCHECKED);
            }

            break;

        case ID_MENU_SIZE_SMALL:
            GetWindowRect(hWnd, &rect);

            if (MoveWindow(hWnd, rect.left, rect.top, 0x148, 0x11e, TRUE) != 0)
            {
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_SMALL, MF_CHECKED);
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_LARGE, MF_UNCHECKED);
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SIZE_MEDIUM, MF_UNCHECKED);
                return 0;
            }

            break;

        case ID_MENU_BIRDS_EYE:
            if (CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_BIRDS_EYE, MF_CHECKED) == 0)
            {
                g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->PostSceneEvent(MSG_MOVE_EYE_TRACK, 0);
                CheckMenuItem(g_gameApp.m_hMenu, 0x7a, MF_UNCHECKED);
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_OVER_SHOULDER, MF_UNCHECKED);
                return 0;
            }

            break;

        case ID_MENU_OVER_SHOULDER:
            if (CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_OVER_SHOULDER, MF_CHECKED) == 0 &&
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_BIRDS_EYE, MF_UNCHECKED) == MF_CHECKED)
            {
                g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->PostSceneEvent(MSG_MOVE_EYE_TRACK, 2);
                return 0;
            }

            break;

        case ID_RENDER_MDM_TEXTURES:
        case ID_RENDER_BILINEAR_FILTER:
        case ID_RENDER_PERSPECTIVE:
            WndProc::HandleRenderMenuCommand(g_gameApp.m_hMenu, (unsigned short)nCommand);
            return 0;

        case ID_MENU_DRAG_OFF:
            g_bDragOff = (g_bDragOff == 0);
            CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_DRAG_OFF, -g_bDragOff & MF_CHECKED);
            return 0;

        case ID_MENU_SOUND_OFF:
            if (g_pAudioMgr->wNoSound == 0)
            {
                g_pAudioMgr->wNoSound = 1;
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SOUND_OFF, MF_CHECKED);
                return 0;
            }

            if (g_pAudioMgr->wNoSound == 1)
            {
                g_pAudioMgr->wNoSound = 0;
                CheckMenuItem(g_gameApp.m_hMenu, ID_MENU_SOUND_OFF, MF_UNCHECKED);
                return 0;
            }

            break;

        default:
        {
            return DefWindowProcA(hWnd, WM_COMMAND, MAKEWPARAM(nCommand, nNotify), lParam);
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x0044d0d0
int MainWindow::OnCreate(HWND hWnd, LPARAM)
{
    g_gameApp.m_hScreenDC = GetDC(hWnd);

    g_pColorPalette->m_hdc = g_gameApp.m_hScreenDC;

    g_gameApp.m_hPalette = g_pColorPalette->ReloadPalette(g_wPaletteMode);

    JoystickMgr* pManager = new JoystickMgr(hWnd, 1);

    g_pJoystickMgr = pManager;

    if (pManager != 0 && pManager->m_nDeviceCount == 0)
    {
        delete pManager;

        g_pJoystickMgr = 0;
    }

    return 1;
}

// The dialog owns the PlanVM from here; its close handler releases it.
// FUNCTION: HYPERX 0x0044d390
int MainWindow::OpenPlanEditor(HINSTANCE hInstance, HWND hParent)
{
    return DialogBoxParamA(hInstance, MAKEINTRESOURCEA(DIALOG_PLAN_EDITOR), hParent, PlanEditorDialog::Proc, (LPARAM) new PlanVM);
}

// FUNCTION: HYPERX 0x0046c2a0
INT_PTR CALLBACK MainWindow::KeyboardDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
            return (BOOL)KeyConfigDialog::OnInitDialog(hDlg, (HWND)wParam, lParam);

        case WM_COMMAND:
            KeyConfigDialog::OnCommand(hDlg, LOWORD(wParam), (HWND)lParam, HIWORD(wParam));

            return 0;
    }

    return 0;
}

short g_wUnusedBesidePalette = 1;
short g_wPaletteMode         = 3;
