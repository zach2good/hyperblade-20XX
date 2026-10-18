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
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "graphics/viewextent.h"
#include "graphics/infobarslot.h"
#include "ui/cddsurfaceowner.h"
#include "math/point.h"
#include "ui/infobarbase.h"
#include "physics/collisionellipsoid.h"
#include "graphics/swrend.h"
#include "engine/statids.h"
#include "app/stringids.h"

constexpr auto kPeriodLampCount = 3;

// Where each of the three lamps sits, as an offset from the bar's own left edge.
// GLOBAL: HYPERX 0x004c2f60
extern int g_anPeriodLightOffset[3];

constexpr auto kPeriodLampTop = 0x28;

constexpr auto kPeriodLampSide = 9;

// GLOBAL: HYPERX 0x004cef64
extern int g_nPeriodLightColour;

constexpr auto kMetricRowGap = 0x19;

// The count prints unsigned; its sign only picks the vertical offset. A zero count stops drawing.

constexpr auto kRokLabelPositive = 0x20;
constexpr auto kRokLabelNegative = 0x2f;

constexpr auto kRokTextBuffer = 0x50;

// The info bar's own cache of the last rok value drawn.
// GLOBAL: HYPERX 0x004cef54
extern int g_nInfoBarLastRokValue;

// FUNCTION: HYPERX 0x0044fc70
void InfoBar::DrawPeriodLight()
{
    m_sPeriodFrame.Draw();

    auto       nX = m_sPeriodFrame.m_ptPosition.x;
    const auto nY = m_sPeriodFrame.m_ptPosition.y;

    auto nLamp = g_pGameEngine->m_clock.nPeriod - 1;

    if (nLamp < 0)
    {
        nLamp = 0;
    }
    else if (nLamp >= kPeriodLampCount)
    {
        nLamp = kPeriodLampCount - 1;
    }

    nX += g_anPeriodLightOffset[nLamp];

    nLamp = g_nPeriodLightColour;

    SWRend::FillRect(nX, nY + kPeriodLampTop, kPeriodLampSide, kPeriodLampSide, nLamp);
}

// FUNCTION: HYPERX 0x00450160
void InfoBar::UpdateRokValue()
{
    char szText[kRokTextBuffer];

    auto* const pRok = g_pGameEngine->GetRok();

    if (pRok != nullptr)
    {
        const int nRokValue = pRok->m_wRokValue;

        if (nRokValue != 0)
        {
            if (nRokValue != g_nInfoBarLastRokValue)
            {
                g_nInfoBarLastRokValue = nRokValue;

                // STRING: HYPERX 0x004c0d6c
                sprintf(szText, "x%d", abs(nRokValue));

                auto* const pLabel = &m_tRokValue;

                pLabel->m_ptDrawOffset.x = 0;

                if (nRokValue > 0)
                {
                    pLabel->m_ptDrawOffset.y = kRokLabelPositive;
                }
                else
                {
                    pLabel->m_ptDrawOffset.y = kRokLabelNegative;
                }

                pLabel->SetText(szText, static_cast<std::uint32_t>(0xffff));
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
    auto* const pObject = g_pGameEngine->GetPlayer(static_cast<short>(nSlot));

    if (pObject != nullptr)
    {
        if (g_pGameEngine->m_nViewedSlot == nSlot)
        {
            pBar->m_nColour = g_nDamageBarColourLit;
        }
        else
        {
            pBar->m_nColour = g_nDamageBarColourDim;
        }

        const double dScaled = pObject->m_stats.GetScalar(kRatingDamage) * 0.01f;

        const auto nFrameCount = pBar->m_nFrameCount;

        const auto dFraction = dScaled * nFrameCount;

        pBar->m_flScalar = static_cast<float>(dFraction);

        auto nFrame = static_cast<int>(dFraction);

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
    char             szText[kTeamLabelBuffer];
    const TeamBlock* pTeam;
    const TeamBlock* pOther;
    int              aFormation[2];
    int              aDamage[kTeamLabelCount];
    int              nColour;
    int              i;

    short nSlot = 0;

    if (g_pGameEngine == nullptr)
    {
        return;
    }

    auto      bDirty      = 0;
    const int nViewedSlot = g_pGameEngine->m_nViewedSlot;

    if ((nViewedSlot & kEngineSideSwapped) != 0)
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

    auto* pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[0] = static_cast<int>(pObject->m_stats.GetScalar(kRatingDamage));

    nSlot += 2;

    pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[1] = static_cast<int>(pObject->m_stats.GetScalar(kRatingDamage));

    auto* const pReserves = g_pGameEngine->GetReserveRows(pTeam);

    aDamage[2] = static_cast<int>(pTeam->aRows[pReserves->anRow[0]].aStat[kRatingDamage]);
    aDamage[3] = static_cast<int>(pTeam->aRows[pReserves->anRow[1]].aStat[kRatingDamage]);

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
        const int nFollowed = (static_cast<unsigned int>(nViewedSlot) >> 1) & 1;

        for (i = 0; i < kTeamLabelCount; i++)
        {
            if (static_cast<float>(aDamage[i]) <= DamageGate())
            {
                nColour = kTeamColourAbsent;
            }
            else if (nFollowed == i)
            {
                nColour = kTeamColourFollowed;
            }
            else if (i < kTeamLabelOnDrome)
            {
                nColour = kTeamColourOnDrome;
            }
            else
            {
                nColour = kTeamColourReserve;
            }

            if (i >= kTeamLabelOnDrome)
            {
                sprintf(szText, "%c. %s", i - kTeamLabelOnDrome + '1', pTeam->GetRosterName(g_anOwnFormationSlotMap[pReserves->anRow[i - kTeamLabelOnDrome]]));
            }
            else
            {
                strcpy(szText,
                       pTeam->GetRosterName(g_anOwnFormationSlotMap[aFormation[i]]));
            }

            m_atOwnRoster[i].SetText(szText, static_cast<std::uint32_t>(nColour));
        }

        m_atTeamNames[0].SetText(pTeam->GetName(), static_cast<std::uint32_t>(kTeamColourReserve));
        m_atTeamNames[1].SetText(pOther->GetName(), static_cast<std::uint32_t>(kTeamColourReserve));
    }

    for (i = 0; i < kTeamLabelCount; i++)
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
    char             szText[kTeamLabelBuffer];
    const TeamBlock* pTeam;
    int              aFormation[2];
    int              aDamage[kTeamLabelCount];
    int              nColour;
    int              i;

    short nSlot = 0;

    if (g_pGameEngine == nullptr)
    {
        return;
    }

    auto bDirty = 0;

    if ((g_pGameEngine->m_nViewedSlot & kEngineSideSwapped) != 0)
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

    auto* pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[0] = static_cast<int>(pObject->m_stats.GetScalar(kRatingDamage));

    nSlot += 2;

    pObject = g_pGameEngine->GetPlayer(nSlot);

    aDamage[1] = static_cast<int>(pObject->m_stats.GetScalar(kRatingDamage));

    auto* const pReserves = g_pGameEngine->GetReserveRows(pTeam);

    aDamage[2] = static_cast<int>(pTeam->aRows[pReserves->anRow[0]].aStat[kRatingDamage]);
    aDamage[3] = static_cast<int>(pTeam->aRows[pReserves->anRow[1]].aStat[kRatingDamage]);

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
        for (i = 0; i < kTeamLabelCount; i++)
        {
            if (static_cast<float>(aDamage[i]) <= DamageGate())
            {
                nColour = kTeamColourAbsent;
            }
            else if (i < kTeamLabelOnDrome)
            {
                nColour = kTeamColourOnDrome;
            }
            else
            {
                nColour = kTeamColourReserve;
            }

            if (i >= kTeamLabelOnDrome)
            {
                strcpy(szText,
                       pTeam->GetRosterName(g_anFoeFormationSlotMap[pReserves->anRow[i - kTeamLabelOnDrome]]));
            }
            else
            {
                strcpy(szText,
                       pTeam->GetRosterName(g_anFoeFormationSlotMap[aFormation[i]]));
            }

            m_atFoeRoster[i].SetText(szText, static_cast<std::uint32_t>(nColour));
        }
    }

    for (i = 0; i < kTeamLabelCount; i++)
    {
        if (aDamage[i] > 0)
        {
            m_atFoeRoster[i].Draw();
        }
    }
}

constexpr auto kTextColour = 0xffff;

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
    char szText[kClockBuffer];

    const int nElapsed = g_pGameEngine->m_nPeriodLengthMs - g_pGameEngine->m_clock.nPauseStartMs + g_pGameEngine->m_clock.nPeriodStartMs;

    const auto nSeconds = static_cast<int>((nElapsed % kMsPerMinute) * 0.001f);
    const auto nMinutes = static_cast<int>(nElapsed * 1.6666667e-005f);

    const int nActive = g_frameTimer.bFrameTimesOn;

    if (g_nInfoBarLastSeconds != nSeconds || nSeconds == 0)
    {
        g_nInfoBarLastSeconds = nSeconds;

        // STRING: HYPERX 0x004c0d70
        sprintf(szText, "%d:%02d", nMinutes, nSeconds);

        m_tClock.SetText(szText, static_cast<std::uint32_t>(kTextColour));

        if (nActive != 0)
        {
            m_tFrameRate.SetText(g_szFpsText, static_cast<std::uint32_t>(kTextColour));
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

    auto nScore = g_pGameEngine->GetHomeScore();

    if (nScore != g_nInfoBarLastHomeScore)
    {
        // STRING: HYPERX 0x004bb814
        sprintf(szText, "%d", nScore);

        m_tHomeScore.SetText(szText, static_cast<std::uint32_t>(kTextColour));

        g_nInfoBarLastHomeScore = nScore;
    }

    m_tHomeScore.Draw();

    nScore = g_pGameEngine->GetAwayScore();

    if (nScore != g_nInfoBarLastAwayScore)
    {
        // STRING: HYPERX 0x004bb814
        sprintf(szText, "%d", nScore);

        m_tAwayScore.SetText(szText, static_cast<std::uint32_t>(kTextColour));

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
        if (g_pGameEngine->m_clock.nTickMs - m_nQuitPromptTick >= kQuitPromptInterval)
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

constexpr auto kEngineFollowLimit = 6;

constexpr auto kColourBlipDromer0 = 0xc8c800;
constexpr auto kColourBlipDromer2 = 0xffff00;
constexpr auto kColourBlipDromer1 = 0xc8;
constexpr auto kColourBlipDromer3 = 0xff;
constexpr auto kColourBlipRok     = 0xffff;
constexpr auto kColourPeriodLight = 0xff;
constexpr auto kColourBarLit      = 0xffff;
constexpr auto kColourBarDim      = 0xc8;
constexpr auto kColourRadarLine   = 0xe0ffff;

// A null engine triggers the re-resolve, which then dereferences it, as in the original.
// FUNCTION: HYPERX 0x00450b80
void InfoBar::DrawScene(int bDraw)
{
    if (g_pBackBufferBitsCopy != nullptr)
    {
        if (g_pGameEngine == nullptr || m_nCachedHomeTeam != g_pGameEngine->m_nTeamHomeIndex || m_nCachedAwayTeam != g_pGameEngine->m_nTeamAwayIndex || m_nCachedArena != g_pGameEngine->m_nArenaIndex)
        {
            m_nCachedHomeTeam = g_pGameEngine->m_nTeamHomeIndex;
            m_nCachedAwayTeam = g_pGameEngine->m_nTeamAwayIndex;
            m_nCachedArena    = g_pGameEngine->m_nArenaIndex;

            g_nRadarBlipColourDromer0 = g_pColorPalette->NearestIndex(kColourBlipDromer0, HudLayer::kKeyIndex);
            g_nRadarBlipColourDromer2 = g_pColorPalette->NearestIndex(kColourBlipDromer2, HudLayer::kKeyIndex);
            g_nRadarBlipColourDromer1 = g_pColorPalette->NearestIndex(kColourBlipDromer1, HudLayer::kKeyIndex);
            g_nRadarBlipColourDromer3 = g_pColorPalette->NearestIndex(kColourBlipDromer3, HudLayer::kKeyIndex);
            g_nRadarBlipColourRok     = g_pColorPalette->NearestIndex(kColourBlipRok, HudLayer::kKeyIndex);
            g_nPeriodLightColour      = g_pColorPalette->NearestIndex(kColourPeriodLight, HudLayer::kKeyIndex);
            g_nDamageBarColourLit     = g_pColorPalette->NearestIndex(kColourBarLit, HudLayer::kKeyIndex);
            g_nDamageBarColourDim     = g_pColorPalette->NearestIndex(kColourBarDim, HudLayer::kKeyIndex);
            g_nRadarLineColour        = g_pColorPalette->NearestIndex(kColourRadarLine, HudLayer::kKeyIndex);
        }

        DrawPeriodLight();

        if (bDraw != 0)
        {
            const int nFollowed = g_pGameEngine->m_nViewedSlot;

            if (nFollowed < kEngineFollowLimit)
            {
                auto* const pObject = g_pGameEngine->GetPlayer(static_cast<short>(nFollowed));

                if (pObject != nullptr)
                {
                    auto* const pPanel = &m_oSpeedMeter;

                    const auto nFrameCount = pPanel->m_nFrameCount;

                    const double dFraction = pObject->m_stats.GetSpeedRatio() * nFrameCount;

                    pPanel->m_flScalar = static_cast<float>(dFraction);

                    auto nFrame = static_cast<int>(dFraction);

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

    // Centred on a sample time: the clock's digits share a width, and periods stay under ten minutes.
    ptPos.x += (ptSize.x - Text::Measure(m_tClock.m_font, "0:00").nWidth) / 2;
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

    auto nY = ptRoster.y + ptSize.y + 2;

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

constexpr auto kInfobarNameBuffer = 0x50;

constexpr auto kFontTimer = 9;
constexpr auto kFontLabel = 10;
constexpr auto kFontPair  = 11;
constexpr auto kFontBody  = 15;
constexpr auto kFontLarge = 20;

constexpr auto kColourYellow = 0xffff;
constexpr auto kColourUnset  = (-1);

// The damage bar strip: twelve frames, the ninth showing, drawn two by eight.
constexpr auto kBarFrameCount = 0x0c;
constexpr auto kBarUnitWidth  = 9;
constexpr auto kBarMinHeight  = 2;
constexpr auto kBarMaxHeight  = 8;

// FUNCTION: HYPERX 0x00451280
InfoBar::InfoBar()
{
    char szName[kInfobarNameBuffer];
    Vec3 vLimits;
    int  i;

    strcpy(g_abInfoBarFontName, "MS Sans Serif");

    m_sPeriodFrame.LoadByName("frper");

    m_tClock.CreateFontFace(g_abInfoBarFontName, kFontLarge);
    m_tFrameRate.CreateFontFace(g_abInfoBarFontName, kFontBody);

    sprintf(szName, "fr%s", "speed");

    m_sSpeedFrame.LoadByName(szName);

    // STRING: HYPERX 0x004c2fe0
    sprintf(szName, "%s01", "speed");

    // This class's LoadByName at 0x0044f8f0; RLESpriteSurface's writes other fields.
    m_oSpeedMeter.LoadByName(szName);

    const Point ptSpeedSize = m_oSpeedMeter.m_ptSize;

    m_oSpeedMeter.SetStrip(ptSpeedSize.x, 1, 0, 0);

    m_tHomeScore.CreateFontFace(g_abInfoBarFontName, kFontBody);

    m_sTeamAFrame.LoadByName("frdamgA");

    m_kTeamADamage1.SetStrip(kBarFrameCount, kBarUnitWidth, kBarMinHeight, kBarMaxHeight);
    m_kTeamADamage2.SetStrip(kBarFrameCount, kBarUnitWidth, kBarMinHeight, kBarMaxHeight);

    m_tAwayScore.CreateFontFace(g_abInfoBarFontName, kFontBody);

    m_sTeamBFrame.LoadByName("frdamgB");

    m_kTeamBDamage1.SetStrip(kBarFrameCount, kBarUnitWidth, kBarMinHeight, kBarMaxHeight);
    m_kTeamBDamage2.SetStrip(kBarFrameCount, kBarUnitWidth, kBarMinHeight, kBarMaxHeight);

    m_tRokValue.CreateFontFace(g_abInfoBarFontName, kFontTimer);

    HideMessage();

    m_sMessageFrame.LoadByName("frmsg");

    m_tMessage.CreateFontFace(g_abInfoBarFontName, kFontBody);

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

    m_tHeadCamText.CreateFontFace(g_abInfoBarFontName, kFontLabel);

    m_tHeadCamText.SetColour(kColourYellow);

    auto* pLabel = &m_tShotCameraLabel;

    pLabel->CreateFontFace(g_abInfoBarFontName, kFontLabel);

    pLabel->SetText("Shot Camera", static_cast<std::uint32_t>(kColourYellow));

    m_bShotPowerWanted = 0;
    SetReticleState(0);

    // STRING: HYPERX 0x004c0844
    m_sReticle.LoadByName("Reticl2");

    // STRING: HYPERX 0x004c2fd8
    m_oShotPower.LoadByName("nhshotp");

    const Point ptShotSize = m_oShotPower.m_ptSize;

    m_oShotPower.SetStrip(ptShotSize.x, 1, 0, 0);

    String strPooled("");

    auto* pszPooled = g_gameApp.m_stringPool.LoadText(kStringExitMatch);

    strPooled.Assign(String::SafeStrlen(pszPooled), pszPooled);

    pLabel = &m_tExitMatchPrompt;

    pLabel->CreateFontFace(g_abInfoBarFontName, kFontBody);
    pLabel->SetText(strPooled, static_cast<std::uint32_t>(kColourYellow));

    pszPooled = g_gameApp.m_stringPool.LoadText(kStringContinue);

    strPooled.Assign(String::SafeStrlen(pszPooled), pszPooled);

    pLabel = &m_tContinuePrompt;

    pLabel->CreateFontFace(g_abInfoBarFontName, kFontBody);
    pLabel->SetText(strPooled, static_cast<std::uint32_t>(kColourYellow));

    m_atTeamNames[0].CreateFontFace(g_abInfoBarFontName, kFontPair);
    m_atTeamNames[1].CreateFontFace(g_abInfoBarFontName, kFontPair);

    m_atTeamNames[0].SetText("X", static_cast<std::uint32_t>(kColourUnset));
    m_atTeamNames[1].SetText("X", static_cast<std::uint32_t>(kColourUnset));

    for (i = 0; i < 4; i++)
    {
        m_atOwnRoster[i].CreateFontFace(g_abInfoBarFontName, kFontTimer);
        m_atOwnRoster[i].SetText("X", static_cast<std::uint32_t>(kColourUnset));
    }

    for (i = 0; i < 4; i++)
    {
        m_atFoeRoster[i].CreateFontFace(g_abInfoBarFontName, kFontTimer);
        m_atFoeRoster[i].SetText("X", static_cast<std::uint32_t>(kColourUnset));
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
void InfoBar::Cleanup() noexcept
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
    m_tHeadCamText.SetText(pszText, static_cast<std::uint32_t>(-1));
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

    offset.x = ptOffset.x + ptOrigin.x + kMetricRowGap;
    offset.y = ptOffset.y + ptOrigin.y + kMetricRowGap;

    return offset;
}

// The top clamp is count less one, so exactly a hundred lands on the last entry.
// FUNCTION: HYPERX 0x00451fe0
void InfoBar::SetShotPower(float flPercent)
{
    const auto nCount = m_oShotPower.m_nFrameCount;

    const auto flPosition = (static_cast<float>(nCount) * flPercent) * 0.01f;

    m_oShotPower.m_flScalar = flPosition;

    auto nIndex = static_cast<int>(flPosition);

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
        case kScoreLineTeam1:
            // STRING: HYPERX 0x004bb814
            g_nHomeScoreLength = sprintf(g_szHomeScore, "%d", g_pGameEngine->IncrementHomeScore());

            g_bHomeScoreDirty = 1;

            g_wActiveScoreLine = 0;
            break;

        case kScoreLineTeam2:
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
