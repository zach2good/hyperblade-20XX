// Sizes and order come from the destructor's member chain.

#ifndef HYPERBLADE_INFOBAR_H
#define HYPERBLADE_INFOBAR_H

#include "common/compat.h"
#include "ui/infobarbase.h"

#include "ui/cddsurfaceowner.h"

class InfoBarSlot;

#include "graphics/rlespritesurface.h"
#include "graphics/cddtext.h"
#include "common/commonglobals.h"
#include "graphics/infobarradar.h"
#include "graphics/renderer.h"
#include "graphics/infobarslot.h"
#include "graphics/viewextent.h"

class InfoBarBase;

// The two score lines IncrementScore takes.
#define SCORE_LINE_TEAM2 4
#define SCORE_LINE_TEAM1 5

// VTABLE: HYPERX 0x004b32ec
class InfoBar : public InfoBarBase
{
public:
    // Free functions in the original.
    static void IncrementScore(int nWhich);

public:
    ViewExtent GetPassBarSize() override;

    // Twenty one and twenty two are EngineObject::CheckObjectAI and Stub::Assert, shared bare rets.
    void SetPassBarMarker(int) override
    {
    }

    void ShowPassBar() override
    {
    }

    void ShowQuitPrompt() override;
    void HideQuitPrompt() override;
    void BeginTick() override;

    InfoBar();
    ~InfoBar() override;

    // Nothrow, which gives the destructor the original's unwind.
    void Cleanup() noexcept;

    void LayoutSurfaces() override;

    void DrawScene(int bDraw) override;
    void DrawHUD(int bFull) override;

    void SetMessageText(const char* pszText, unsigned long color) override;
    void ShowMessage() override;
    void HideMessage() override;
    void SetHomeLogo(char* pszName) override;
    void SetAwayLogo(char* pszName) override;

    void       ShowHeadCam() override;
    void       SetHeadCamText(const char* pszText) override;
    ViewExtent GetHeadCamSize() override;
    ViewExtent GetHeadCamOrigin() override;
    void       SetShotPower(float flPercent) override;
    void       ShowShotPower() override;
    void       SetReticlePos(int nX, int nY) override;
    ViewExtent GetReticleSize() override;
    void       SetReticleState(int nMode) override;

    void DrawPeriodLight();

    // Never reads this object.
    void UpdateDamageBar(InfoBarSlot* pBar, int nSlot);

    void UpdateRokValue();
    void UpdateOwnTeamDisplay();
    void UpdateFoeTeamDisplay();

    // The constructor leaves them at minus one.
    int              m_nCachedHomeTeam;  // 0x00c
    int              m_nCachedAwayTeam;  // 0x010
    int              m_nCachedArena;     // 0x014
    RLESpriteSurface m_sPeriodFrame;     // 0x018
    CDDText          m_tClock;           // 0x03c
    CDDText          m_tFrameRate;       // 0x064
    RLESpriteSurface m_sSpeedFrame;      // 0x08c
    CDDSurfaceOwner  m_oSpeedMeter;      // 0x0b0
    CDDText          m_tHomeScore;       // 0x0f0
    RLESpriteSurface m_sTeamAFrame;      // 0x118
    RLESpriteSurface m_sTeamALogo;       // 0x13c
    InfoBarSlot      m_kTeamADamage1;    // 0x160
    InfoBarSlot      m_kTeamADamage2;    // 0x198
    CDDText          m_tAwayScore;       // 0x1d0
    RLESpriteSurface m_sTeamBFrame;      // 0x1f8
    RLESpriteSurface m_sTeamBLogo;       // 0x21c
    InfoBarSlot      m_kTeamBDamage1;    // 0x240
    InfoBarSlot      m_kTeamBDamage2;    // 0x278
    CDDText          m_tRokValue;        // 0x2b0
    RLESpriteSurface m_sNetworkIcon;     // 0x2d8
    int              m_bMessageVisible;  // 0x2fc
    RLESpriteSurface m_sMessageFrame;    // 0x300
    CDDText          m_tMessage;         // 0x324
    RLESpriteSurface m_sRadarFrame;      // 0x34c
    InfoBarRadar     m_radar;            // 0x370
    int              m_bHeadCamPending;  // 0x3b0
    RLESpriteSurface m_sHeadCamFrame;    // 0x3b4
    CDDText          m_tHeadCamText;     // 0x3d8
    int              m_bShotPowerWanted; // 0x400
    int              m_bReticleVisible;  // 0x404
    RLESpriteSurface m_sReticle;         // 0x408
    CDDSurfaceOwner  m_oShotPower;       // 0x42c
    CDDText          m_tShotCameraLabel; // 0x46c
    CDDText          m_tExitMatchPrompt; // 0x494
    CDDText          m_tContinuePrompt;  // 0x4bc
    CDDText          m_atOwnRoster[4];   // 0x4e4
    CDDText          m_atFoeRoster[4];   // 0x584
    CDDText          m_atTeamNames[2];   // 0x624

    // Past the last destroyed member: the allocation is 0x67c.
    int m_bShotPowerDrawn; // 0x674
    int m_bHeadCamTextDue; // 0x678

    void ForgetPalette()
    {
        m_nCachedHomeTeam = -1;
        m_nCachedAwayTeam = -1;
        m_nCachedArena    = -1;
    }
};

// SYNTHETIC: HYPERX 0x00451cc0
// InfoBar::`scalar deleting destructor'

#endif
