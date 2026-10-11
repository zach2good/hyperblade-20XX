#ifndef HYPERBLADE_JOYSTICKMGR_H
#define HYPERBLADE_JOYSTICKMGR_H

#include "common/compat.h"
#include "globals.h"

class JoystickDevice;

class JoystickMgr
{
public:
    static void __cdecl StartCapture(int nTwoPlayers);

public:
    JoystickMgr(HWND hWnd, int bCaptureEnabled);

    ~JoystickMgr();
    int CaptureJoystick(UINT uJoyID, HWND hWnd);
    int SetThreshold(UINT uJoyID, int nShift);

    void HandleButtonPress(int nDevice, unsigned int nButton, unsigned int nX, unsigned int nY);
    void HandleButtonRelease(int nDevice, unsigned int nButton, unsigned int nX, unsigned int nY);
    void HandleJoyAxis(int nDevice, unsigned int nFlags, unsigned short nX, unsigned short nY);

    HWND            m_hWnd;            // 0x000
    int             m_bCaptureEnabled; // 0x004
    JoystickDevice* m_pDevices;        // 0x008
    // The driver's id count; m_nDeviceCount is how many of those answered.
    unsigned int m_nDriverDeviceCount; // 0x00c
    unsigned int m_nDeviceCount;       // 0x010
    int          m_nLastButtonCode;    // 0x014, the button combo code the last press mapped to a
                                       // Key, so a release can tell which key to lift.

    static void LoadButtonConfig(void);
};

#endif
