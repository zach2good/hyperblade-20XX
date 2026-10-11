// Each static control's text is a version key, replaced by its value from the version resource.

#include "common/compat.h"
#include "globals.h"
#include "app/gameapp.h"
#include "app/frametimer.h"
#include <stdlib.h>
#include <string.h>
#include "common/stringpool.h"
#include "graphics/gdi.h"
#include "engine/game.h"
#include "ui/splashdialog.h"
#include "app/win32.h"
#include "engine/gameengine.h"
#include "app/dialogresources.h"

#define SPLASH_FIRST_CONTROL 1000
#define SPLASH_LAST_CONTROL  1004

#define SPLASH_SUBBLOCK_STRING 0x3c
#define SPLASH_FACE_STRING     0x3d

// FUNCTION: HYPERX 0x004070b0
BOOL CALLBACK SplashDialog::Proc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam)
{
    switch (nMsg)
    {
        case WM_PAINT:
            SplashDialog::PaintSplashBitmap(hDlg);

            return FALSE;

        case WM_DESTROY:
            DeleteObject((HGDIOBJ)SendMessageA(GetDlgItem(hDlg, SPLASH_FIRST_CONTROL), WM_GETFONT, 0, 0));

            return FALSE;

        case WM_INITDIALOG:
            return SplashDialog::OnInitDialog(hDlg, wParam, lParam);

        case WM_COMMAND:
            SplashDialog::OnCommand(hDlg, LOWORD(wParam), lParam, HIWORD(wParam));

            break;
    }

    return FALSE;
}

// FUNCTION: HYPERX 0x00407150
BOOL __cdecl SplashDialog::OnInitDialog(HWND hDlg, UINT, LONG)
{
    DWORD dwHandle;
    char* pszPrefix;
    char  szSubBlock[MAX_PATH];
    char  szText[MAX_PATH];
    char  szFileName[MAX_PATH];
    HFONT hFont;
    int   nId;

    Win32::CenterWindow(hDlg, GetParent(hDlg));

    GetModuleFileNameA((HMODULE)GetWindowLongA(hDlg, GWL_HINSTANCE), szFileName, sizeof(szFileName));

    const DWORD dwSize = GetFileVersionInfoSizeA(szFileName, &dwHandle);

    if (dwSize != 0)
    {
        void* const pVersion = malloc(dwSize);

        if (pVersion != 0)
        {
            GetFileVersionInfoA(szFileName, dwHandle, dwSize, pVersion);

            pszPrefix = g_gameApp.m_stringPool.LoadText(g_gameApp.GetInstance(), SPLASH_SUBBLOCK_STRING);

            strcpy(szSubBlock, pszPrefix);

            char* pszEnd = szSubBlock + strlen(szSubBlock);

            for (nId = SPLASH_FIRST_CONTROL; nId <= SPLASH_LAST_CONTROL; nId++)
            {
                GetDlgItemTextA(hDlg, nId, szText, sizeof(szText));

                *pszEnd = 0;

                strcat(szSubBlock, szText);

                UINT  uLen   = 0;
                void* pValue = 0;

                if (VerQueryValueA(pVersion, szSubBlock, &pValue, &uLen) && uLen != 0 && pValue != 0)
                {
                    strcpy(szText, (const char*)pValue);

                    SetDlgItemTextA(hDlg, nId, szText);
                }
            }

            free(pVersion);
        }
    }

    hFont = CreateFontA(14, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, VARIABLE_PITCH | FF_SWISS, g_gameApp.m_stringPool.LoadText(g_gameApp.m_hInstance, SPLASH_FACE_STRING));

    if (hFont != 0)
    {
        for (nId = SPLASH_FIRST_CONTROL; nId <= SPLASH_LAST_CONTROL; nId++)
        {
            SendMessageA(GetDlgItem(hDlg, nId), WM_SETFONT, (WPARAM)hFont, 1);
        }
    }

    return 1;
}

// FUNCTION: HYPERX 0x00407360
int SplashDialog::OnCommand(HWND hDlg, int nId, LONG, int)
{
    switch (nId)
    {
        case IDCANCEL:
        case ID_SPLASH_PLAY:
        case ID_SPLASH_SETUP:
            EndDialog(hDlg, nId != ID_SPLASH_SETUP);

            return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x004073a0
BOOL __cdecl SplashDialog::PaintSplashBitmap(HWND hDlg)
{
    HPALETTE hPalette;
    BITMAP   bm;

    BOOL bPainted = FALSE;

    if (GetDlgItem(hDlg, ID_SPLASH_PLAY) == 0)
    {
        // STRING: HYPERX 0x004bb3ac
        const HBITMAP hBitmap = GDI::LoadBitmapWithPalette("Final512", &hPalette);

        if (hBitmap != 0)
        {
            if (GetObjectA(hBitmap, sizeof(BITMAP), &bm) != 0)
            {
                const HDC hDC = GetDC(hDlg);

                if (hDC != 0)
                {
                    SelectPalette(hDC, hPalette, FALSE);
                    RealizePalette(hDC);

                    const HDC hMemDC = CreateCompatibleDC(hDC);

                    if (hMemDC != 0)
                    {
                        SelectPalette(hMemDC, hPalette, FALSE);
                        RealizePalette(hMemDC);

                        HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);

                        bPainted = BitBlt(hDC, 15, 15, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);

                        DeleteObject(SelectObject(hMemDC, hOldBitmap));
                        DeleteDC(hMemDC);
                    }

                    ReleaseDC(hDlg, hDC);
                }

                DeleteObject(hPalette);
            }

            DeleteObject(hBitmap);
        }
    }

    return bPainted;
}

// FUNCTION: HYPERX 0x0044d320
void SplashDialog::ShowAbout(HINSTANCE hInstance, HWND hWnd)
{
    switch (DialogBoxParamA(hInstance, MAKEINTRESOURCEA(DIALOG_SPLASH), hWnd, SplashDialog::Proc, 0))
    {
        case 0:
            Game::SetEngineState(ESTATE_LOBBY);

            g_nPrevEngineState = ESTATE_MATCH_LIVE;
            break;

        case 1:
            if (g_frameTimer.bFrameTimesOn != 0)
            {
                g_frameTimer.nClockReads++;

                g_frameTimer.dwFrameStartTime = timeGetTime();

                GameEngine::TogglePause(hWnd, 1);

                return;
            }
            break;
    }

    GameEngine::TogglePause(hWnd, 1);
}
