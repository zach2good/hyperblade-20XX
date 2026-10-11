// The joystick device, a cache of what Windows reports about one stick.

#ifndef HYPERBLADE_JOYSTICKDEVICE_H
#define HYPERBLADE_JOYSTICKDEVICE_H

#include "common/compat.h"

class JoystickDevice
{
public:
    JoystickDevice();

    ~JoystickDevice()
    {
    }

    int Poll(UINT uJoyID) throw();

    UINT      m_uJoyID;       // 0x000
    UINT      m_uThreshold;   // 0x004
    int       m_nShift;       // 0x008
    int       m_nHalfRangeX;  // 0x00c
    int       m_nRangeX;      // 0x010
    int       m_nRangeY;      // 0x014
    JOYINFOEX m_info;         // 0x018
    JOYINFOEX m_infoPrevious; // 0x04c
    JOYCAPSA  m_caps;         // 0x080
};

#endif
