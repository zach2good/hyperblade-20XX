// symbols/hud_surfaces.csv maps the database's names to these.

#ifndef HYPERBLADE_HUDSURFACES_H
#define HYPERBLADE_HUDSURFACES_H

#include "math/point.h"
#include "graphics/surfacepoint.h"
#include <stddef.h>

class CDDTextSurface;
class CDDBitMap;
class FramedBarBitmap;
class DamageBar;
class ScrollBitmap;
class PassBar;
class Radar;
class String;

// DDraw::ClearDeviceState clears all 0x70 words with one rep stosd.
struct HudSurfaces
{
    CDDTextSurface*  pTeamAScoreText;
    CDDTextSurface*  pTeamBScoreText;
    CDDTextSurface*  pClockText;
    CDDTextSurface*  pSmallText;
    FramedBarBitmap* pSpeedMeter;
    DamageBar*       pTeamADamageBar1;
    DamageBar*       pTeamADamageBar2;
    DamageBar*       pTeamBDamageBar1;
    DamageBar*       pTeamBDamageBar2;
    CDDBitMap*       pNetworkIcon;
    int              anUnusedA[4];
    // Cleared beside the message border and never read anywhere in the image.
    int             nUnusedB;
    CDDBitMap*      pMessageFrame;
    CDDTextSurface* pMessageText;
    CDDBitMap*      pTeamAFrame;
    CDDBitMap*      pTeamBFrame;
    CDDBitMap*      pTeamABitmap;
    CDDBitMap*      pTeamBBitmap;
    int             anUnusedC[2];
    CDDBitMap*      pHeadCamFrame;
    CDDBitMap*      pReticle;
    // The D3D HUD's pass bar. Nothing in the image writes it, so that HUD never shows one.
    PassBar*        pPassBar;
    ScrollBitmap*   pShotPowerMeter;
    Radar*          pRadar;
    CDDBitMap*      pPeriodFrame;
    CDDBitMap*      pPeriodOverlay;
    CDDTextSurface* pExitMatchText;
    CDDTextSurface* pContinueText;
    String*         pExitMatchString;
    String*         pContinueString;
    // Destroyed as runs by the teardown loops.
    CDDTextSurface* apPlayerWidgets[8];
    CDDTextSurface* apTeamNames[2];
    // Nothing writes it, so the 320-wide HUD never shows.
    int bHud320;
    int anUnusedD[0x41];
    int nShotPowerPosition;
    int nHudSurfaceState;
};

// GLOBAL: HYPERX 0x004cd4e8
extern HudSurfaces g_hudSurfaces;

// GLOBAL: HYPERX 0x004cd4d0
extern SurfacePoint g_ptHudMargin;
// Only y is read.
// GLOBAL: HYPERX 0x004cd4d8
extern SurfacePoint g_ptHudInset;

// GLOBAL: HYPERX 0x004cedd8
extern SurfacePoint g_ptHudClockAnchor;

// GLOBAL: HYPERX 0x004cede0
extern Point g_ptHudFpsAnchor;

// The right margin, which is the screen width less ten.
// GLOBAL: HYPERX 0x004ceeb4
extern int g_nHudRightMargin;

#endif
