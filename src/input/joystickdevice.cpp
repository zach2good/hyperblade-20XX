// Both position blocks are primed so a device never polled still hands joyGetPosEx a valid block.

#include "common/compat.h"
#include "joystickdevice.h"

// FUNCTION: HYPERX 0x00423b60
int JoystickDevice::Poll(UINT uJoyID) throw()
{
    JOYINFOEX* const pInfo = &m_info;

    pInfo->dwSize  = sizeof(JOYINFOEX);
    pInfo->dwFlags = JOY_RETURNALL | JOY_RETURNCENTERED | JOY_USEDEADZONE;

    if (joyGetPosEx(uJoyID, pInfo) == JOYERR_NOERROR)
    {
        m_infoPrevious = *pInfo;

        joyGetDevCapsA(uJoyID, &m_caps, sizeof(JOYCAPSA));

        m_nRangeX = (m_caps.wXmax - m_caps.wXmin) >> 1;
        m_nRangeY = (m_caps.wYmax - m_caps.wYmin) >> 1;

        joyGetThreshold(uJoyID, &m_uThreshold);

        m_uJoyID = uJoyID;

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00423dc0
JoystickDevice::JoystickDevice()
{
    JOYINFOEX* pInfo = &m_info;

    memset(pInfo, 0, sizeof(JOYINFOEX));

    pInfo->dwSize  = sizeof(JOYINFOEX);
    pInfo->dwFlags = JOY_RETURNALL | JOY_RETURNCENTERED | JOY_USEDEADZONE;

    pInfo = &m_infoPrevious;

    memset(pInfo, 0, sizeof(JOYINFOEX));

    pInfo->dwSize  = sizeof(JOYINFOEX);
    pInfo->dwFlags = JOY_RETURNALL | JOY_RETURNCENTERED | JOY_USEDEADZONE;
}
