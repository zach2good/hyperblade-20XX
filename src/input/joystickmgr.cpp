#include "common/compat.h"
#include "joystickdevice.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "app/gameapp.h"
#include "inputglobals.h"
#include "keyboarddeviceobj.h"
#include <windows.h>
#include <mmsystem.h>
#include "joystickmgr.h"
#include "engine/gameoptions.h"

// FUNCTION: HYPERX 0x00423bf0
void __cdecl JoystickMgr::StartCapture(int nTwoPlayers)
{
    if (g_pJoystickMgr != 0)
    {
        const unsigned int nDevices = g_pJoystickMgr->m_nDeviceCount;

        g_nJoystickSlotA = 0;
        g_nJoystickSlotB = 1;

        if (nDevices > 1)
        {
            g_bJoystickShared = 0;

            if (nTwoPlayers != 0)
            {
                g_nJoystickId1 = 0;

                g_pJoystickMgr->CaptureJoystick(0, g_pJoystickMgr->m_hWnd);

                g_nJoystickId2 = g_nJoystickSlotB;

                g_pJoystickMgr->CaptureJoystick(1, g_pJoystickMgr->m_hWnd);

                return;
            }

            g_nJoystickId1 = 1;

            g_pJoystickMgr->CaptureJoystick(0, g_pJoystickMgr->m_hWnd);

            return;
        }

        if (nTwoPlayers != 0)
        {
            g_nJoystickId1    = 0;
            g_bJoystickShared = 1;

            g_pJoystickMgr->CaptureJoystick(0, g_pJoystickMgr->m_hWnd);

            return;
        }

        g_nJoystickId1    = 1;
        g_bJoystickShared = 0;

        g_pJoystickMgr->CaptureJoystick(0, g_pJoystickMgr->m_hWnd);

        return;
    }

    g_nJoystickSlotA  = 0;
    g_bJoystickShared = 0;
}

// Only ports that answer joyGetPosEx are counted, so with no stick m_pDevices stays null.
// FUNCTION: HYPERX 0x00423ca0
JoystickMgr::JoystickMgr(HWND hWnd, int bCaptureEnabled)
{
    JOYINFOEX    probe;
    unsigned int uJoyID;

    m_hWnd            = hWnd;
    m_bCaptureEnabled = bCaptureEnabled;
    m_pDevices        = 0;
    m_nDeviceCount    = 0;
    m_nLastButtonCode = 0;

    m_nDriverDeviceCount = joyGetNumDevs();

    if (m_nDriverDeviceCount == 0)
    {
        return;
    }

    for (uJoyID = 0; uJoyID < m_nDriverDeviceCount; uJoyID++)
    {
        memset(&probe, 0, sizeof(probe));

        probe.dwSize  = sizeof(JOYINFOEX);
        probe.dwFlags = JOY_RETURNALL | JOY_RETURNCENTERED | JOY_USEDEADZONE;

        if (joyGetPosEx(uJoyID, &probe) == JOYERR_NOERROR)
        {
            m_nDeviceCount++;
        }
    }

    if (m_nDeviceCount == 0)
    {
        return;
    }

    m_pDevices = new JoystickDevice[m_nDeviceCount];

    unsigned int uWorking = 0;

    for (uJoyID = 0; uJoyID < m_nDriverDeviceCount; uJoyID++)
    {
        if (m_pDevices[uWorking].Poll(uJoyID) != 0)
        {
            uWorking++;
        }

        if (uWorking >= m_nDeviceCount)
        {
            break;
        }
    }
}

// FUNCTION: HYPERX 0x00423e00
JoystickMgr::~JoystickMgr()
{
    for (unsigned int nJoy = 0; nJoy < m_nDeviceCount; nJoy++)
    {
        joyReleaseCapture(nJoy);
    }

    if (m_pDevices != 0)
    {
        delete[] m_pDevices;

        m_pDevices = 0;
    }
}

// FUNCTION: HYPERX 0x00423e60
int JoystickMgr::CaptureJoystick(UINT uJoyID, HWND hWnd)
{
    int nResult = JOYERR_NOCANDO;

    if (m_bCaptureEnabled != 0)
    {
        m_hWnd = hWnd;

        joySetCapture(hWnd, uJoyID, 0, TRUE);

        nResult = SetThreshold(uJoyID, 10);
    }

    return nResult;
}

// FUNCTION: HYPERX 0x00423ea0
int JoystickMgr::SetThreshold(UINT uJoyID, int nShift)
{
    const UINT uThreshold = 1 << nShift;

    const int nErr = joySetThreshold(uJoyID, uThreshold);

    if (nErr == JOYERR_NOERROR)
    {
        JoystickDevice* const pDevice = &m_pDevices[uJoyID];

        pDevice->m_nShift     = nShift;
        pDevice->m_uThreshold = uThreshold;

        const int nRangeX = (pDevice->m_caps.wXmax - pDevice->m_caps.wXmin + 1) >> (nShift + 1);
        const int nRangeY = (pDevice->m_caps.wYmax - pDevice->m_caps.wYmin + 1) >> (nShift + 1);

        pDevice->m_nRangeX     = nRangeX;
        pDevice->m_nRangeY     = nRangeY;
        pDevice->m_nHalfRangeX = nRangeX >> 1;
    }

    return nErr;
}

// Outside a live match the pad sends menu keys.
// FUNCTION: HYPERX 0x00423f20
void JoystickMgr::HandleButtonPress(int, unsigned int nButton, unsigned int, unsigned int)
{
    if (g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
    {
        if (g_pInputDevice == 0)
        {
            return;
        }

        unsigned int* const pnKeyBindings = g_pInputDevice->GetDefaultKeyBindings();

        JoystickMgr::LoadButtonConfig();

        if ((nButton & JOY_BUTTON1CHG) != 0 && g_anGamepadButton[0] >= 0)
        {
            g_pInputDevice->HandleKeyDown(pnKeyBindings[g_anGamepadButton[0]]);
        }
        if ((nButton & JOY_BUTTON2CHG) != 0 && g_anGamepadButton[1] >= 0)
        {
            g_pInputDevice->HandleKeyDown(pnKeyBindings[g_anGamepadButton[1]]);
        }
        if ((nButton & JOY_BUTTON3CHG) != 0 && g_anGamepadButton[2] >= 0)
        {
            g_pInputDevice->HandleKeyDown(pnKeyBindings[g_anGamepadButton[2]]);
        }
        if ((nButton & JOY_BUTTON4CHG) != 0 && g_anGamepadButton[3] >= 0)
        {
            g_pInputDevice->HandleKeyDown(pnKeyBindings[g_anGamepadButton[3]]);
            return;
        }
    }
    else
    {
        switch (nButton)
        {
            case JOY_BUTTON3CHG | JOY_BUTTON3:
                keybd_event(VK_CANCEL, 0, 0, 0);
                m_nLastButtonCode = JOY_BUTTON3CHG | JOY_BUTTON3;
                return;
            case JOY_BUTTON1CHG | JOY_BUTTON1:
                keybd_event(VK_TAB, 0, 0, 0);
                m_nLastButtonCode = JOY_BUTTON1CHG | JOY_BUTTON1;
                return;
            case JOY_BUTTON1CHG | JOY_BUTTON1 | JOY_BUTTON3:
                keybd_event(VK_MENU, 0, 0, 0);
                m_nLastButtonCode = JOY_BUTTON1CHG | JOY_BUTTON1 | JOY_BUTTON3;
                return;
            case JOY_BUTTON2CHG | JOY_BUTTON2:
                keybd_event(VK_RETURN, 0, 0, 0);
                m_nLastButtonCode = JOY_BUTTON2CHG | JOY_BUTTON2;
                return;
            case JOY_BUTTON4CHG | JOY_BUTTON4:
                keybd_event(VK_SHIFT, 0, 0, 0);
                keybd_event(VK_TAB, 0, 0, 0);
                m_nLastButtonCode = JOY_BUTTON4CHG | JOY_BUTTON4;
                break;
            case JOY_BUTTON3CHG | JOY_BUTTON3 | JOY_BUTTON4:
                keybd_event('Q', 0, 0, 0);
                m_nLastButtonCode = JOY_BUTTON3CHG | JOY_BUTTON3 | JOY_BUTTON4;
                return;
        }
    }
}

// Each button defaults to its own position; a value outside one to four leaves it unbound.
// FUNCTION: HYPERX 0x004240d0
void JoystickMgr::LoadButtonConfig(void)
{
    int nButton;

    if (g_bGamepadConfigLoaded != 0)
    {
        return;
    }

    if (g_pGameOptions == 0)
    {
        return;
    }

    g_bGamepadConfigLoaded = 1;

    for (int i = 0; i < 4; i++)
    {
        // STRING: HYPERX 0x004bc4ec
        nButton = g_pGameOptions->ReadInt("Gamepad", g_apszGamepadButtonKey[i], i + 1) - 1;

        if (nButton >= 0 && nButton < 4)
        {
            g_anGamepadButton[i] = nButton + BINDING_ATTACK;
        }
        else
        {
            g_anGamepadButton[i] = -1;
        }
    }
}

// FUNCTION: HYPERX 0x00424140
void JoystickMgr::HandleButtonRelease(int, unsigned int nButton, unsigned int, unsigned int)
{
    if (g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
    {
        if (g_pInputDevice == 0)
        {
            return;
        }

        unsigned int* const pnKeyBindings = g_pInputDevice->GetDefaultKeyBindings();

        JoystickMgr::LoadButtonConfig();

        if ((nButton & JOY_BUTTON1CHG) != 0)
        {
            g_pInputDevice->HandleKeyUp(pnKeyBindings[g_anGamepadButton[0]]);
        }
        if ((nButton & JOY_BUTTON2CHG) != 0)
        {
            g_pInputDevice->HandleKeyUp(pnKeyBindings[g_anGamepadButton[1]]);
        }
        if ((nButton & JOY_BUTTON3CHG) != 0)
        {
            g_pInputDevice->HandleKeyUp(pnKeyBindings[g_anGamepadButton[2]]);
        }
        if ((nButton & JOY_BUTTON4CHG) != 0)
        {
            g_pInputDevice->HandleKeyUp(pnKeyBindings[g_anGamepadButton[3]]);
            return;
        }
    }
    else
    {
        switch (nButton)
        {
            case JOY_BUTTON3CHG:
                if (m_nLastButtonCode == (JOY_BUTTON3CHG | JOY_BUTTON3 | JOY_BUTTON4))
                {
                    keybd_event('Q', 0, KEYEVENTF_KEYUP, 0);
                    return;
                }
                keybd_event(VK_CANCEL, 0, KEYEVENTF_KEYUP, 0);
                return;
            case JOY_BUTTON1CHG:
                if (m_nLastButtonCode == (JOY_BUTTON1CHG | JOY_BUTTON1 | JOY_BUTTON3))
                {
                    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
                    return;
                }
                keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
                return;
            case JOY_BUTTON2CHG:
                keybd_event(VK_RETURN, 0, KEYEVENTF_KEYUP, 0);
                return;
            case JOY_BUTTON4CHG:
                keybd_event(VK_TAB, 0, KEYEVENTF_KEYUP, 0);
                keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
        }
    }
}

// FUNCTION: HYPERX 0x004242b0
void JoystickMgr::HandleJoyAxis(int nDevice, unsigned int, unsigned short nX, unsigned short nY)
{
    unsigned int* pnKeys;
    unsigned int  nVKeyX;

    if (g_pInputDevice != 0)
    {
        pnKeys = g_pInputDevice->GetDefaultKeyBindings();
    }
    else
    {
        pnKeys = g_anJoystickDefaultKeys;
    }

    const unsigned short nShift    = (unsigned short)m_pDevices[nDevice].m_nShift;
    const unsigned int   nKeyHighX = pnKeys[BINDING_RIGHT];
    nX                             = (unsigned short)(nX >> nShift);
    nY                             = (unsigned short)(nY >> nShift);
    const unsigned int nKeyLow     = pnKeys[BINDING_LEFT];
    const unsigned int nPrevPosX   = m_pDevices[nDevice].m_infoPrevious.dwXpos;
    const unsigned int nPrevX      = g_dwPrevJoyXVKey;

    if (nX < (unsigned int)(m_pDevices[nDevice].m_nRangeX - m_pDevices[nDevice].m_nHalfRangeX))
    {
        if (g_pInputDevice != 0)
        {
            if (nPrevX != nKeyLow)
            {
                g_pInputDevice->HandleKeyUp(nPrevX);
            }
            g_pInputDevice->HandleKeyDown(nKeyLow);
        }
        nVKeyX = nKeyLow;
    }
    else if (nX > (unsigned int)(m_pDevices[nDevice].m_nHalfRangeX + m_pDevices[nDevice].m_nRangeX))
    {
        if (g_pInputDevice != 0)
        {
            if (nPrevX != nKeyHighX)
            {
                g_pInputDevice->HandleKeyUp(nPrevX);
            }
            g_pInputDevice->HandleKeyDown(nKeyHighX);
        }
        nVKeyX = nKeyHighX;
    }
    else
    {
        nVKeyX = nPrevX;

        if (nX != nPrevPosX && g_pInputDevice != 0)
        {
            g_pInputDevice->HandleKeyUp(nPrevX);
        }
    }

    g_dwPrevJoyXVKey             = nVKeyX;
    unsigned int       nKeyUp    = *pnKeys;
    const unsigned int nVKeyY    = pnKeys[BINDING_DOWN];
    const unsigned int nPrevPosY = m_pDevices[nDevice].m_infoPrevious.dwYpos;
    const unsigned int nPrevY    = g_dwPrevJoyYVKey;

    if (nY < (unsigned int)(m_pDevices[nDevice].m_nRangeY - m_pDevices[nDevice].m_nHalfRangeX))
    {
        if (g_pInputDevice != 0)
        {
            if (nPrevY != nKeyUp)
            {
                g_pInputDevice->HandleKeyUp(nPrevY);
            }
            g_pInputDevice->HandleKeyDown(nKeyUp);
        }
    }
    else if (nY > (unsigned int)(m_pDevices[nDevice].m_nHalfRangeX + m_pDevices[nDevice].m_nRangeY))
    {
        nKeyUp = nVKeyY;

        if (g_pInputDevice != 0)
        {
            if (nPrevY != nVKeyY)
            {
                g_pInputDevice->HandleKeyUp(nPrevY);
            }
            g_pInputDevice->HandleKeyDown(nVKeyY);
        }
    }
    else
    {
        nKeyUp = nPrevY;

        if (nY != nPrevPosY && g_pInputDevice != 0)
        {
            g_pInputDevice->HandleKeyUp(nPrevY);
        }
    }

    g_dwPrevJoyYVKey = nKeyUp;

    m_pDevices[nDevice].m_infoPrevious.dwXpos = nX;
    m_pDevices[nDevice].m_infoPrevious.dwYpos = nY;
}

int g_nJoystickId1       = -1;
int g_nJoystickId2       = -1;
int g_nJoystickSlotA     = -1;
int g_nJoystickSlotB     = -1;
int g_anGamepadButton[4] = { 4, 6, 5, 7 };

unsigned int g_anJoystickDefaultKeys[12] = {
    VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, 'S', 'D', VK_SPACE, 'C', '1', '2', '3', '4'
};

const char* g_apszGamepadButtonKey[4] = { "Attack", "Speed", "Shoot", "Pass" };
