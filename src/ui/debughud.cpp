#include "common/compat.h"
#include "globals.h"
#include "app/gameapp.h"
#include "hudglobals.h"
#include "common/staticinit.h"
#include "common/pathstring.h"
#include "uiglobals.h"
#include <stdio.h>
#include <string.h>
#include "ui/debughud.h"

// Angles print in degrees modulo a turn; the elevation adds a quarter turn, so down is nought.

#define DEGREES_PER_TURN 360

// GLOBAL: HYPERX 0x004d0990
extern char g_szDebugLightLine[0x64];

// GLOBAL: HYPERX 0x004c67b8
extern const float g_flDebugLightDistance;

// FUNCTION: HYPERX 0x0047ab90
void DebugHUD::UpdateLightingInfo(void)
{
    int nLength;

    // STRING: HYPERX 0x004c67fc
    nLength = sprintf(g_szDebugLightLine, "Light: Distance  %0.2f Attenuation %0.2f", g_flDebugLightDistance, g_flLightAttenuation);

    DebugHUD::PushLightString(g_szDebugLightLine, (short)nLength);

    // STRING: HYPERX 0x004c67c8
    nLength = sprintf(g_szDebugLightLine, "Light: Elevation Angle  %03d Azimuth Angle  %03d", (int)((g_flHalfPi + g_gameApp.m_vLightSpherical.z) / g_flDegToRad) % DEGREES_PER_TURN, (int)(g_gameApp.m_vLightSpherical.x / g_flDegToRad) % DEGREES_PER_TURN);

    DebugHUD::PushLightString(g_szDebugLightLine, (short)nLength);
}

// FUNCTION: HYPERX 0x0047ae10
void DebugHUD::PushLightString(const char* pszSource, short nLength)
{
    if (g_nDebugLineNext == 10)
    {
        g_nDebugLineNext = 0;
    }

    g_anDebugLineLength[g_nDebugLineNext] = nLength;

    memcpy(g_aDebugLineText[g_nDebugLineNext], pszSource, nLength);

    g_nDebugLineNext++;
}
