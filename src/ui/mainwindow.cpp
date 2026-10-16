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
#include "input/gamepad.h"
#include "engine/objectclasses.h"
#include "anim/animset.h"
#include "planvm/planvm.h"
#include "ui/dialogids.h"
#include "ui/dialog.h"
#include "ui/mainwindow.h"
#include "engine/object.h"
#include "app/win32.h"
#include "app/wndproc.h"
#include "app/menuids.h"

extern unsigned char g_bEnergyOn;

// Cases 4 and 0x66 share a resize-check tail: the original jumps into the other case's body.
// FUNCTION: HYPERX 0x0044cac0
LRESULT __cdecl MainWindow::OnCommand(HWND hWnd, unsigned int nCommand, LPARAM lParam, int nNotify)
{
    RECT rect;

    switch (nCommand)
    {
        case 0x7a:
        case 0x9f:
        case ID_MENU_MIDI_OFF:
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

    Gamepad::Init();

    return 1;
}

short g_wUnusedBesidePalette = 1;
short g_wPaletteMode         = 3;
