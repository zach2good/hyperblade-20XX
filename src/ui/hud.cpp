#include "common/compat.h"
#include "engine/messagequeue.h"
#include "globals.h"
#include "graphics/graphicsglobals.h"
#include "app/frametimer.h"
#include "uiglobals.h"
#include "hud.h"
#include "engine/objectclasses.h"
#include "engine/rokobject.h"
#include "engine/statids.h"
#include "graphics/damagebar.h"
#include "engine/objectstats.h"
#include "engine/gameengine.h"
#include "hudglobals.h"
#include "common/pathstring.h"
#include "graphics/texture.h"
#include "radar.h"
#include "infobar.h"
#include "graphics/scrollbitmap.h"
#include "graphics/cddbitmap.h"
#include "graphics/renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include "graphics/cddtextsurface.h"
#include "graphics/framedbarbitmap.h"
#include "engine/playerobject.h"
#include "graphics/steppedscrollbitmap.h"
#include "graphics/passbar.h"
#include "graphics/viewextent.h"
#include "math/point.h"
#include "app/gameapp.h"
#include "common/stringpool.h"
#include "hudsurfaces.h"
#include "planvm/damagegate.h"
#include "physics/collisionellipsoid.h"
#include "graphics/ddrawapi.h"
#include "common/stub.h"

SurfacePoint g_ptHudInset(2, 2);
SurfacePoint g_ptHudMargin(0x14, 0x14);

#define SCORE_NAME_BUFFER 0x50

// GLOBAL: HYPERX 0x004c0cf4
extern int g_nOwnRowStampB;

// Every one ships at minus one, so the first frame after a load rebuilds.
// GLOBAL: HYPERX 0x004c0cf0
extern int g_nOwnRowStampA;

// GLOBAL: HYPERX 0x004c0cf8
extern int g_nOwnRowSlot;

// GLOBAL: HYPERX 0x004c0cfc
extern int g_nOwnRowFormationRow;

// GLOBAL: HYPERX 0x004c0d00
extern int g_nOwnRowFormationColumn;

// GLOBAL: HYPERX 0x004c0d04
extern int g_nOwnRowDamage0;

// GLOBAL: HYPERX 0x004c0d08
extern int g_nOwnRowDamage1;

// GLOBAL: HYPERX 0x004c0d0c
extern int g_nOwnRowRating0;

// GLOBAL: HYPERX 0x004c0d10
extern int g_nOwnRowRating1;

#define COLOUR_BLACK 0

#define COLOUR_FOLLOWED 0xffff

#define COLOUR_ACTIVE 0x100ff

#define COLOUR_BENCH 0xff00

// The first two rows are on the drome; the rest are the bench.
inline int HUD::RowColour(int nRow)
{
    if (nRow < 2)
    {
        return COLOUR_ACTIVE;
    }

    return COLOUR_BENCH;
}

// The third is skipped.
// GLOBAL: HYPERX 0x004c0d14
extern int g_anOwnRowNameOrder[4];

inline void HUD::SetLabelText(CDDTextSurface* pLabel, const char* pszText)
{
    String* const pString = &pLabel->m_strText;

    pString->Assign(String::SafeStrlen(pszText), pszText);

    pLabel->RestoreContents();
}

// GLOBAL: HYPERX 0x004c0d24
extern int g_nFoeRowStampA;

// GLOBAL: HYPERX 0x004c0d28
extern int g_nFoeRowStampB;

// GLOBAL: HYPERX 0x004c0d2c
extern int g_nFoeRowSlot;

// GLOBAL: HYPERX 0x004c0d30
extern int g_nFoeRowFormationRow;

// GLOBAL: HYPERX 0x004c0d34
extern int g_nFoeRowFormationColumn;

// GLOBAL: HYPERX 0x004c0d38
extern int g_nFoeRowDamage0;

// GLOBAL: HYPERX 0x004c0d3c
extern int g_nFoeRowDamage1;

// GLOBAL: HYPERX 0x004c0d40
extern int g_nFoeRowRating0;

// GLOBAL: HYPERX 0x004c0d44
extern int g_nFoeRowRating1;

// Two active players, then two on the bench.
#define FOE_ROWS        4
#define FOE_ACTIVE_ROWS 2

// GLOBAL: HYPERX 0x004c0d48
extern int g_anFoeRowNameOrder[4];

// symbols/hud_surfaces.csv maps these surface names to the database's.
// FUNCTION: HYPERX 0x0043d8d0
void __stdcall HUD::PlaceSurfaces(void)
{
    Point ptFrameSize;
    int   nWidget;
    Point ptPos;
    Point ptColumn;
    Point ptAnchor;

    if (g_ddrawDevice.bD3DActive != 0)
    {
        ptPos.Set(0, 0);
        ptAnchor.Set(0, 0);
        ptFrameSize.Set(0, 0);
        g_nHudRightMargin = g_nPrevWidth - 10;

        if (g_hudSurfaces.pTeamAFrame != 0)
        {
            ptFrameSize = g_hudSurfaces.pTeamAFrame->GetSize();
        }

        ptPos.x  = 0;
        ptPos.y  = (g_nPrevHeight + ptFrameSize.y * -2) - 5;
        ptAnchor = ptPos;

        if (g_hudSurfaces.pTeamAFrame != 0)
        {
            g_hudSurfaces.pTeamAFrame->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pTeamABitmap != 0)
        {
            g_hudSurfaces.pTeamABitmap->DispatchRect(-1, 0x16, 0, 0);
        }

        if (g_hudSurfaces.pTeamAScoreText != 0)
        {
            g_hudSurfaces.pTeamAScoreText->DispatchRect(-6, 5, -8, 0);
        }

        if (g_hudSurfaces.pMessageFrame != 0)
        {
            g_hudSurfaces.pMessageFrame->DispatchRect(0, 0x20, 0xe2, 0);
        }

        if (g_hudSurfaces.pMessageText != 0)
        {
            g_hudSurfaces.pMessageText->DispatchRect(8, 6, 0, 0);
        }

        if (g_hudSurfaces.pTeamBFrame != 0)
        {
            ptFrameSize = g_hudSurfaces.pTeamBFrame->GetSize();
        }

        ptPos.x  = ptAnchor.x;
        ptPos.y  = (ptAnchor.y - 2) + ptFrameSize.y;
        ptAnchor = ptPos;

        if (g_hudSurfaces.pTeamBFrame != 0)
        {
            g_hudSurfaces.pTeamBFrame->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pTeamBBitmap != 0)
        {
            g_hudSurfaces.pTeamBBitmap->DispatchRect(-1, 0, 0, 0);
        }

        if (g_hudSurfaces.pTeamBScoreText != 0)
        {
            g_hudSurfaces.pTeamBScoreText->DispatchRect(-6, 0x14, -8, 0);
        }

        if (g_hudSurfaces.pTeamAFrame != 0)
        {
            ptFrameSize = g_hudSurfaces.pTeamAFrame->GetSize();
            ptAnchor    = g_hudSurfaces.pTeamAFrame->GetPosition();
        }

        ptPos.x = ptAnchor.x + 0x4b;

        if (g_hudSurfaces.pTeamADamageBar1 != 0)
        {
            ptFrameSize = g_hudSurfaces.pTeamADamageBar1->GetSize();
        }

        ptPos.y = ptAnchor.y + 0xc + ptFrameSize.y;

        if (g_hudSurfaces.pTeamADamageBar1 != 0)
        {
            g_hudSurfaces.pTeamADamageBar1->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pTeamADamageBar2 != 0)
        {
            ptFrameSize = g_hudSurfaces.pTeamADamageBar2->GetSize();
        }

        ptPos.y = ptPos.y - 1 + ptFrameSize.y;

        if (g_hudSurfaces.pTeamADamageBar2 != 0)
        {
            g_hudSurfaces.pTeamADamageBar2->SetPosition(ptPos);
        }

        ptPos.y = ptPos.y + 6 + ptFrameSize.y;

        if (g_hudSurfaces.pTeamBDamageBar1 != 0)
        {
            g_hudSurfaces.pTeamBDamageBar1->SetPosition(ptPos);
        }

        ptPos.y = ptPos.y - 1 + ptFrameSize.y;

        if (g_hudSurfaces.pTeamBDamageBar2 != 0)
        {
            g_hudSurfaces.pTeamBDamageBar2->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pRadar != 0)
        {
            ptFrameSize = g_hudSurfaces.pRadar->GetSize();
            ptPos.x     = ((g_nPrevWidth - g_ptHudInset.y) - ptFrameSize.x) - 0x12;
            ptPos.y     = ((g_nPrevHeight - g_ptHudInset.y) - ptFrameSize.y) - 0x12;
            g_hudSurfaces.pRadar->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pClockText != 0)
        {
            ptFrameSize = g_hudSurfaces.pClockText->GetSize();
        }

        g_ptHudClockAnchor.x = (g_nPrevWidth >> 1) - (ptFrameSize.x >> 1);
        g_ptHudClockAnchor.y = g_ptHudInset.y + 5;

        if (g_hudSurfaces.pClockText != 0)
        {
            g_hudSurfaces.pClockText->SetPosition(g_ptHudClockAnchor);
        }

        if (g_hudSurfaces.pPeriodFrame != 0)
        {
            ptFrameSize = g_hudSurfaces.pPeriodFrame->GetSize();
            ptPos.y     = 1;
            ptPos.x     = (g_nPrevWidth >> 1) - (ptFrameSize.x >> 1);
            g_hudSurfaces.pPeriodFrame->SetPosition(ptPos);

            if (g_hudSurfaces.pPeriodOverlay != 0)
            {
                g_hudSurfaces.pPeriodOverlay->DispatchRect(0x13, 0x28, 0, 0);
            }
        }

        ptPos.x = 0;
        ptPos.y = 0;

        if (g_hudSurfaces.pSpeedMeter != 0)
        {
            g_hudSurfaces.pSpeedMeter->SetPosition(ptPos);
            ptFrameSize = g_hudSurfaces.pSpeedMeter->GetSize();
        }

        ptColumn.Zero();
        ptColumn.y         = ptFrameSize.y + ptPos.y;
        ptColumn.x         = ptPos.x + 6;
        ptPos              = g_ptHudMargin;
        g_ptHudFpsAnchor.x = ptPos.x;
        g_ptHudFpsAnchor.y = ptFrameSize.y + ptPos.y * 2;

        if (g_ddrawDevice.pFpsText != 0)
        {
            g_ddrawDevice.pFpsText->SetPosition(g_ptHudFpsAnchor);
        }

        if (g_hudSurfaces.pNetworkIcon != 0)
        {
            ptFrameSize = g_hudSurfaces.pNetworkIcon->GetSize();
            ptPos.x     = (g_nPrevWidth - g_ptHudMargin.x) - ptFrameSize.x;
            ptPos.y     = g_ptHudMargin.y;
            g_hudSurfaces.pNetworkIcon->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pHeadCamFrame != 0)
        {
            ptFrameSize = g_hudSurfaces.pHeadCamFrame->GetSize();
            ptPos.x     = g_nPrevWidth - ptFrameSize.x;
            ptPos.y     = 0;
            g_hudSurfaces.pHeadCamFrame->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pReticle != 0)
        {
            Point ptCamSize = g_hudSurfaces.pHeadCamFrame->GetSize();
            ptAnchor        = g_hudSurfaces.pHeadCamFrame->GetPosition();
            ptFrameSize     = g_hudSurfaces.pReticle->GetSize();
            ptPos.x         = ((ptCamSize.x >> 1) - (ptFrameSize.x >> 1)) + ptAnchor.x;
            ptPos.y         = ((ptCamSize.y >> 1) - (ptFrameSize.x >> 1)) + ptAnchor.y;
            g_hudSurfaces.pReticle->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pShotPowerMeter != 0)
        {
            ptAnchor = g_hudSurfaces.pHeadCamFrame->GetPosition();
            ptPos.x  = ptAnchor.x + 0x23;
            ptPos.y  = ptAnchor.y + 0x10;
            g_hudSurfaces.pShotPowerMeter->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pExitMatchText != 0)
        {
            ptFrameSize = g_hudSurfaces.pExitMatchText->GetSize();
            ptPos.x     = (g_nPrevWidth - ptFrameSize.x) / 2;
            ptPos.y     = (g_nPrevHeight - ptFrameSize.y) / 2;
            g_hudSurfaces.pExitMatchText->SetPosition(ptPos);
        }

        if (g_hudSurfaces.pContinueText != 0)
        {
            ptFrameSize = g_hudSurfaces.pContinueText->GetSize();
            ptPos.x     = (g_nPrevWidth - ptFrameSize.x) / 2;
            ptPos.y     = (g_nPrevHeight - ptFrameSize.y) / 2;

            if (g_hudSurfaces.pExitMatchText != 0)
            {
                ptFrameSize = g_hudSurfaces.pExitMatchText->GetSize();
                ptPos.y     = ptPos.y + ptFrameSize.y + 6;
            }

            g_hudSurfaces.pContinueText->SetPosition(ptPos);
        }

        if (g_hudSurfaces.apTeamNames[0] != 0)
        {
            g_hudSurfaces.apTeamNames[0]->SetPosition(ptColumn);
            ptFrameSize = g_hudSurfaces.apTeamNames[0]->GetSize();
            ptColumn.y  = ptColumn.y + 2 + ptFrameSize.y;
        }

        for (nWidget = 0; nWidget < 4; nWidget++)
        {
            if (g_hudSurfaces.apPlayerWidgets[nWidget] != 0)
            {
                g_hudSurfaces.apPlayerWidgets[nWidget]->SetPosition(ptColumn);
                ptFrameSize = g_hudSurfaces.apPlayerWidgets[nWidget]->GetSize();
                ptColumn.y  = ptColumn.y + ptFrameSize.y + 2;
            }

            if (nWidget == 1)
            {
                ptColumn.y += 6;
            }
        }

        ptColumn.y += 0x1c;

        if (g_hudSurfaces.apTeamNames[1] != 0)
        {
            g_hudSurfaces.apTeamNames[1]->SetPosition(ptColumn);
            ptFrameSize = g_hudSurfaces.apTeamNames[1]->GetSize();
            ptColumn.y  = ptColumn.y + ptFrameSize.y + 2;
        }

        for (nWidget = 4; nWidget < 8; nWidget++)
        {
            if (g_hudSurfaces.apPlayerWidgets[nWidget] != 0)
            {
                g_hudSurfaces.apPlayerWidgets[nWidget]->SetPosition(ptColumn);
                ptFrameSize = g_hudSurfaces.apPlayerWidgets[nWidget]->GetSize();
                ptColumn.y  = ptColumn.y + ptFrameSize.y + 2;
            }

            if (nWidget == 5)
            {
                ptColumn.y += 6;
            }
        }
    }
}

// FUNCTION: HYPERX 0x0043e070
HUD::HUD(int)
{
    m_bMessageVisible  = 0;
    m_bShotPowerWanted = 0;
    m_bHeadCamWanted   = 0;
    m_bPassBarWanted   = 0;

    g_hudSurfaces.nHudSurfaceState = 0;

    LayoutSurfaces();
}

// FUNCTION: HYPERX 0x0043e0f0
void HUD::DrawHUD(int, HDC)
{
}

// Only restores InfoBarBase's vtable; slot nought calls it out of line.
// FUNCTION: HYPERX 0x0043e120
HUD::~HUD()
{
}

#define DAMAGE_COLOUR_FOLLOWED   0xffff
#define DAMAGE_COLOUR_FIRST_BAR  0xc8
#define DAMAGE_COLOUR_SECOND_BAR 0xff

#define ROK_OVERLAY_POSITIVE 0x20
#define ROK_OVERLAY_NEGATIVE 0x2f
#define ROK_OVERLAY_TOP      -0x15

#define OVERLAY_TOP 0x28

inline void HUD::AssignName(CDDTextSurface* pLabel, const char* psz)
{
    String* const pName = &pLabel->m_strText;

    pName->Assign(String::SafeStrlen(psz), psz);
}

// FUNCTION: HYPERX 0x0043e140
void HUD::LayoutSurfaces()
{
    HUD::PlaceSurfaces();
}

// FUNCTION: HYPERX 0x0043e150
void HUD::DrawScene(int bDraw)
{
    char szClock[CLOCK_BUFFER];

    PlayerObject* pSlot;
    int           nElapsed;

    const int nFollowed = g_pGameEngine->m_nViewedSlot;
    nElapsed            = g_pGameEngine->m_nPeriodLengthMs - g_pGameEngine->m_clock.nPauseStartMs + g_pGameEngine->m_clock.nPeriodStartMs;

    const int nSeconds = (int)((nElapsed % MS_PER_MINUTE) * g_flPerSecond);
    const int nMinutes = (int)(nElapsed * g_flPerMinute);

    const int nActive = g_frameTimer.bFrameTimesOn;

    if (g_nLastClockSeconds != nSeconds || nSeconds == 0)
    {
        g_nLastClockSeconds = nSeconds;

        // STRING: HYPERX 0x004c0d70
        sprintf(szClock, "%d:%02d", nMinutes, nSeconds);

        CDDTextSurface* const pLabel = g_hudSurfaces.pClockText;

        if (pLabel != 0)
        {
            HUD::AssignName(pLabel, szClock);

            pLabel->RestoreContents();
        }

        if (nActive != 0 && g_ddrawDevice.pFpsText != 0)
        {
            g_ddrawDevice.pFpsText->DrawText(g_szFpsText);
        }
    }

    if (nActive != 0)
    {
        g_ddrawDevice.pFpsText->Draw();
    }

    if (bDraw == 0)
    {
        return;
    }

    if (g_hudSurfaces.pClockText != 0)
    {
        g_hudSurfaces.pClockText->Draw();
    }

    if (g_hudSurfaces.pPeriodFrame != 0)
    {
        g_hudSurfaces.pPeriodFrame->Draw();
    }

    if (g_pGameEngine->m_clock.nPeriod != g_nLastOverlayPeriod)
    {
        if (g_hudSurfaces.pPeriodOverlay != 0)
        {
            g_hudSurfaces.pPeriodOverlay
                ->DispatchRect(g_anPeriodOverlayX[g_pGameEngine->m_clock.nPeriod],
                               OVERLAY_TOP,
                               0,
                               0);
        }
    }

    if (g_hudSurfaces.pPeriodOverlay != 0)
    {
        g_hudSurfaces.pPeriodOverlay->Draw();
    }

    if (g_hudSurfaces.pMessageFrame != 0 && g_hudSurfaces.pMessageText != 0 && m_bMessageVisible != 0)
    {
        g_hudSurfaces.pMessageFrame->Draw();
        g_hudSurfaces.pMessageText->Draw();
    }

    if (g_hudSurfaces.pRadar != 0)
    {
        g_hudSurfaces.pRadar->Update((short)nFollowed);

        if (nFollowed != 0)
        {
            g_hudSurfaces.pRadar->DrawObject(0);
        }

        if (nFollowed != 1)
        {
            g_hudSurfaces.pRadar->DrawObject(1);
        }

        if (nFollowed != 2)
        {
            g_hudSurfaces.pRadar->DrawObject(2);
        }

        if (nFollowed != 3)
        {
            g_hudSurfaces.pRadar->DrawObject(3);
        }

        g_hudSurfaces.pRadar->DrawObject(SLOT_ROK);

        g_hudSurfaces.pRadar->Draw();
    }

    if (g_hudSurfaces.pTeamABitmap != 0)
    {
        g_hudSurfaces.pTeamABitmap->Draw();
    }

    if (g_hudSurfaces.pTeamBBitmap != 0)
    {
        g_hudSurfaces.pTeamBBitmap->Draw();
    }

    if (g_hudSurfaces.pTeamAScoreText != 0)
    {
        if (g_bHomeScoreDirty != 0)
        {
            g_hudSurfaces.pTeamAScoreText->DrawText(g_szHomeScore);

            g_bHomeScoreDirty = 0;
        }
        else
        {
            g_hudSurfaces.pTeamAScoreText->DrawText(0);
        }
    }

    if (g_hudSurfaces.pTeamBScoreText != 0)
    {
        if (g_bAwayScoreDirty != 0)
        {
            g_hudSurfaces.pTeamBScoreText->DrawText(g_szAwayScore);

            g_bAwayScoreDirty = 0;
        }
        else
        {
            g_hudSurfaces.pTeamBScoreText->DrawText(0);
        }
    }

    if (g_hudSurfaces.pSpeedMeter != 0)
    {
        pSlot = g_pGameEngine->GetPlayer((short)nFollowed);

        Point size = g_hudSurfaces.pSpeedMeter->GetSize();
        g_hudSurfaces.pSpeedMeter->DrawWithWidth((int)(pSlot->m_stats.GetSpeedRatio() * size.x));
    }

    if (g_hudSurfaces.pPassBar != 0)
    {
        g_hudSurfaces.pPassBar->AutoScroll();
    }

    if (g_hudSurfaces.pTeamADamageBar1 != 0)
    {
        if (nFollowed == 0)
        {
            g_hudSurfaces.pTeamADamageBar1->SetColorMode(DAMAGE_COLOUR_FOLLOWED);
        }
        else
        {
            g_hudSurfaces.pTeamADamageBar1->SetColorMode(DAMAGE_COLOUR_FIRST_BAR);
        }

        if (nFollowed != g_anPeriodOverlayX[0])
        {
            g_hudSurfaces.pTeamADamageBar1->SetDirty();
        }

        pSlot = g_pGameEngine->GetPlayer(0);

        g_hudSurfaces.pTeamADamageBar1->ScrollTo((int)(pSlot->m_stats.GetScalar(RATING_DAMAGE) + 0.5));
    }

    if (g_hudSurfaces.pTeamADamageBar2 != 0)
    {
        if (nFollowed == 2)
        {
            g_hudSurfaces.pTeamADamageBar2->SetColorMode(DAMAGE_COLOUR_FOLLOWED);
        }
        else
        {
            g_hudSurfaces.pTeamADamageBar2->SetColorMode(DAMAGE_COLOUR_SECOND_BAR);
        }

        if (nFollowed != g_anPeriodOverlayX[0])
        {
            g_hudSurfaces.pTeamADamageBar2->SetDirty();
        }

        pSlot = g_pGameEngine->GetPlayer(2);

        g_hudSurfaces.pTeamADamageBar2->ScrollTo((int)(pSlot->m_stats.GetScalar(RATING_DAMAGE) + 0.5));
    }

    if (g_hudSurfaces.pTeamBDamageBar1 != 0)
    {
        if (nFollowed == 1)
        {
            g_hudSurfaces.pTeamBDamageBar1->SetColorMode(DAMAGE_COLOUR_FOLLOWED);
        }
        else
        {
            g_hudSurfaces.pTeamBDamageBar1->SetColorMode(DAMAGE_COLOUR_FIRST_BAR);
        }

        if (nFollowed != g_anPeriodOverlayX[0])
        {
            g_hudSurfaces.pTeamBDamageBar1->SetDirty();
        }

        pSlot = g_pGameEngine->GetPlayer(1);

        g_hudSurfaces.pTeamBDamageBar1->ScrollTo((int)(pSlot->m_stats.GetScalar(RATING_DAMAGE) + 0.5));
    }

    if (g_hudSurfaces.pTeamBDamageBar2 != 0)
    {
        if (nFollowed == 3)
        {
            g_hudSurfaces.pTeamBDamageBar2->SetColorMode(DAMAGE_COLOUR_FOLLOWED);
        }
        else
        {
            g_hudSurfaces.pTeamBDamageBar2->SetColorMode(DAMAGE_COLOUR_SECOND_BAR);
        }

        // The fourth bar marks the first dirty rather than itself, as in the original.
        if (nFollowed != g_anPeriodOverlayX[0])
        {
            g_hudSurfaces.pTeamADamageBar1->SetDirty();
        }

        pSlot = g_pGameEngine->GetPlayer(3);

        g_hudSurfaces.pTeamBDamageBar2->ScrollTo((int)(pSlot->m_stats.GetScalar(RATING_DAMAGE) + 0.5));
    }

    if (g_hudSurfaces.pTeamAFrame != 0)
    {
        g_hudSurfaces.pTeamAFrame->Draw();
    }

    if (g_hudSurfaces.pTeamBFrame != 0)
    {
        g_hudSurfaces.pTeamBFrame->Draw();
    }

    if (g_nSessionState != 0 && g_hudSurfaces.pNetworkIcon != 0)
    {
        g_hudSurfaces.pNetworkIcon->Draw();
    }

    if (g_hudSurfaces.pSmallText != 0)
    {
        const int nRokValue = g_pGameEngine->GetRok()->m_wRokValue;

        if (nRokValue != 0)
        {
            if (nRokValue != g_nLastRokValue)
            {
                g_nLastRokValue = nRokValue;

                // STRING: HYPERX 0x004c0d6c
                sprintf(g_szRokValueText, "x%d", abs(nRokValue));

                if (nRokValue > 0)
                {
                    g_hudSurfaces.pSmallText
                        ->DispatchRect(1, ROK_OVERLAY_POSITIVE, ROK_OVERLAY_TOP, 0);
                }
                else
                {
                    g_hudSurfaces.pSmallText
                        ->DispatchRect(1, ROK_OVERLAY_NEGATIVE, ROK_OVERLAY_TOP, 0);
                }
            }

            g_hudSurfaces.pSmallText->DrawText(g_szRokValueText);
        }
    }

    if (m_bQuitPromptVisible != 0)
    {
        if (g_pGameEngine->m_clock.nTickMs - m_nQuitPromptTick >= QUIT_PROMPT_INTERVAL)
        {
            HideQuitPrompt();
        }
        else
        {
            if (g_hudSurfaces.pExitMatchText != 0)
            {
                g_hudSurfaces.pExitMatchText->DrawText(g_hudSurfaces.pExitMatchString->m_pchData);
            }

            if (g_hudSurfaces.pContinueText != 0)
            {
                g_hudSurfaces.pContinueText->DrawText(g_hudSurfaces.pContinueString->m_pchData);
            }
        }
    }

    HUD::UpdateOwnScoreboard();
    HUD::UpdateFoeScoreboard();
}

// The own-team row and the opposing row are one function written twice.
// FUNCTION: HYPERX 0x0043e740
void __stdcall HUD::UpdateOwnScoreboard(void)
{
    char       szName[SCORE_NAME_BUFFER];
    int        anSeat[2];
    int        anLevel[4];
    TeamBlock* pTeam;
    TeamBlock* pOther;

    int nColour;
    int i;

    short nBase = 0;

    if (g_pGameEngine == 0)
    {
        return;
    }

    const short nSlot    = g_pGameEngine->m_nViewedSlot;
    int         nChanged = 0;
    if ((nSlot & 1) != 0)
    {
        pOther = g_pGameEngine->m_pTeamA;
        pTeam  = g_pGameEngine->m_pTeamB;
        nBase  = 1;

        if (g_nOwnRowStampB != g_pGameEngine->m_nTeamAwayIndex)
        {
            nChanged        = 1;
            g_nOwnRowStampB = g_pGameEngine->m_nTeamAwayIndex;
        }
    }
    else
    {
        pOther = g_pGameEngine->m_pTeamB;
        pTeam  = g_pGameEngine->m_pTeamA;

        if (g_nOwnRowStampA != g_pGameEngine->m_nTeamHomeIndex)
        {
            nChanged        = 1;
            g_nOwnRowStampA = g_pGameEngine->m_nTeamHomeIndex;
        }
    }

    // The -1 sentinel matches no real slot.
    if (nSlot != g_nOwnRowSlot)
    {
        g_nOwnRowSlot = nSlot;
        nChanged      = 1;
    }

    anSeat[0] = pTeam->aSeat[0];
    anSeat[1] = pTeam->aSeat[1];

    if (anSeat[0] != g_nOwnRowFormationRow || anSeat[1] != g_nOwnRowFormationColumn)
    {
        g_nOwnRowFormationRow    = anSeat[0];
        g_nOwnRowFormationColumn = anSeat[1];
        nChanged                 = 1;
    }

    PlayerObject* pSlot = g_pGameEngine->GetPlayer(nBase);

    anLevel[0] = (int)pSlot->m_stats.GetScalar(RATING_DAMAGE);

    nBase += 2;

    pSlot = g_pGameEngine->GetPlayer(nBase);

    anLevel[1] = (int)pSlot->m_stats.GetScalar(RATING_DAMAGE);

    const ReserveRows* pReserves = g_pGameEngine->GetReserveRows(pTeam);

    anLevel[2] = (int)pTeam->GetDamageRating(pReserves->anRow[0]);
    anLevel[3] = (int)pTeam->GetDamageRating(pReserves->anRow[1]);

    if (anLevel[0] != g_nOwnRowDamage0 || anLevel[1] != g_nOwnRowDamage1 || anLevel[2] != g_nOwnRowRating0 || anLevel[3] != g_nOwnRowRating1)
    {
        g_nOwnRowDamage0 = anLevel[0];
        g_nOwnRowDamage1 = anLevel[1];
        g_nOwnRowRating0 = anLevel[2];
        g_nOwnRowRating1 = anLevel[3];

        nChanged = 1;
    }

    if (nChanged != 0)
    {
        const int nHalf = (nSlot >> 1) & 1;

        for (i = 0; i < 4; i++)
        {
            if (g_hudSurfaces.apPlayerWidgets[i] != 0)
            {
                const float flGate = DamageGate();

                if (anLevel[i] <= flGate)
                {
                    nColour = COLOUR_BLACK;
                }
                else if (nHalf == i)
                {
                    nColour = COLOUR_FOLLOWED;
                }
                else
                {
                    nColour = RowColour(i);
                }

                if (i >= 2)
                {
                    // STRING: HYPERX 0x004c0d78
                    sprintf(szName, "%c. %s", i + '0' - 1, pTeam->GetRosterName(g_anOwnRowNameOrder[pReserves->anRow[i - 2]]));
                }
                else
                {
                    strcpy(szName,
                           pTeam->GetRosterName(g_anOwnRowNameOrder[anSeat[i]]));
                }

                g_hudSurfaces.apPlayerWidgets[i]->SetColorMode(nColour);

                SetLabelText(g_hudSurfaces.apPlayerWidgets[i], szName);
            }
        }

        g_hudSurfaces.apTeamNames[0]->SetColorMode(COLOUR_BENCH);
        g_hudSurfaces.apTeamNames[1]->SetColorMode(COLOUR_BENCH);

        SetLabelText(g_hudSurfaces.apTeamNames[0], pTeam->GetName());
        SetLabelText(g_hudSurfaces.apTeamNames[1], pOther->GetName());
    }

    for (i = 0; i < 4; i++)
    {
        if (anLevel[i] > 0 && g_hudSurfaces.apPlayerWidgets[i] != 0)
        {
            g_hudSurfaces.apPlayerWidgets[i]->Draw();
        }
    }

    g_hudSurfaces.apTeamNames[0]->Draw();
    g_hudSurfaces.apTeamNames[1]->Draw();
}

// FUNCTION: HYPERX 0x0043eb30
void __stdcall HUD::UpdateFoeScoreboard(void)
{
    char       szName[SCORE_NAME_BUFFER];
    int        anSeat[2];
    int        anLevel[4];
    TeamBlock* pTeam;
    int        nColour;
    int        i;

    short nBase = 0;

    if (g_pGameEngine == 0)
    {
        return;
    }

    const unsigned int nSlot    = g_pGameEngine->m_nViewedSlot;
    int                nChanged = 0;

    if ((nSlot & 1) != 0)
    {
        pTeam = g_pGameEngine->m_pTeamA;

        if (g_nFoeRowStampA != g_pGameEngine->m_nTeamHomeIndex)
        {
            nChanged        = 1;
            g_nFoeRowStampA = g_pGameEngine->m_nTeamHomeIndex;
        }
    }
    else
    {
        pTeam = g_pGameEngine->m_pTeamB;
        nBase = 1;

        if (g_nFoeRowStampB != g_pGameEngine->m_nTeamAwayIndex)
        {
            nChanged        = 1;
            g_nFoeRowStampB = g_pGameEngine->m_nTeamAwayIndex;
        }
    }

    if (nSlot != (unsigned int)g_nFoeRowSlot)
    {
        g_nFoeRowSlot = nSlot;
        nChanged      = 1;
    }

    anSeat[0] = pTeam->aSeat[0];
    anSeat[1] = pTeam->aSeat[1];

    if (anSeat[0] != g_nFoeRowFormationRow || anSeat[1] != g_nFoeRowFormationColumn)
    {
        g_nFoeRowFormationRow    = anSeat[0];
        g_nFoeRowFormationColumn = anSeat[1];
        nChanged                 = 1;
    }

    PlayerObject* pSlot = g_pGameEngine->GetPlayer(nBase);

    anLevel[0] = (int)pSlot->m_stats.GetScalar(RATING_DAMAGE);

    nBase += 2;

    pSlot = g_pGameEngine->GetPlayer(nBase);

    anLevel[1] = (int)pSlot->m_stats.GetScalar(RATING_DAMAGE);

    const ReserveRows* pReserves = g_pGameEngine->GetReserveRows(pTeam);

    anLevel[2] = (int)pTeam->GetDamageRating(pReserves->anRow[0]);
    anLevel[3] = (int)pTeam->GetDamageRating(pReserves->anRow[1]);

    if (anLevel[0] != g_nFoeRowDamage0 || anLevel[1] != g_nFoeRowDamage1 || anLevel[2] != g_nFoeRowRating0 || anLevel[3] != g_nFoeRowRating1)
    {
        g_nFoeRowDamage0 = anLevel[0];
        g_nFoeRowDamage1 = anLevel[1];
        g_nFoeRowRating0 = anLevel[2];
        g_nFoeRowRating1 = anLevel[3];

        nChanged = 1;
    }

    if (nChanged != 0)
    {
        for (i = 0; i < FOE_ROWS; i++)
        {
            if (g_hudSurfaces.apPlayerWidgets[4 + i] != 0)
            {
                const float flGate = DamageGate();

                if (anLevel[i] <= flGate)
                {
                    nColour = COLOUR_BLACK;
                }
                else
                {
                    nColour = RowColour(i);
                }

                if (i >= FOE_ACTIVE_ROWS)
                {
                    strcpy(szName,
                           pTeam->GetRosterName(g_anFoeRowNameOrder[pReserves->anRow[i - 2]]));
                }
                else
                {
                    strcpy(szName,
                           pTeam->GetRosterName(g_anFoeRowNameOrder[anSeat[i]]));
                }

                g_hudSurfaces.apPlayerWidgets[4 + i]->SetColorMode(nColour);

                SetLabelText(g_hudSurfaces.apPlayerWidgets[4 + i], szName);
            }
        }
    }

    for (i = 0; i < FOE_ROWS; i++)
    {
        if (anLevel[i] > 0 && g_hudSurfaces.apPlayerWidgets[4 + i] != 0)
        {
            g_hudSurfaces.apPlayerWidgets[4 + i]->Draw();
        }
    }
}

// FUNCTION: HYPERX 0x0043ee50
void HUD::SetMessageText(const char* pszText, unsigned long nCode)
{
    int nLength;

    CDDTextSurface* pFrame = g_hudSurfaces.pMessageText;

    if (pFrame != 0)
    {
        String* const pText = &pFrame->m_strText;

        if (pszText == 0)
        {
            nLength = 0;
        }
        else
        {
            nLength = lstrlen(pszText);
        }

        pText->Assign(nLength, pszText);

        pFrame->RestoreContents();

        if (nCode != -1)
        {
            g_hudSurfaces.pMessageText->SetColorMode(nCode);
        }

        m_bMessageVisible = 1;
    }
}

// FUNCTION: HYPERX 0x0043eeb0
void HUD::ShowMessage()
{
    m_bMessageVisible = 1;
}

// FUNCTION: HYPERX 0x0043eec0
void HUD::HideMessage()
{
    m_bMessageVisible = 0;
}

// FUNCTION: HYPERX 0x0043eed0
void HUD::SetHomeLogo(char* pTeamData)
{
    if (g_hudSurfaces.pTeamABitmap != 0)
    {
        g_hudSurfaces.pTeamABitmap->SetFile(pTeamData, 1);
    }
}

// FUNCTION: HYPERX 0x0043eef0
void HUD::SetAwayLogo(char* pTeamData)
{
    if (g_hudSurfaces.pTeamBBitmap != 0)
    {
        g_hudSurfaces.pTeamBBitmap->SetFile(pTeamData, 1);
    }
}

// FUNCTION: HYPERX 0x0043ef10
Texture* HUD::GetHeadCamSurface()
{
    return g_hudSurfaces.pHeadCamFrame;
}

// FUNCTION: HYPERX 0x0043ef20
void HUD::ShowHeadCam()
{
    if (g_hudSurfaces.pHeadCamFrame != 0)
    {
        m_bHeadCamWanted = 1;

        g_hudSurfaces.pHeadCamFrame->SetVisible(1);
    }
}

// FUNCTION: HYPERX 0x0043ef40
void HUD::SetHeadCamText(const char* pszText)
{
    int nLength;

    CDDBitMap* const pOwner = g_hudSurfaces.pHeadCamFrame;

    if (pOwner != 0)
    {
        CDDTextSurface* pFrame = pOwner->m_pOverlay;

        if (pFrame != 0)
        {
            String* const pText = &pFrame->m_strText;

            if (pszText == 0)
            {
                nLength = 0;
            }
            else
            {
                nLength = lstrlen(pszText);
            }

            pText->Assign(nLength, pszText);

            pFrame->RestoreContents();
        }
    }
}

// The point is shrunk by the pair twice over.

// FUNCTION: HYPERX 0x0043ef90
ViewExtent HUD::GetHeadCamSize()
{
    ViewExtent pt;

    pt.Set(0, 0);

    if (g_hudSurfaces.pHeadCamFrame != 0)
    {
        Point amounts = g_hudSurfaces.pHeadCamFrame->GetInset();

        pt.Point::operator=(g_hudSurfaces.pHeadCamFrame->GetSize());

        pt.ShrinkClamp(amounts.x, amounts.y);
        pt.ShrinkClamp(amounts.x, amounts.y);
    }

    return pt;
}

// The scratch point is zeroed and never read.
// FUNCTION: HYPERX 0x0043f010
ViewExtent HUD::GetHeadCamOrigin()
{
    ViewExtent pt;
    Point      scratch;

    scratch.Zero();

    pt.y = 0;
    pt.x = 0;

    return pt;
}

// The truncated reading is also kept in a global nothing else reads.
// FUNCTION: HYPERX 0x0043f030
void HUD::SetShotPower(float flPower)
{
    if (g_hudSurfaces.pShotPowerMeter != 0)
    {
        const int nPosition = (int)flPower;

        g_hudSurfaces.nShotPowerPosition = nPosition;
        g_hudSurfaces.nHudSurfaceState   = 1;

        m_bShotPowerWanted = 1;

        g_hudSurfaces.pShotPowerMeter->SetVisible(1);

        g_hudSurfaces.pShotPowerMeter->ScrollTo(nPosition);
    }
}

// FUNCTION: HYPERX 0x0043f080
void HUD::ShowShotPower()
{
    if (g_hudSurfaces.pShotPowerMeter != 0)
    {
        g_hudSurfaces.nHudSurfaceState = 1;

        m_bShotPowerWanted = 1;

        g_hudSurfaces.pShotPowerMeter->SetVisible(1);
    }
}

// FUNCTION: HYPERX 0x0043f0b0
void HUD::SetReticlePos(int nDx, int nDy)
{
    if (g_hudSurfaces.pHeadCamFrame != 0 && g_hudSurfaces.pReticle != 0)
    {
        Point pt = g_hudSurfaces.pHeadCamFrame->GetPosition();

        pt.Offset(g_hudSurfaces.pHeadCamFrame->GetInset());
        pt.Offset(nDx, nDy);

        g_hudSurfaces.pReticle->SetPosition(pt);
    }
}

// FUNCTION: HYPERX 0x0043f130
ViewExtent HUD::GetReticleSize()
{
    ViewExtent pt;
    pt.Set(0, 0);

    if (g_hudSurfaces.pReticle != 0)
    {
        pt.Point::operator=(g_hudSurfaces.pReticle->GetSize());
    }

    return pt;
}

// Mode zero hides the reticle; the other three show it and name a file.

#define RETICLE_FILE_KEEP 1

// FUNCTION: HYPERX 0x0043f190
void HUD::SetReticleState(int nMode)
{
    if (g_hudSurfaces.pReticle != 0)
    {
        switch (nMode)
        {
            case 0:
                g_hudSurfaces.pReticle->SetVisible(0);
                return;

            case 1:
                g_hudSurfaces.pReticle->SetVisible(1);

                // STRING: HYPERX 0x004c0844
                g_hudSurfaces.pReticle->SetFile("Reticl2", RETICLE_FILE_KEEP);
                return;

            case 2:
                g_hudSurfaces.pReticle->SetVisible(1);

                // STRING: HYPERX 0x004c0d88
                g_hudSurfaces.pReticle->SetFile("Reticl3", RETICLE_FILE_KEEP);
                return;

            case 3:
                g_hudSurfaces.pReticle->SetVisible(1);

                // STRING: HYPERX 0x004c0d80
                g_hudSurfaces.pReticle->SetFile("Reticl4", RETICLE_FILE_KEEP);
                return;
        }
    }
}

// 100 by 10 when there is no bar.
// FUNCTION: HYPERX 0x0043f220
ViewExtent HUD::GetPassBarSize()
{
    ViewExtent pt;

    pt.Set(100, 10);

    if (g_hudSurfaces.pPassBar != 0)
    {
        pt.x = g_hudSurfaces.pPassBar->m_ptRange.x;
        pt.y = g_hudSurfaces.pPassBar->m_ptRange.y;
    }

    return pt;
}

// FUNCTION: HYPERX 0x0043f280
void HUD::SetPassBarMarker(int nPosition)
{
    if (g_hudSurfaces.pPassBar != 0)
    {
        m_bPassBarWanted = 1;

        g_hudSurfaces.pPassBar->SetVisible(1);

        g_hudSurfaces.pPassBar->DrawAtPos(nPosition);
    }
}

// FUNCTION: HYPERX 0x0043f2b0
void HUD::ShowPassBar()
{
    if (g_hudSurfaces.pPassBar != 0)
    {
        m_bPassBarWanted = 1;

        g_hudSurfaces.pPassBar->SetVisible(1);
    }
}

// FUNCTION: HYPERX 0x0043f2d0
void HUD::ShowQuitPrompt()
{
    m_bQuitPromptVisible = 1;

    if (g_hudSurfaces.pExitMatchText != 0)
    {
        g_hudSurfaces.pExitMatchText->SetVisible(1);
    }

    if (g_hudSurfaces.pContinueText != 0)
    {
        g_hudSurfaces.pContinueText->SetVisible(1);
    }

    m_nQuitPromptTick = g_pGameEngine->m_clock.nTickMs;
}

// FUNCTION: HYPERX 0x0043f320
void HUD::HideQuitPrompt()
{
    m_bQuitPromptVisible = 0;

    if (g_hudSurfaces.pExitMatchText != 0)
    {
        g_hudSurfaces.pExitMatchText->SetVisible(0);
    }

    if (g_hudSurfaces.pContinueText != 0)
    {
        g_hudSurfaces.pContinueText->SetVisible(0);
    }
}

int g_nOwnRowStampA          = -1;
int g_nOwnRowStampB          = -1;
int g_nOwnRowSlot            = -1;
int g_nOwnRowFormationRow    = -1;
int g_nOwnRowFormationColumn = -1;
int g_nOwnRowDamage0         = -1;
int g_nOwnRowDamage1         = -1;
int g_nOwnRowRating0         = -1;
int g_nOwnRowRating1         = -1;

// Which of the team's player records each of the four scoreboard positions names.
int g_anOwnRowNameOrder[4]   = { 0, 1, 3, 4 };
int g_nFoeRowStampA          = -1;
int g_nFoeRowStampB          = -1;
int g_nFoeRowSlot            = -1;
int g_nFoeRowFormationRow    = -1;
int g_nFoeRowFormationColumn = -1;
int g_nFoeRowDamage0         = -1;
int g_nFoeRowDamage1         = -1;
int g_nFoeRowRating0         = -1;
int g_nFoeRowRating1         = -1;

int g_anFoeRowNameOrder[4] = { 0, 1, 3, 4 };
int g_anPeriodOverlayX[4]  = { -1, 19, 38, 57 };
int g_nLastRokValue        = -1;

void HUD::BeginTick()
{
    if (g_hudSurfaces.pHeadCamFrame != 0)
    {
        g_hudSurfaces.pHeadCamFrame->SetVisible(0);
    }

    m_bHeadCamWanted = 0;

    if (g_hudSurfaces.pShotPowerMeter != 0)
    {
        g_hudSurfaces.pShotPowerMeter->SetVisible(0);
    }

    m_bShotPowerWanted = 0;

    g_hudSurfaces.nHudSurfaceState = 0;

    if (g_hudSurfaces.pPassBar != 0)
    {
        g_hudSurfaces.pPassBar->SetVisible(0);
    }

    m_bPassBarWanted = 0;

    g_pMessageQueue->Update();
}
