#include "common/compat.h"
#include "graphics/hudlayer.h"
#include "globals.h"
#include "app/gameapp.h"
#include "app/frametimer.h"
#include "infobar.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "graphics/cddtext.h"
#include "engine/objectstats.h"
#include "infobarteams.h"
#include "planvm/damagegate.h"
#include "engine/teamplayerrow.h"
#include "graphics/colorpalette.h"
#include "math/vector.h"
#include "hudglobals.h"
#include "common/pathstring.h"
#include "graphics/rlespritesurface.h"
#include "engine/messagequeue.h"
#include "graphics/renderer.h"
#include "engine/rokobject.h"
#include "engine/playerobject.h"
#include "common/stringpool.h"
#include "uiglobals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graphics/viewextent.h"
#include "graphics/infobarslot.h"
#include "ui/cddsurfaceowner.h"
#include "math/point.h"
#include "ui/infobarbase.h"
#include "physics/collisionellipsoid.h"
#include "graphics/swrend.h"
#include "engine/statids.h"
#include "app/stringids.h"

#define PERIOD_LAMP_COUNT 3

// Where each of the three lamps sits, as an offset from the bar's own left edge.
// GLOBAL: HYPERX 0x004c2f60
extern int g_anPeriodLightOffset[3];

#define PERIOD_LAMP_TOP 0x28

#define PERIOD_LAMP_SIDE 9

// GLOBAL: HYPERX 0x004cef64
extern int g_nPeriodLightColour;

#define METRIC_ROW_GAP 0x19

// The count prints unsigned; its sign only picks the vertical offset. A zero count stops drawing.

#define ROK_LABEL_POSITIVE 0x20
#define ROK_LABEL_NEGATIVE 0x2f

#define ROK_TEXT_BUFFER 0x50

// The info bar's own cache of the last rok value drawn.
// GLOBAL: HYPERX 0x004cef54
extern int g_nInfoBarLastRokValue;

// FUNCTION: HYPERX 0x0044fc70
void InfoBar::DrawPeriodLight()
{
    m_sPeriodFrame.Draw();

    int       nX = m_sPeriodFrame.m_ptPosition.x;
    const int nY = m_sPeriodFrame.m_ptPosition.y;

    int nLamp = g_pGameEngine->m_clock.nPeriod - 1;

    if (nLamp < 0)
    {
        nLamp = 0;
    }
    else if (nLamp >= PERIOD_LAMP_COUNT)
    {
        nLamp = PERIOD_LAMP_COUNT - 1;
    }

    nX += g_anPeriodLightOffset[nLamp];

    nLamp = g_nPeriodLightColour;

    SWRend::FillRect(nX, nY + PERIOD_LAMP_TOP, PERIOD_LAMP_SIDE, PERIOD_LAMP_SIDE, nLamp);
}

// FUNCTION: HYPERX 0x00450160
void InfoBar::UpdateRokValue()
{
    char szText[ROK_TEXT_BUFFER];

    RokObject* const pRok = g_pGameEngine->GetRok();

    if (pRok != 0)
    {
        const int nRokValue = pRok->m_wRokValue;

        if (nRokValue != 0)
        {
            if (nRokValue != g_nInfoBarLastRokValue)
            {
                g_nInfoBarLastRokValue = nRokValue;

                // STRING: HYPERX 0x004c0d6c
                sprintf(szText, "x%d", abs(nRokValue));

                CDDText* const pLabel = &m_tRokValue;

                pLabel->m_ptDrawOffset.x = 0;

                if (nRokValue > 0)
                {
                    pLabel->m_ptDrawOffset.y = ROK_LABEL_POSITIVE;
                }
                else
                {
                    pLabel->m_ptDrawOffset.y = ROK_LABEL_NEGATIVE;
                }

                pLabel->SetText(szText, (COLORREF)0xffff);
            }

            m_tRokValue.Draw();
        }
    }
}

// GLOBAL: HYPERX 0x004cef60
extern int g_nDamageBarColourLit;

// GLOBAL: HYPERX 0x004cef5c
extern int g_nDamageBarColourDim;

// Never reads its own object.
// FUNCTION: HYPERX 0x004501f0
void InfoBar::UpdateDamageBar(InfoBarSlot* pBar, int nSlot)
{
    PlayerObject* const pObject = g_pGameEngine->GetPlayer((short)nSlot);

    if (pObject != 0)
    {
        if (g_pGameEngine->m_nViewedSlot == nSlot)
        {
            pBar->m_nColour = g_nDamageBarColourLit;
        }
        else
        {
            pBar->m_nColour = g_nDamageBarColourDim;
        }

        const double dScaled = pObject->m_stats.GetScalar(RATING_DAMAGE) * 0.01f;

        const int nFrameCount = pBar->m_nFrameCount;

        const double dFraction = dScaled * nFrameCount;

        pBar->m_flScalar = (float)dFraction;

        int nFrame = (int)dFraction;

        if (nFrame >= nFrameCount)
        {
            nFrame = nFrameCount - 1;
        }

        if (nFrame < 0)
        {
            nFrame = 0;
        }

        pBar->m_nFrame = nFrame;

        pBar->Draw();
    }
}

// All six start at minus one, which no display word or damage can be.
// GLOBAL: HYPERX 0x004c2f6c
extern int g_nInfoBarOwnHomeTeam;

// GLOBAL: HYPERX 0x004c2f70
extern int g_nInfoBarOwnAwayTeam;

// GLOBAL: HYPERX 0x004c2f74
extern int g_nInfoBarOwnViewedSlot;

// GLOBAL: HYPERX 0x004c2f78
extern int g_anInfoBarOwnFormation[2];

// GLOBAL: HYPERX 0x004c2f80
extern int g_anInfoBarOwnDamage[4];

// Reads 0 1 3 4, so slot two is skipped.
// GLOBAL: HYPERX 0x004c2f90
extern int g_anOwnFormationSlotMap[4];

// Blank at or below the damage gate; yellow followed, green reserve, red otherwise.
// FUNCTION: HYPERX 0x00450270
void InfoBar::UpdateOwnTeamDisplay()
{
    char             szText[TEAM_LABEL_BUFFER];
    const TeamBlock* pTeam;
    const TeamBlock* pOther;
    int              aFormation[2];
    int              aDamage[TEAM_LABEL_COUNT];
    int              nColour;
    int              i;

    short nSlot = 0;

    if (g_pGameEngine == 0)
    {
        return;
    }

    int       bDirty      = 0;
    const int nViewedSlot = g_pGameEngine->m_nViewedSlot;

    if ((nViewedSlot & ENGINE_SIDE_SWAPPED) != 0)
    {
        pOther = g_pGameEngine->m_pTeamA;
        pTeam  = g_pGameEngine->m_pTeamB;
        nSlot  = 1;

        if (g_nInfoBarOwnAwayTeam != g_pGameEngine->m_nTeamAwayIndex)
        {
            bDirty                = 1;
            g_nInfoBarOwnAwayTeam = g_pGameEngine->m_nTeamAwayIndex;
        }
    }
    else
    {
        pOther = g_pGameEngine->m_pTeamB;
        pTeam  = g_pGameEngine->m_pTeamA;

        if (g_nInfoBarOwnHomeTeam != g_pGameEngine->m_nTeamHomeIndex)
        {
            bDirty                = 1;
            g_nInfoBarOwnHomeTeam = g_pGameEngine->m_nTeamHomeIndex;
        }
    }

    if (nViewedSlot != g_nInfoBarOwnViewedSlot)
    {
        g_nInfoBarOwnViewedSlot = nViewedSlot;
        bDirty                  = 1;
    }

    aFormation[0] = pTeam->aSeat[0];
    aFormation[1] = pTeam->aSeat[1];

    if (aFormation[0] != g_anInfoBarOwnFormation[0] || aFormation[1] != g_anInfoBarOwnFormation[1])
    {
        g_anInfoBarOwnFormation[0] = aFormation[0];
        g_anInfoBarOwnFormation[1] = aFormation[1];
        bDirty                     = 1;
    }

    PlayerObject* pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[0] = (int)pObject->m_stats.GetScalar(RATING_DAMAGE);

    nSlot += 2;

    pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[1] = (int)pObject->m_stats.GetScalar(RATING_DAMAGE);

    ReserveRows* const pReserves = g_pGameEngine->GetReserveRows(pTeam);

    aDamage[2] = (int)pTeam->aRows[pReserves->anRow[0]].aStat[RATING_DAMAGE];
    aDamage[3] = (int)pTeam->aRows[pReserves->anRow[1]].aStat[RATING_DAMAGE];

    if (aDamage[0] != g_anInfoBarOwnDamage[0] || aDamage[1] != g_anInfoBarOwnDamage[1] || aDamage[2] != g_anInfoBarOwnDamage[2] || aDamage[3] != g_anInfoBarOwnDamage[3])
    {
        g_anInfoBarOwnDamage[0] = aDamage[0];
        g_anInfoBarOwnDamage[1] = aDamage[1];
        g_anInfoBarOwnDamage[2] = aDamage[2];
        g_anInfoBarOwnDamage[3] = aDamage[3];
        bDirty                  = 1;
    }

    if (bDirty != 0)
    {
        const int nFollowed = ((unsigned int)nViewedSlot >> 1) & 1;

        for (i = 0; i < TEAM_LABEL_COUNT; i++)
        {
            if ((float)aDamage[i] <= DamageGate())
            {
                nColour = TEAM_COLOUR_ABSENT;
            }
            else if (nFollowed == i)
            {
                nColour = TEAM_COLOUR_FOLLOWED;
            }
            else if (i < TEAM_LABEL_ON_DROME)
            {
                nColour = TEAM_COLOUR_ON_DROME;
            }
            else
            {
                nColour = TEAM_COLOUR_RESERVE;
            }

            if (i >= TEAM_LABEL_ON_DROME)
            {
                sprintf(szText, "%c. %s", i - TEAM_LABEL_ON_DROME + '1', pTeam->GetRosterName(g_anOwnFormationSlotMap[pReserves->anRow[i - TEAM_LABEL_ON_DROME]]));
            }
            else
            {
                strcpy(szText,
                       pTeam->GetRosterName(g_anOwnFormationSlotMap[aFormation[i]]));
            }

            m_atOwnRoster[i].SetText(szText, (COLORREF)nColour);
        }

        m_atTeamNames[0].SetText(pTeam->GetName(), (COLORREF)TEAM_COLOUR_RESERVE);
        m_atTeamNames[1].SetText(pOther->GetName(), (COLORREF)TEAM_COLOUR_RESERVE);
    }

    for (i = 0; i < TEAM_LABEL_COUNT; i++)
    {
        if (aDamage[i] > 0)
        {
            m_atOwnRoster[i].Draw();
        }
    }

    m_atTeamNames[0].Draw();
    m_atTeamNames[1].Draw();
}

// GLOBAL: HYPERX 0x004c2fa0
extern int g_nInfoBarFoeHomeTeam;

// GLOBAL: HYPERX 0x004c2fa4
extern int g_nInfoBarFoeAwayTeam;

// GLOBAL: HYPERX 0x004c2fa8
extern int g_anInfoBarFoeFormation[2];

// GLOBAL: HYPERX 0x004c2fb0
extern int g_anInfoBarFoeDamage[4];

// GLOBAL: HYPERX 0x004c2fc0
extern int g_anFoeFormationSlotMap[4];

// The viewed team's panel written again, minus the follow colour, reserve numbers and team names.
// FUNCTION: HYPERX 0x00450600
void InfoBar::UpdateFoeTeamDisplay()
{
    char             szText[TEAM_LABEL_BUFFER];
    const TeamBlock* pTeam;
    int              aFormation[2];
    int              aDamage[TEAM_LABEL_COUNT];
    int              nColour;
    int              i;

    short nSlot = 0;

    if (g_pGameEngine == 0)
    {
        return;
    }

    int bDirty = 0;

    if ((g_pGameEngine->m_nViewedSlot & ENGINE_SIDE_SWAPPED) != 0)
    {
        pTeam = g_pGameEngine->m_pTeamA;

        if (g_nInfoBarFoeHomeTeam != g_pGameEngine->m_nTeamHomeIndex)
        {
            bDirty                = 1;
            g_nInfoBarFoeHomeTeam = g_pGameEngine->m_nTeamHomeIndex;
        }
    }
    else
    {
        pTeam = g_pGameEngine->m_pTeamB;
        nSlot = 1;

        if (g_nInfoBarFoeAwayTeam != g_pGameEngine->m_nTeamAwayIndex)
        {
            bDirty                = 1;
            g_nInfoBarFoeAwayTeam = g_pGameEngine->m_nTeamAwayIndex;
        }
    }

    aFormation[0] = pTeam->aSeat[0];
    aFormation[1] = pTeam->aSeat[1];

    if (aFormation[0] != g_anInfoBarFoeFormation[0] || aFormation[1] != g_anInfoBarFoeFormation[1])
    {
        g_anInfoBarFoeFormation[0] = aFormation[0];
        g_anInfoBarFoeFormation[1] = aFormation[1];
        bDirty                     = 1;
    }

    PlayerObject* pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[0] = (int)pObject->m_stats.GetScalar(RATING_DAMAGE);

    nSlot += 2;

    pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[1] = (int)pObject->m_stats.GetScalar(RATING_DAMAGE);

    ReserveRows* const pReserves = g_pGameEngine->GetReserveRows(pTeam);

    aDamage[2] = (int)pTeam->aRows[pReserves->anRow[0]].aStat[RATING_DAMAGE];
    aDamage[3] = (int)pTeam->aRows[pReserves->anRow[1]].aStat[RATING_DAMAGE];

    if (aDamage[0] != g_anInfoBarFoeDamage[0] || aDamage[1] != g_anInfoBarFoeDamage[1] || aDamage[2] != g_anInfoBarFoeDamage[2] || aDamage[3] != g_anInfoBarFoeDamage[3])
    {
        g_anInfoBarFoeDamage[0] = aDamage[0];
        g_anInfoBarFoeDamage[1] = aDamage[1];
        g_anInfoBarFoeDamage[2] = aDamage[2];
        g_anInfoBarFoeDamage[3] = aDamage[3];
        bDirty                  = 1;
    }

    if (bDirty != 0)
    {
        for (i = 0; i < TEAM_LABEL_COUNT; i++)
        {
            if ((float)aDamage[i] <= DamageGate())
            {
                nColour = TEAM_COLOUR_ABSENT;
            }
            else if (i < TEAM_LABEL_ON_DROME)
            {
                nColour = TEAM_COLOUR_ON_DROME;
            }
            else
            {
                nColour = TEAM_COLOUR_RESERVE;
            }

            if (i >= TEAM_LABEL_ON_DROME)
            {
                strcpy(szText,
                       pTeam->GetRosterName(g_anFoeFormationSlotMap[pReserves->anRow[i - TEAM_LABEL_ON_DROME]]));
            }
            else
            {
                strcpy(szText,
                       pTeam->GetRosterName(g_anFoeFormationSlotMap[aFormation[i]]));
            }

            m_atFoeRoster[i].SetText(szText, (COLORREF)nColour);
        }
    }

    for (i = 0; i < TEAM_LABEL_COUNT; i++)
    {
        if (aDamage[i] > 0)
        {
            m_atFoeRoster[i].Draw();
        }
    }
}

#define TEXT_COLOUR 0xffff

// GLOBAL: HYPERX 0x004cefe0
extern int g_nInfoBarLastSeconds;

// GLOBAL: HYPERX 0x004c2fd0
extern int g_nInfoBarLastHomeScore;

// GLOBAL: HYPERX 0x004c2fd4
extern int g_nInfoBarLastAwayScore;

// Slot four. The quit prompts hide once five seconds have passed since they went up.
// FUNCTION: HYPERX 0x004508f0
void InfoBar::DrawHUD(int bFull)
{
    char szText[CLOCK_BUFFER];

    const int nElapsed = g_pGameEngine->m_nPeriodLengthMs - g_pGameEngine->m_clock.nPauseStartMs + g_pGameEngine->m_clock.nPeriodStartMs;

    const int nSeconds = (int)((nElapsed % MS_PER_MINUTE) * 0.001f);
    const int nMinutes = (int)(nElapsed * 1.6666667e-005f);

    const int nActive = g_frameTimer.bFrameTimesOn;

    if (g_nInfoBarLastSeconds != nSeconds || nSeconds == 0)
    {
        g_nInfoBarLastSeconds = nSeconds;

        // STRING: HYPERX 0x004c0d70
        sprintf(szText, "%d:%02d", nMinutes, nSeconds);

        m_tClock.SetText(szText, (COLORREF)TEXT_COLOUR);

        if (nActive != 0)
        {
            m_tFrameRate.SetText(g_szFpsText, (COLORREF)TEXT_COLOUR);
        }
    }

    m_tClock.Draw();

    if (nActive != 0)
    {
        m_tFrameRate.Draw();
    }

    if (bFull == 0)
    {
        return;
    }

    int nScore = g_pGameEngine->GetHomeScore();

    if (nScore != g_nInfoBarLastHomeScore)
    {
        // STRING: HYPERX 0x004bb814
        sprintf(szText, "%d", nScore);

        m_tHomeScore.SetText(szText, (COLORREF)TEXT_COLOUR);

        g_nInfoBarLastHomeScore = nScore;
    }

    m_tHomeScore.Draw();

    nScore = g_pGameEngine->GetAwayScore();

    if (nScore != g_nInfoBarLastAwayScore)
    {
        // STRING: HYPERX 0x004bb814
        sprintf(szText, "%d", nScore);

        m_tAwayScore.SetText(szText, (COLORREF)TEXT_COLOUR);

        g_nInfoBarLastAwayScore = nScore;
    }

    m_tAwayScore.Draw();

    UpdateRokValue();

    if (m_bMessageVisible != 0)
    {
        m_tMessage.Draw();
    }

    if (m_bQuitPromptVisible != 0)
    {
        if (g_pGameEngine->m_clock.nTickMs - m_nQuitPromptTick >= QUIT_PROMPT_INTERVAL)
        {
            HideQuitPrompt();
        }
        else
        {
            m_tExitMatchPrompt.Draw();
            m_tContinuePrompt.Draw();
        }
    }

    if (m_bShotPowerDrawn != 0)
    {
        m_tShotCameraLabel.Draw();
    }
    else if (m_bHeadCamTextDue != 0)
    {
        m_tHeadCamText.Draw();
    }

    UpdateOwnTeamDisplay();
    UpdateFoeTeamDisplay();
}

#define ENGINE_FOLLOW_LIMIT 6

#define COLOUR_BLIP_DROMER0 0xc8c800
#define COLOUR_BLIP_DROMER2 0xffff00
#define COLOUR_BLIP_DROMER1 0xc8
#define COLOUR_BLIP_DROMER3 0xff
#define COLOUR_BLIP_ROK     0xffff
#define COLOUR_PERIOD_LIGHT 0xff
#define COLOUR_BAR_LIT      0xffff
#define COLOUR_BAR_DIM      0xc8
#define COLOUR_RADAR_LINE   0xe0ffff

// A null engine triggers the re-resolve, which then dereferences it, as in the original.
// FUNCTION: HYPERX 0x00450b80
void InfoBar::DrawScene(int bDraw)
{
    if (g_pBackBufferBitsCopy != 0)
    {
        if (g_pGameEngine == 0 || m_nCachedHomeTeam != g_pGameEngine->m_nTeamHomeIndex || m_nCachedAwayTeam != g_pGameEngine->m_nTeamAwayIndex || m_nCachedArena != g_pGameEngine->m_nArenaIndex)
        {
            m_nCachedHomeTeam = g_pGameEngine->m_nTeamHomeIndex;
            m_nCachedAwayTeam = g_pGameEngine->m_nTeamAwayIndex;
            m_nCachedArena    = g_pGameEngine->m_nArenaIndex;

            g_nRadarBlipColourDromer0 = g_pColorPalette->NearestIndex(COLOUR_BLIP_DROMER0, HudLayer::KEY_INDEX);
            g_nRadarBlipColourDromer2 = g_pColorPalette->NearestIndex(COLOUR_BLIP_DROMER2, HudLayer::KEY_INDEX);
            g_nRadarBlipColourDromer1 = g_pColorPalette->NearestIndex(COLOUR_BLIP_DROMER1, HudLayer::KEY_INDEX);
            g_nRadarBlipColourDromer3 = g_pColorPalette->NearestIndex(COLOUR_BLIP_DROMER3, HudLayer::KEY_INDEX);
            g_nRadarBlipColourRok     = g_pColorPalette->NearestIndex(COLOUR_BLIP_ROK, HudLayer::KEY_INDEX);
            g_nPeriodLightColour      = g_pColorPalette->NearestIndex(COLOUR_PERIOD_LIGHT, HudLayer::KEY_INDEX);
            g_nDamageBarColourLit     = g_pColorPalette->NearestIndex(COLOUR_BAR_LIT, HudLayer::KEY_INDEX);
            g_nDamageBarColourDim     = g_pColorPalette->NearestIndex(COLOUR_BAR_DIM, HudLayer::KEY_INDEX);
            g_nRadarLineColour        = g_pColorPalette->NearestIndex(COLOUR_RADAR_LINE, HudLayer::KEY_INDEX);
        }

        DrawPeriodLight();

        if (bDraw != 0)
        {
            const int nFollowed = g_pGameEngine->m_nViewedSlot;

            if (nFollowed < ENGINE_FOLLOW_LIMIT)
            {
                PlayerObject* const pObject = g_pGameEngine->GetPlayer((short)nFollowed);

                if (pObject != 0)
                {
                    CDDSurfaceOwner* const pPanel = &m_oSpeedMeter;

                    const int nFrameCount = pPanel->m_nFrameCount;

                    const double dFraction = pObject->m_stats.GetSpeedRatio() * nFrameCount;

                    pPanel->m_flScalar = (float)dFraction;

                    int nFrame = (int)dFraction;

                    if (nFrame >= nFrameCount)
                    {
                        nFrame = nFrameCount - 1;
                    }

                    if (nFrame < 0)
                    {
                        nFrame = 0;
                    }

                    pPanel->m_nFrame = nFrame;

                    m_oSpeedMeter.Draw();
                }
            }

            m_sSpeedFrame.Draw();

            UpdateDamageBar(&m_kTeamADamage1, 0);
            UpdateDamageBar(&m_kTeamADamage2, 2);

            m_sTeamALogo.Draw();
            m_sTeamAFrame.Draw();

            UpdateDamageBar(&m_kTeamBDamage1, 1);
            UpdateDamageBar(&m_kTeamBDamage2, 3);

            m_sTeamBLogo.Draw();
            m_sTeamBFrame.Draw();

            if (m_bMessageVisible != 0)
            {
                m_sMessageFrame.Draw();
            }

            m_radar.Draw();
            m_sRadarFrame.Draw();

            m_bShotPowerDrawn = 0;
            m_bHeadCamTextDue = 0;

            if (m_bHeadCamPending != 0)
            {
                if (m_bShotPowerWanted != 0)
                {
                    m_oShotPower.Draw();

                    m_bShotPowerDrawn = 1;
                }
                else
                {
                    m_bHeadCamTextDue = 1;
                }

                if (m_bReticleVisible != 0)
                {
                    m_sReticle.Draw();
                }

                m_sHeadCamFrame.Draw();
            }
        }
    }
}

// The two lists each stack four entries ten words apart by height, two pixels between.

// FUNCTION: HYPERX 0x00450e60
void InfoBar::LayoutSurfaces()
{
    if (g_nBackBufferHeight == 0 || g_nBackBufferWidth == 0)
    {
        return;
    }

    // Every position is in the HUD layer, which is a fraction of the back buffer.
    const HudLayer::Scope layer;

    SurfacePoint ptPos;
    SurfacePoint ptSize;

    // The period frame is centred along the top, with the clock inside it.
    ptSize = m_sPeriodFrame.GetSize();

    ptPos.x = (g_nBackBufferWidth - ptSize.x) / 2;
    ptPos.y = 2;
    m_sPeriodFrame.SetPosition(ptPos);

    ptPos.x += 0x19;
    ptPos.y = 9;
    m_tClock.SetPosition(ptPos);

    ptPos.x = 0;
    ptPos.y = 0;
    m_sSpeedFrame.SetPosition(ptPos);
    m_oSpeedMeter.SetPosition(ptPos);

    // The team names and rosters run down the left edge below the speed frame.
    ptSize = m_sSpeedFrame.GetSize();

    SurfacePoint ptRoster;

    ptRoster.y = ptSize.y + ptPos.y;
    ptRoster.x = ptPos.x + 6;

    ptPos.x = 0x14;
    ptPos.y = ptSize.y + 0x14;
    m_tFrameRate.SetPosition(ptPos);

    ptSize = m_sTeamAFrame.GetSize();

    Point ptFrame;

    ptFrame.x = 0;
    ptFrame.y = g_nBackBufferHeight - ptSize.y * 2 - 3;
    m_sTeamAFrame.SetPosition(ptFrame);

    Point ptBar;

    ptBar.x = 0x4e;
    ptBar.y = ptFrame.y + 0x18;
    m_kTeamADamage1.SetPosition(ptBar);

    ptBar.y += 0xc;
    m_kTeamADamage2.SetPosition(ptBar);

    ptPos   = ptFrame;
    ptPos.x = 2;
    ptPos.y += 0x16;
    m_sTeamALogo.SetPosition(ptPos);

    Point ptScore;

    ptScore.x = ptSize.x - 0x20;
    ptScore.y = ptFrame.y + 0xa;
    m_tHomeScore.SetPosition(ptScore);

    ptScore.x -= 3;
    ptScore.y -= 8;
    m_tRokValue.SetPosition(ptScore);

    // The second team's strip is placed by the first team's frame height, as in the original.
    ptFrame.x = 0;
    ptFrame.y = g_nBackBufferHeight - ptSize.y - 5;
    m_sTeamBFrame.SetPosition(ptFrame);

    ptBar.x = 0x4e;
    ptBar.y = ptFrame.y + 4;
    m_kTeamBDamage1.SetPosition(ptBar);

    ptBar.y += 0xc;
    m_kTeamBDamage2.SetPosition(ptBar);

    ptPos   = ptFrame;
    ptPos.x = 2;
    ptPos.y += 4;
    m_sTeamBLogo.SetPosition(ptPos);

    ptScore.x = ptSize.x - 0x20;
    ptScore.y = ptFrame.y + 0x19;
    m_tAwayScore.SetPosition(ptScore);

    Point ptMessage;

    ptMessage.x = ptSize.x - 0x14;
    ptMessage.y = ptFrame.y - 0xc;
    m_sMessageFrame.SetPosition(ptMessage);

    ptMessage.x += 6;
    ptMessage.y += 5;
    m_tMessage.SetPosition(ptMessage);

    ptSize = m_sRadarFrame.GetSize();

    ptPos.x = g_nBackBufferWidth - ptSize.x - 0x14;
    ptPos.y = g_nBackBufferHeight - ptSize.y - 0x14;
    m_sRadarFrame.SetPosition(ptPos);
    m_radar.SetPosition(ptPos);

    ptSize = m_sNetworkIcon.GetSize();

    ptPos.x = g_nBackBufferWidth - ptSize.x - 0x14;
    ptPos.y = 0x14;
    m_sNetworkIcon.SetPosition(ptPos);

    // The head camera sits in the top right corner, its caption and the shot meter beside it.
    ptSize = m_sHeadCamFrame.GetSize();

    ptPos.x = g_nBackBufferWidth - ptSize.x;
    ptPos.y = 0;
    m_sHeadCamFrame.SetPosition(ptPos);

    const Point ptCamera = ptPos;

    ptPos.x += 0x1e;
    ptPos.y = ptSize.y;
    m_tHeadCamText.SetPosition(ptPos);
    m_tShotCameraLabel.SetPosition(ptPos);

    m_sReticle.m_ptPosition = ptPos = GetHeadCamOrigin();
    ptPos                           = ptCamera;
    m_oShotPower.SetPosition(ptPos);
    m_oShotPower.m_ptDrawOffset.x = 0x23;
    m_oShotPower.m_ptDrawOffset.y = 0x10;

    // The two prompts are centred, the continue prompt below the exit prompt.
    ptSize = m_tExitMatchPrompt.GetSize();

    ptPos.x = (g_nBackBufferWidth - ptSize.x) / 2;
    ptPos.y = (g_nBackBufferHeight - ptSize.y) / 2;
    m_tExitMatchPrompt.SetPosition(ptPos);

    ptSize = m_tContinuePrompt.GetSize();

    ptPos.x = (g_nBackBufferWidth - ptSize.x) / 2;
    ptPos.y = (g_nBackBufferHeight - ptSize.y) / 2;

    ptSize = m_tExitMatchPrompt.GetSize();

    ptPos.y += ptSize.y + 6;
    m_tContinuePrompt.SetPosition(ptPos);

    m_atTeamNames[0].SetPosition(ptRoster);

    ptSize = m_atTeamNames[0].GetSize();

    int nY = ptRoster.y + ptSize.y + 2;

    for (int i = 0; i < 4; i++)
    {
        m_atOwnRoster[i].m_ptPosition.x = ptRoster.x;
        m_atOwnRoster[i].m_ptPosition.y = nY;

        ptSize = m_atOwnRoster[i].GetSize();

        nY += ptSize.y + 2;

        // A wider gap separates the first two players from the last two.
        if (i == 1)
        {
            nY += 6;
        }
    }

    nY += 0x1c;
    m_atTeamNames[1].m_ptPosition.x = ptRoster.x;
    m_atTeamNames[1].m_ptPosition.y = nY;

    ptSize = m_atTeamNames[1].GetSize();

    nY += ptSize.y + 2;

    for (int j = 0; j < 4; j++)
    {
        m_atFoeRoster[j].m_ptPosition.x = ptRoster.x;
        m_atFoeRoster[j].m_ptPosition.y = nY;

        ptSize = m_atFoeRoster[j].GetSize();

        nY += ptSize.y + 2;

        if (j == 1)
        {
            nY += 6;
        }
    }
}

// All eighteen fonts read the face from the global rather than the literal.

// GLOBAL: HYPERX 0x004cef7c
extern char g_abInfoBarFontName[0x50];

#define INFOBAR_NAME_BUFFER 0x50

#define FONT_TIMER 9
#define FONT_LABEL 10
#define FONT_PAIR  11
#define FONT_BODY  15
#define FONT_LARGE 20

#define COLOUR_YELLOW 0xffff
#define COLOUR_UNSET  (-1)

// The damage bar strip: twelve frames, the ninth showing, drawn two by eight.
#define BAR_FRAME_COUNT 0x0c
#define BAR_UNIT_WIDTH  9
#define BAR_MIN_HEIGHT  2
#define BAR_MAX_HEIGHT  8

// FUNCTION: HYPERX 0x00451280
InfoBar::InfoBar()
{
    char szName[INFOBAR_NAME_BUFFER];
    Vec3 vLimits;
    int  i;

    strcpy(g_abInfoBarFontName, "MS Sans Serif");

    m_sPeriodFrame.LoadByName("frper");

    m_tClock.CreateFontFace(g_abInfoBarFontName, FONT_LARGE);
    m_tFrameRate.CreateFontFace(g_abInfoBarFontName, FONT_BODY);

    sprintf(szName, "fr%s", "speed");

    m_sSpeedFrame.LoadByName(szName);

    // STRING: HYPERX 0x004c2fe0
    sprintf(szName, "%s01", "speed");

    // This class's LoadByName at 0x0044f8f0; RLESpriteSurface's writes other fields.
    m_oSpeedMeter.LoadByName(szName);

    const Point ptSpeedSize = m_oSpeedMeter.m_ptSize;

    m_oSpeedMeter.SetStrip(ptSpeedSize.x, 1, 0, 0);

    m_tHomeScore.CreateFontFace(g_abInfoBarFontName, FONT_BODY);

    m_sTeamAFrame.LoadByName("frdamgA");

    m_kTeamADamage1.SetStrip(BAR_FRAME_COUNT, BAR_UNIT_WIDTH, BAR_MIN_HEIGHT, BAR_MAX_HEIGHT);
    m_kTeamADamage2.SetStrip(BAR_FRAME_COUNT, BAR_UNIT_WIDTH, BAR_MIN_HEIGHT, BAR_MAX_HEIGHT);

    m_tAwayScore.CreateFontFace(g_abInfoBarFontName, FONT_BODY);

    m_sTeamBFrame.LoadByName("frdamgB");

    m_kTeamBDamage1.SetStrip(BAR_FRAME_COUNT, BAR_UNIT_WIDTH, BAR_MIN_HEIGHT, BAR_MAX_HEIGHT);
    m_kTeamBDamage2.SetStrip(BAR_FRAME_COUNT, BAR_UNIT_WIDTH, BAR_MIN_HEIGHT, BAR_MAX_HEIGHT);

    m_tRokValue.CreateFontFace(g_abInfoBarFontName, FONT_TIMER);

    HideMessage();

    m_sMessageFrame.LoadByName("frmsg");

    m_tMessage.CreateFontFace(g_abInfoBarFontName, FONT_BODY);

    m_sRadarFrame.LoadByName("frradar");

    {
        SurfacePoint ptRadarSize(100, 100);

        vLimits.x = g_arenaEllipsoid.GetRadiusX();
        vLimits.y = g_arenaEllipsoid.GetRadiusY();
        vLimits.z = g_arenaEllipsoid.GetRadiusZ();

        m_radar.ComputeScale(&m_sRadarFrame, vLimits);
    }

    m_sNetworkIcon.LoadByName("network");

    m_bHeadCamPending = 0;

    m_sHeadCamFrame.LoadByName("frshot");

    m_tHeadCamText.CreateFontFace(g_abInfoBarFontName, FONT_LABEL);

    m_tHeadCamText.SetColour(COLOUR_YELLOW);

    CDDText* pLabel = &m_tShotCameraLabel;

    pLabel->CreateFontFace(g_abInfoBarFontName, FONT_LABEL);

    pLabel->SetText("Shot Camera", (COLORREF)COLOUR_YELLOW);

    m_bShotPowerWanted = 0;
    SetReticleState(0);

    // STRING: HYPERX 0x004c0844
    m_sReticle.LoadByName("Reticl2");

    // STRING: HYPERX 0x004c2fd8
    m_oShotPower.LoadByName("nhshotp");

    const Point ptShotSize = m_oShotPower.m_ptSize;

    m_oShotPower.SetStrip(ptShotSize.x, 1, 0, 0);

    String strPooled("");

    char* pszPooled = g_gameApp.m_stringPool.LoadText(STRING_EXIT_MATCH);

    strPooled.Assign(String::SafeStrlen(pszPooled), pszPooled);

    pLabel = &m_tExitMatchPrompt;

    pLabel->CreateFontFace(g_abInfoBarFontName, FONT_BODY);
    pLabel->SetText(strPooled, (COLORREF)COLOUR_YELLOW);

    pszPooled = g_gameApp.m_stringPool.LoadText(STRING_CONTINUE);

    strPooled.Assign(String::SafeStrlen(pszPooled), pszPooled);

    pLabel = &m_tContinuePrompt;

    pLabel->CreateFontFace(g_abInfoBarFontName, FONT_BODY);
    pLabel->SetText(strPooled, (COLORREF)COLOUR_YELLOW);

    m_atTeamNames[0].CreateFontFace(g_abInfoBarFontName, FONT_PAIR);
    m_atTeamNames[1].CreateFontFace(g_abInfoBarFontName, FONT_PAIR);

    m_atTeamNames[0].SetText("X", (COLORREF)COLOUR_UNSET);
    m_atTeamNames[1].SetText("X", (COLORREF)COLOUR_UNSET);

    for (i = 0; i < 4; i++)
    {
        m_atOwnRoster[i].CreateFontFace(g_abInfoBarFontName, FONT_TIMER);
        m_atOwnRoster[i].SetText("X", (COLORREF)COLOUR_UNSET);
    }

    for (i = 0; i < 4; i++)
    {
        m_atFoeRoster[i].CreateFontFace(g_abInfoBarFontName, FONT_TIMER);
        m_atFoeRoster[i].SetText("X", (COLORREF)COLOUR_UNSET);
    }

    InfoBar::LayoutSurfaces();

    ForgetPalette();
}

// FUNCTION: HYPERX 0x00451ce0
InfoBar::~InfoBar()
{
    Cleanup();
}

// The original's linker shares this empty body with Stub::Assert; this build cannot fold it.
void InfoBar::Cleanup() throw()
{
}

// FUNCTION: HYPERX 0x00451ed0
void InfoBar::SetMessageText(const char* pszText, unsigned long color)
{
    m_tMessage.SetText(pszText, color);
}

// FUNCTION: HYPERX 0x00451ef0
void InfoBar::ShowMessage()
{
    m_bMessageVisible = 1;
}

// FUNCTION: HYPERX 0x00451f00
inline void InfoBar::HideMessage()
{
    m_bMessageVisible = 0;
}

// Slot eight.

// FUNCTION: HYPERX 0x00451f10
void InfoBar::SetHomeLogo(char* pszName)
{
    m_sTeamALogo.LoadByName(pszName);
}

// Slot nine.

// FUNCTION: HYPERX 0x00451f30
void InfoBar::SetAwayLogo(char* pszName)
{
    m_sTeamBLogo.LoadByName(pszName);
}

// FUNCTION: HYPERX 0x00451f50
void InfoBar::ShowHeadCam()
{
    m_bHeadCamPending = 1;
}

// FUNCTION: HYPERX 0x00451f60
void InfoBar::SetHeadCamText(const char* pszText)
{
    m_tHeadCamText.SetText(pszText, (COLORREF)-1);
}

// Both answer constants; the unread stack point is the original's.

// FUNCTION: HYPERX 0x00451f80
ViewExtent InfoBar::GetHeadCamSize()
{
    SurfacePoint pt;
    ViewExtent   size;

    size.x = 0x7c;
    size.y = 0x54;

    return size;
}

// The corner is twenty five pixels past each axis sum.
// FUNCTION: HYPERX 0x00451fb0
ViewExtent InfoBar::GetHeadCamOrigin()
{
    const Point ptOrigin = m_sHeadCamFrame.m_ptPosition;
    const Point ptOffset = m_sHeadCamFrame.m_ptDrawOffset;

    ViewExtent offset;

    offset.x = ptOffset.x + ptOrigin.x + METRIC_ROW_GAP;
    offset.y = ptOffset.y + ptOrigin.y + METRIC_ROW_GAP;

    return offset;
}

// The top clamp is count less one, so exactly a hundred lands on the last entry.
// FUNCTION: HYPERX 0x00451fe0
void InfoBar::SetShotPower(float flPercent)
{
    const int nCount = m_oShotPower.m_nFrameCount;

    const float flPosition = ((float)nCount * flPercent) * 0.01f;

    m_oShotPower.m_flScalar = flPosition;

    int nIndex = (int)flPosition;

    if (nIndex >= nCount)
    {
        nIndex = nCount - 1;
    }

    if (nIndex < 0)
    {
        nIndex = 0;
    }

    m_oShotPower.m_nFrame = nIndex;
}

// FUNCTION: HYPERX 0x00452030
void InfoBar::ShowShotPower()
{
    m_bShotPowerWanted = 1;
}

// FUNCTION: HYPERX 0x00452040
void InfoBar::SetReticlePos(int nX, int nY)
{
    m_bReticleVisible           = 1;
    m_sReticle.m_ptDrawOffset.x = nX;
    m_sReticle.m_ptDrawOffset.y = nY;
}

// FUNCTION: HYPERX 0x00452070
ViewExtent InfoBar::GetReticleSize()
{
    ViewExtent extent;

    extent = m_sReticle.m_ptSize;

    return extent;
}

// A mode outside the four stores nothing.
// FUNCTION: HYPERX 0x00452090
inline void InfoBar::SetReticleState(int nMode)
{
    switch (nMode)
    {
        case 0:
            m_bReticleVisible = 0;
            break;

        case 1:
            m_bReticleVisible = 1;
            break;

        case 2:
            m_bReticleVisible = 1;
            break;

        case 3:
            m_bReticleVisible = 1;
            break;

        default:
            break;
    }
}

// FUNCTION: HYPERX 0x004520d0
ViewExtent InfoBar::GetPassBarSize()
{
    SurfacePoint pt;
    ViewExtent   size;

    size.x = 0x64;
    size.y = 0x14;

    return size;
}

// FUNCTION: HYPERX 0x00452100
void InfoBar::ShowQuitPrompt()
{
    m_bQuitPromptVisible = 1;

    m_nQuitPromptTick = g_pGameEngine->m_clock.nTickMs;
}

// FUNCTION: HYPERX 0x00452120
void InfoBar::HideQuitPrompt()
{
    m_bQuitPromptVisible = 0;
}

// Only five (team one) and four (team two) do anything.

// FUNCTION: HYPERX 0x0047ae70
void InfoBar::IncrementScore(int nWhich)
{
    switch (nWhich)
    {
        case SCORE_LINE_TEAM1:
            // STRING: HYPERX 0x004bb814
            g_nHomeScoreLength = sprintf(g_szHomeScore, "%d", g_pGameEngine->IncrementHomeScore());

            g_bHomeScoreDirty = 1;

            g_wActiveScoreLine = 0;
            break;

        case SCORE_LINE_TEAM2:
            g_nAwayScoreLength = sprintf(g_szAwayScore, "%d", g_pGameEngine->IncrementAwayScore());

            g_bAwayScoreDirty = 1;

            g_wActiveScoreLine = 1;
            break;
    }
}

int g_anPeriodLightOffset[3] = { 19, 38, 57 };

// Minus one, which no display word or damage can be, forces the first rebuild.
int g_nInfoBarOwnHomeTeam      = -1;
int g_nInfoBarOwnAwayTeam      = -1;
int g_nInfoBarOwnViewedSlot    = -1;
int g_anInfoBarOwnFormation[2] = { -1, -1 };
int g_anInfoBarOwnDamage[4]    = { -1, -1, -1, -1 };
int g_anOwnFormationSlotMap[4] = { 0, 1, 3, 4 };
int g_nInfoBarFoeHomeTeam      = -1;
int g_nInfoBarFoeAwayTeam      = -1;
int g_anInfoBarFoeFormation[2] = { -1, -1 };
int g_anInfoBarFoeDamage[4]    = { -1, -1, -1, -1 };
int g_anFoeFormationSlotMap[4] = { 0, 1, 3, 4 };
int g_nInfoBarLastHomeScore    = -1;
int g_nInfoBarLastAwayScore    = -1;

void InfoBar::BeginTick()
{
    m_bHeadCamPending  = 0;
    m_bShotPowerWanted = 0;
    m_bReticleVisible  = 0;

    g_pMessageQueue->Update();
}
