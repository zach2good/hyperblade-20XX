#include "common/compat.h"
#include "input/inputevent.h"
#include "engine/teamsblock.h"
#include "globals.h"
#include "ui/screen.h"
#include "ui/torchbitmaps.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "engine/gameengine.h"
#include "ui/buttonwidget.h"
#include "ui/widget.h"
#include "ui/framepos.h"
#include "ui/pausemenu.h"
#include "ui/screens/headtoheadsetupuiscreen.h"
#include "ui/loopflipbookwidget.h"
#include "ui/displaywidget.h"
#include "ui/saveobject.h"
#include "graphics/gdi.h"
#include "engine/game.h"
#include "common/stub.h"

// A team's two ratings cards, in the order the drome selector uses.
struct TeamRatingCards
{
    const char* pszStarters;
    const char* pszScrubs;
};

int g_nDromeTeamHome = 5;
int g_nDromeTeamAway = 10;
int g_nDromeArena    = 1;

// GLOBAL: HYPERX 0x004c1fe0
TeamRatingCards g_aTeamRatingCards[14] = {
    { "chi_cb", "chi_cc" },
    { "hou_ab", "hou_kr" },
    { "jam_es", "jam_nb" },
    { "veg_rw", "veg_vo" },
    { "lnd_ls", "lnd_mp" },
    { "los_am", "los_cs" },
    { "mos_ab", "mos_as" },
    { "mun_dj", "mun_kw" },
    { "nem_06", "nem_08" },
    { "new_jn", "new_jr" },
    { "sea_rk", "sea_tk" },
    { "tok_hs", "tok_kn" },
    { "tor_jh", "tor_sl" },
    { "war_aw", "war_ws" }
};

// FUNCTION: HYPERX 0x00444190
HeadToHeadSetupUIScreen::~HeadToHeadSetupUIScreen() = default;

// FUNCTION: HYPERX 0x004441c0
HeadToHeadSetupUIScreen::HeadToHeadSetupUIScreen()
{
    m_nLastWidget = H2H_LAST_WIDGET;

    m_apWidgets[UI_WIDGET_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 4]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 3]->SetHidden(1);
    m_nTeamCount      = g_pGameEngine->m_pTeamTable->GetTeamCount();
    m_nSelection      = 1;
    m_nSelectionLimit = 14;

    // STRING: HYPERX 0x004c21d0
    m_hBackground = GDI::LoadBitmapAsBackground("Hviewp", &m_bPaletted);

    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);

    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->ClearFrames();
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetDirtyFlag(1);
    // STRING: HYPERX 0x004c21c8
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AppendFrame(GDI::LoadBitmapHandle("2three3"));
    // STRING: HYPERX 0x004c21c0
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AppendFrame(GDI::LoadBitmapHandle("2three4"));
    // STRING: HYPERX 0x004c21b8
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AppendFrame(GDI::LoadBitmapHandle("2three5"));
    // STRING: HYPERX 0x004c21b0
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AppendFrame(GDI::LoadBitmapHandle("2gray6"));
    // STRING: HYPERX 0x004c21a8
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AppendFrame(GDI::LoadBitmapHandle("2three1"));
    // STRING: HYPERX 0x004c21a0
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AppendFrame(GDI::LoadBitmapHandle("2three2"));
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AppendFrame(GDI::LoadBitmapHandle("2three3"));

    m_apWidgets[H2H_HOME_TEAM_BUTTON]->ClearFrames();
    m_apWidgets[H2H_HOME_TEAM_BUTTON]->SetDepressed(1);
    // STRING: HYPERX 0x004c2198
    m_apWidgets[H2H_HOME_TEAM_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Homtem"));
    // STRING: HYPERX 0x004c2190
    m_apWidgets[H2H_HOME_TEAM_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Homtemr"));
    // STRING: HYPERX 0x004c2188
    m_apWidgets[H2H_HOME_TEAM_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Homtemd"));

    m_apWidgets[H2H_ARENA_BUTTON]->ClearFrames();
    // STRING: HYPERX 0x004bde6c
    m_apWidgets[H2H_ARENA_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Arena"));
    // STRING: HYPERX 0x004bde64
    m_apWidgets[H2H_ARENA_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Arenar"));
    // STRING: HYPERX 0x004bde5c
    m_apWidgets[H2H_ARENA_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Arenad"));

    m_apWidgets[H2H_VISITOR_TEAM_BUTTON]->ClearFrames();
    // STRING: HYPERX 0x004c2180
    m_apWidgets[H2H_VISITOR_TEAM_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Vistem"));
    // STRING: HYPERX 0x004c2178
    m_apWidgets[H2H_VISITOR_TEAM_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Vistemr"));
    // STRING: HYPERX 0x004c2170
    m_apWidgets[H2H_VISITOR_TEAM_BUTTON]->AppendFrame(GDI::LoadBitmapHandle("Vistemd"));

    m_apWidgets[UI_WIDGET_OKAY]->ClearFrames();
    // STRING: HYPERX 0x004c2168
    m_apWidgets[UI_WIDGET_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okay"));
    // STRING: HYPERX 0x004c2160
    m_apWidgets[UI_WIDGET_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okayr"));
    // STRING: HYPERX 0x004c2158
    m_apWidgets[UI_WIDGET_OKAY]->AppendFrame(GDI::LoadBitmapHandle("Okayd"));

    m_apWidgets[UI_WIDGET_CANCEL]->ClearFrames();
    // STRING: HYPERX 0x004c2150
    m_apWidgets[UI_WIDGET_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Cancl"));
    // STRING: HYPERX 0x004c2148
    m_apWidgets[UI_WIDGET_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Canclr"));
    // STRING: HYPERX 0x004c2140
    m_apWidgets[UI_WIDGET_CANCEL]->AppendFrame(GDI::LoadBitmapHandle("Cancld"));
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDepressed(1);

    m_apWidgets[UI_WIDGET_LEFT_ARROW]->ClearFrames();
    // STRING: HYPERX 0x004bde3c
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AppendFrame(GDI::LoadBitmapHandle("Stleft"));
    // STRING: HYPERX 0x004bde34
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AppendFrame(GDI::LoadBitmapHandle("Stleftr"));
    // STRING: HYPERX 0x004bde2c
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AppendFrame(GDI::LoadBitmapHandle("Stleftd"));

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->ClearFrames();
    // STRING: HYPERX 0x004bde54
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AppendFrame(GDI::LoadBitmapHandle("Strite"));
    // STRING: HYPERX 0x004bde4c
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AppendFrame(GDI::LoadBitmapHandle("Striter"));
    // STRING: HYPERX 0x004bde44
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AppendFrame(GDI::LoadBitmapHandle("Strited"));
    // STRING: HYPERX 0x004c2138
    m_hBitmap = GDI::LoadBitmapHandle("Chipl01");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    GDI::SafeDeleteBitmap(m_hBitmap);
    AppendWidget(new LoopFlipbookWidget(206, 63, m_bitmap.nWidth, m_bitmap.nHeight, 1, 1, 19, 0, 19, 19, this, 1, -1));
    for (int i = 0; i < 20; i++)
    {
        m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(nullptr);
    }
    m_apWidgets[H2H_PLAYER_PORTRAITS]->SetHidden(1);
    AppendWidget(new DisplayWidget(0, 0, 1, 1, this, -1));
    m_apWidgets[H2H_TEAM_NAME]->AppendFrame(nullptr);
    m_apWidgets[H2H_TEAM_NAME]->SetHidden(1);
    AppendWidget(new DisplayWidget(9, 101, 1, 1, this, -1));
    m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(nullptr);
    m_apWidgets[H2H_RATINGS_CARD]->SetHidden(1);
    // STRING: HYPERX 0x004c2130
    m_hBitmap = GDI::LoadBitmapHandle("startr1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    AppendWidget(new ButtonWidget(0, 7, 67, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(nullptr);
    m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(nullptr);
    m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(nullptr);
    m_apWidgets[H2H_STARTERS_TAB]->SetHidden(1);
    m_apWidgets[H2H_STARTERS_TAB]->SetDepressed(1);
    m_hBitmap = GDI::LoadBitmapHandle("startr1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    AppendWidget(new ButtonWidget(0, 107, 67, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
    m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(nullptr);
    m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(nullptr);
    m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(nullptr);
    m_apWidgets[H2H_SCRUBS_TAB]->SetHidden(1);
    m_apWidgets[H2H_SCRUBS_TAB]->SetDepressed(0);
    AppendWidget(new DisplayWidget(430, 67, 1, 1, this, -1));
    m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(nullptr);
    m_apWidgets[H2H_TEAM_TEXT]->SetHidden(1);
    AppendWidget(new DisplayWidget(580, 9, 1, 1, this, -1));
    m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(nullptr);
    m_apWidgets[H2H_TEAM_LOGO]->SetHidden(1);

    InitAllWidgets();

    g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), 1);

    g_nDromeArena = g_pGameEngine->m_pSaveObject
                        ->WrapToNextValidTeam(static_cast<short>(g_nDromeArena), 1);

    g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                           ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 1);

    StepHomeSelection(1);
    StepArenaSelection(1);
    StepAwaySelection(1);

    m_nSelectionMode = H2H_HOME_TEAM_BUTTON;

    RefreshSelection();
}

// FUNCTION: HYPERX 0x00444a10
void HeadToHeadSetupUIScreen::OnFirstPageTabClick()
{
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDirtyFlag(1);

    m_nSelectionMode = H2H_HOME_TEAM_BUTTON;

    RefreshSelection();
}

// FUNCTION: HYPERX 0x00444a70
void HeadToHeadSetupUIScreen::OnSecondPageTabClick()
{
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDirtyFlag(1);

    m_nSelectionMode = H2H_ARENA_BUTTON;

    RefreshSelection();
}

// FUNCTION: HYPERX 0x00444ad0
void HeadToHeadSetupUIScreen::OnThirdPageTabClick()
{
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDirtyFlag(1);

    m_nSelectionMode = H2H_VISITOR_TEAM_BUTTON;

    RefreshSelection();
}

// Only the two team columns have a ratings card; the arena column shows none.
// FUNCTION: HYPERX 0x00444b40
void HeadToHeadSetupUIScreen::ShowStartersCard()
{
    m_apWidgets[H2H_RATINGS_CARD]->ClearFrames();

    if (m_nSelectionMode == H2H_HOME_TEAM_BUTTON)
    {
        m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(
            GDI::LoadBitmapHandle(g_aTeamRatingCards[g_nDromeTeamHome].pszStarters));
        m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
    }
    else if (m_nSelectionMode == H2H_VISITOR_TEAM_BUTTON)
    {
        m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(
            GDI::LoadBitmapHandle(g_aTeamRatingCards[g_nDromeTeamAway].pszStarters));
        m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
    }

    m_apWidgets[H2H_RATINGS_CARD]->SetDirtyFlag(1);
    m_apWidgets[H2H_RATINGS_CARD]->SetHidden(0);

    InitFrame();
}

// FUNCTION: HYPERX 0x00444bd0
void HeadToHeadSetupUIScreen::ShowScrubsCard()
{
    m_apWidgets[H2H_RATINGS_CARD]->ClearFrames();

    if (m_nSelectionMode == H2H_HOME_TEAM_BUTTON)
    {
        m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(
            GDI::LoadBitmapHandle(g_aTeamRatingCards[g_nDromeTeamHome].pszScrubs));
        m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
    }
    else if (m_nSelectionMode == H2H_VISITOR_TEAM_BUTTON)
    {
        m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(
            GDI::LoadBitmapHandle(g_aTeamRatingCards[g_nDromeTeamAway].pszScrubs));
        m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
    }

    m_apWidgets[H2H_RATINGS_CARD]->SetDirtyFlag(1);
    m_apWidgets[H2H_RATINGS_CARD]->SetHidden(0);

    InitFrame();
}

// Two players can share an arena, so the arena column skips no pick.
// FUNCTION: HYPERX 0x00444c60
void HeadToHeadSetupUIScreen::OnLeftArrowClick()
{
    switch (m_nSelectionMode)
    {
        case H2H_ARENA_BUTTON:
            g_nDromeArena--;

            if (g_nDromeArena < 0)
            {
                g_nDromeArena = m_nTeamCount - 1;
            }

            g_nDromeArena = g_pGameEngine->m_pSaveObject
                                ->WrapToNextValidTeam(static_cast<short>(g_nDromeArena), 0);

            StepArenaSelection(0);
            RefreshSelection();
            break;

        case H2H_HOME_TEAM_BUTTON:
            g_nDromeTeamHome--;

            g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), 0);

            if (g_nDromeTeamHome == g_nDromeTeamAway)
            {
                g_nDromeTeamHome--;
            }

            if (g_nDromeTeamHome < 0)
            {
                g_nDromeTeamHome = m_nTeamCount - 1;

                if (g_nDromeTeamHome == g_nDromeTeamAway)
                {
                    g_nDromeTeamHome--;
                }
            }

            g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), 0);

            StepHomeSelection(0);
            RefreshSelection();
            break;

        case H2H_VISITOR_TEAM_BUTTON:
            g_nDromeTeamAway--;

            g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                                   ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 0);

            if (g_nDromeTeamAway == g_nDromeTeamHome)
            {
                g_nDromeTeamAway--;
            }

            if (g_nDromeTeamAway < 0)
            {
                g_nDromeTeamAway = m_nTeamCount - 1;

                if (g_nDromeTeamAway == g_nDromeTeamHome)
                {
                    g_nDromeTeamAway--;
                }
            }

            g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                                   ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 0);

            StepAwaySelection(0);
            RefreshSelection();
            break;
    }
}

// Wrapping lands on zero unless the other team is there, then on one.
// FUNCTION: HYPERX 0x00444dc0
void HeadToHeadSetupUIScreen::OnRightArrowClick()
{
    GameEngine* pEngine;
    int         nTeam;
    int         nOther;

    switch (m_nSelectionMode)
    {
        case H2H_HOME_TEAM_BUTTON:
            nTeam            = g_nDromeTeamHome + 1;
            pEngine          = g_pGameEngine;
            g_nDromeTeamHome = nTeam;

            pEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(nTeam), 1);

            nTeam  = g_nDromeTeamHome;
            nOther = g_nDromeTeamAway;

            if (nTeam == nOther)
            {
                nTeam++;
                g_nDromeTeamHome = nTeam;
            }

            if (nTeam >= m_nTeamCount)
            {
                nTeam            = 0;
                g_nDromeTeamHome = nTeam;

                if (nOther == 0)
                {
                    nTeam            = 1;
                    g_nDromeTeamHome = nTeam;
                }
            }

            pEngine = g_pGameEngine;

            pEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(nTeam), 1);

            StepHomeSelection(1);
            break;

        case H2H_ARENA_BUTTON:
            g_nDromeArena++;

            if (g_nDromeArena >= m_nTeamCount)
            {
                g_nDromeArena = 0;
            }

            pEngine = g_pGameEngine;

            nTeam = pEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(g_nDromeArena), 1);

            g_nDromeArena = nTeam;

            StepArenaSelection(1);
            break;

        case H2H_VISITOR_TEAM_BUTTON:
            g_nDromeTeamAway++;

            nTeam = g_pGameEngine->m_pSaveObject
                        ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 1);

            nOther = g_nDromeTeamHome;

            g_nDromeTeamAway = nTeam;

            if (nTeam == nOther)
            {
                nTeam++;
                g_nDromeTeamAway = nTeam;
            }

            if (nTeam >= m_nTeamCount)
            {
                nTeam            = 0;
                g_nDromeTeamAway = nTeam;

                if (nOther == 0)
                {
                    nTeam            = 1;
                    g_nDromeTeamAway = nTeam;
                }
            }

            pEngine = g_pGameEngine;

            nTeam = pEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(nTeam), 1);

            g_nDromeTeamAway = nTeam;

            StepAwaySelection(1);
            break;

        default:
            return;
    }

    RefreshSelection();
}

// The trip bound is the team count, so a column with nothing selectable gives up.
// FUNCTION: HYPERX 0x00444f30
void HeadToHeadSetupUIScreen::StepHomeSelection(int nStep)
{
    int nLocked;

    int nRemaining = m_nTeamCount;

    BuildLockedFlagsArray();

    if (nStep == 0)
    {
        g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), nStep);

        if (g_nDromeTeamHome == g_nDromeTeamAway)
        {
            g_nDromeTeamHome--;
        }

        if (g_nDromeTeamHome < 0)
        {
            g_nDromeTeamHome = m_nTeamCount - 1;

            if (g_nDromeTeamHome == g_nDromeTeamAway)
            {
                g_nDromeTeamHome--;
            }
        }

        g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), 0);

        nLocked = 0;
        if (m_nLockedCount != 0)
        {
            nLocked = m_pLockedFlags[g_nDromeTeamHome];
        }

        nRemaining--;

        while (nLocked != 0 && nRemaining != 0)
        {
            g_nDromeTeamHome--;

            g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), 0);

            if (g_nDromeTeamHome == g_nDromeTeamAway)
            {
                g_nDromeTeamHome--;
            }

            if (g_nDromeTeamHome < 0)
            {
                g_nDromeTeamHome = m_nTeamCount - 1;

                if (g_nDromeTeamHome == g_nDromeTeamAway)
                {
                    g_nDromeTeamHome--;
                }
            }

            g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), 0);

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[g_nDromeTeamHome];
            }

            nRemaining--;
        }
    }
    else
    {
        g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), nStep);

        if (g_nDromeTeamHome == g_nDromeTeamAway)
        {
            g_nDromeTeamHome++;
        }

        if (g_nDromeTeamHome >= m_nTeamCount)
        {
            g_nDromeTeamHome = 0;

            if (g_nDromeTeamAway == 0)
            {
                g_nDromeTeamHome = 1;
            }
        }

        g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), nStep);

        nLocked = 0;
        if (m_nLockedCount != 0)
        {
            nLocked = m_pLockedFlags[g_nDromeTeamHome];
        }

        nRemaining--;

        while (nLocked != 0 && nRemaining != 0)
        {
            g_nDromeTeamHome++;

            g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), nStep);

            if (g_nDromeTeamHome == g_nDromeTeamAway)
            {
                g_nDromeTeamHome++;
            }

            if (g_nDromeTeamHome >= m_nTeamCount)
            {
                g_nDromeTeamHome = 0;

                if (g_nDromeTeamAway == 0)
                {
                    g_nDromeTeamHome = 1;
                }
            }

            g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(g_nDromeTeamHome), nStep);

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[g_nDromeTeamHome];
            }

            nRemaining--;
        }
    }
}

// FUNCTION: HYPERX 0x00445170
void HeadToHeadSetupUIScreen::StepAwaySelection(int nStep)
{
    int nLocked    = 0;
    int nRemaining = m_nTeamCount;

    // The first test reads the array the previous step left behind.
    if (m_nLockedCount != 0)
    {
        nLocked = m_pLockedFlags[g_nDromeTeamAway];
    }

    BuildLockedFlagsArray();

    if (nStep == 0)
    {
        while (nLocked != 0 && nRemaining != 0)
        {
            g_nDromeTeamAway--;

            g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                                   ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 0);

            if (g_nDromeTeamAway == g_nDromeTeamHome)
            {
                g_nDromeTeamAway--;
            }

            if (g_nDromeTeamAway < 0)
            {
                g_nDromeTeamAway = m_nTeamCount - 1;

                if (g_nDromeTeamAway == g_nDromeTeamHome)
                {
                    g_nDromeTeamAway--;
                }
            }

            g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                                   ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 0);

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[g_nDromeTeamAway];
            }

            nRemaining--;
        }
    }
    else
    {
        while (nLocked != 0 && nRemaining != 0)
        {
            g_nDromeTeamAway++;

            g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                                   ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 1);

            if (g_nDromeTeamAway == g_nDromeTeamHome)
            {
                g_nDromeTeamAway++;
            }

            if (g_nDromeTeamAway >= m_nTeamCount)
            {
                g_nDromeTeamAway = 0;

                if (g_nDromeTeamHome == 0)
                {
                    g_nDromeTeamAway = 1;
                }
            }

            g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                                   ->WrapToNextValidTeam(static_cast<short>(g_nDromeTeamAway), 1);

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[g_nDromeTeamAway];
            }

            nRemaining--;
        }
    }
}

// Each step costs one of a fixed budget, so a run of locked arenas cannot spin.
// FUNCTION: HYPERX 0x004452f0
void HeadToHeadSetupUIScreen::StepArenaSelection(int nForward)
{
    int nLocked   = 0;
    int nAttempts = m_nTeamCount;

    BuildLockedFlagsArray();

    if (m_nLockedCount != 0)
    {
        nLocked = m_pLockedFlags[g_nDromeArena];
    }

    if (nForward == 0)
    {
        for (; nLocked != 0 && nAttempts != 0; nAttempts--)
        {
            g_nDromeArena--;

            if (g_nDromeArena < 0)
            {
                g_nDromeArena = m_nTeamCount - 1;
            }

            g_nDromeArena = g_pGameEngine->m_pSaveObject
                                ->WrapToNextValidTeam(static_cast<short>(g_nDromeArena), 0);

            nLocked = 0;

            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[g_nDromeArena];
            }
        }
    }
    else
    {
        for (; nLocked != 0 && nAttempts != 0; nAttempts--)
        {
            g_nDromeArena++;

            if (g_nDromeArena >= m_nTeamCount)
            {
                g_nDromeArena = 0;
            }

            g_nDromeArena = g_pGameEngine->m_pSaveObject
                                ->WrapToNextValidTeam(static_cast<short>(g_nDromeArena), 1);

            nLocked = 0;

            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[g_nDromeArena];
            }
        }
    }
}

// FUNCTION: HYPERX 0x004453e0
void HeadToHeadSetupUIScreen::OnOtherButton(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case H2H_PLAYER_PORTRAITS:
            ResetFrameOnLoopDone(nMsg);
            break;

        case H2H_TEAM_NAME:
        case H2H_RATINGS_CARD:
            break;

        case H2H_STARTERS_TAB:
            if (nMsg == kInputLeftUp)
            {
                ShowStartersCard();
            }
            break;

        case H2H_SCRUBS_TAB:
            if (nMsg == kInputLeftUp)
            {
                ShowScrubsCard();
            }
            break;

        case H2H_TEAM_TEXT:
        case H2H_TEAM_LOGO:
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// FUNCTION: HYPERX 0x00445450
void HeadToHeadSetupUIScreen::RefreshSelection()
{
    RefreshFrames();

    switch (m_nSelectionMode)
    {
        default:
            RELEASE_ASSERT(0);

            return;

        case H2H_VISITOR_TEAM_BUTTON:
            LoadTeamPlayerBitmaps(g_nDromeTeamAway);

            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetDirtyFlag(1);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetCurrentFrame(0);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetHighlighted(1);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetDepressed(1);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetHidden(0);

            LoadTeamInfoPanel(g_nDromeTeamAway);

            m_apWidgets[H2H_TEAM_NAME]->SetDirtyFlag(1);
            m_apWidgets[H2H_TEAM_NAME]->SetHidden(0);
            m_apWidgets[H2H_RATINGS_CARD]->SetDirtyFlag(1);
            m_apWidgets[H2H_RATINGS_CARD]->SetHidden(0);
            m_apWidgets[H2H_STARTERS_TAB]->SetDirtyFlag(1);
            m_apWidgets[H2H_STARTERS_TAB]->SetHidden(0);
            m_apWidgets[H2H_STARTERS_TAB]->SetDepressed(1);
            m_apWidgets[H2H_SCRUBS_TAB]->SetDirtyFlag(1);
            m_apWidgets[H2H_SCRUBS_TAB]->SetHidden(0);
            m_apWidgets[H2H_SCRUBS_TAB]->SetDepressed(0);
            m_apWidgets[H2H_TEAM_TEXT]->SetDirtyFlag(1);
            m_apWidgets[H2H_TEAM_TEXT]->SetHidden(0);
            m_apWidgets[H2H_TEAM_LOGO]->SetDirtyFlag(1);
            m_apWidgets[H2H_TEAM_LOGO]->SetHidden(0);

            return;

        case H2H_ARENA_BUTTON:
            LoadTeamPlayerBitmaps(100);

            m_apWidgets[H2H_TEAM_NAME]->SetHidden(1);
            m_apWidgets[H2H_RATINGS_CARD]->SetHidden(1);
            m_apWidgets[H2H_STARTERS_TAB]->SetHidden(1);
            m_apWidgets[H2H_SCRUBS_TAB]->SetHidden(1);
            m_apWidgets[H2H_TEAM_TEXT]->SetHidden(1);
            m_apWidgets[H2H_TEAM_LOGO]->SetHidden(1);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetHidden(1);

            return;

        case H2H_HOME_TEAM_BUTTON:
            LoadTeamPlayerBitmaps(g_nDromeTeamHome);

            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetDirtyFlag(1);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetCurrentFrame(0);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetHighlighted(1);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetDepressed(1);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->SetHidden(0);

            LoadTeamInfoPanel(g_nDromeTeamHome);

            m_apWidgets[H2H_TEAM_NAME]->SetDirtyFlag(1);
            m_apWidgets[H2H_TEAM_NAME]->SetHidden(0);
            m_apWidgets[H2H_RATINGS_CARD]->SetDirtyFlag(1);
            m_apWidgets[H2H_RATINGS_CARD]->SetHidden(0);
            m_apWidgets[H2H_STARTERS_TAB]->SetDirtyFlag(1);
            m_apWidgets[H2H_STARTERS_TAB]->SetHidden(0);
            m_apWidgets[H2H_STARTERS_TAB]->SetDepressed(1);
            m_apWidgets[H2H_SCRUBS_TAB]->SetDirtyFlag(1);
            m_apWidgets[H2H_SCRUBS_TAB]->SetHidden(0);
            m_apWidgets[H2H_SCRUBS_TAB]->SetDepressed(0);
            m_apWidgets[H2H_TEAM_TEXT]->SetDirtyFlag(1);
            m_apWidgets[H2H_TEAM_TEXT]->SetHidden(0);
            m_apWidgets[H2H_TEAM_LOGO]->SetDirtyFlag(1);
            m_apWidgets[H2H_TEAM_LOGO]->SetHidden(0);

            return;
    }
}

// Ten players a team, each appended twice. Case 100 is the empty slot.
// FUNCTION: HYPERX 0x00445620
void HeadToHeadSetupUIScreen::LoadTeamPlayerBitmaps(int nTeam)
{
    GDI::Bitmap* hBitmap;

    m_apWidgets[H2H_PLAYER_PORTRAITS]->ClearFrames();

    switch (nTeam)
    {
        case 0:
            // STRING: HYPERX 0x004c2138
            hBitmap = GDI::LoadBitmapHandle("Chipl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2628
            hBitmap = GDI::LoadBitmapHandle("Chipl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2620
            hBitmap = GDI::LoadBitmapHandle("Chipl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2618
            hBitmap = GDI::LoadBitmapHandle("Chipl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2610
            hBitmap = GDI::LoadBitmapHandle("Chipl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2608
            hBitmap = GDI::LoadBitmapHandle("Chipl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2600
            hBitmap = GDI::LoadBitmapHandle("Chipl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25f8
            hBitmap = GDI::LoadBitmapHandle("Chipl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25f0
            hBitmap = GDI::LoadBitmapHandle("Chipl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25e8
            hBitmap = GDI::LoadBitmapHandle("Chipl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 1:
            // STRING: HYPERX 0x004c25e0
            hBitmap = GDI::LoadBitmapHandle("Houpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25d8
            hBitmap = GDI::LoadBitmapHandle("Houpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25d0
            hBitmap = GDI::LoadBitmapHandle("Houpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25c8
            hBitmap = GDI::LoadBitmapHandle("Houpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25c0
            hBitmap = GDI::LoadBitmapHandle("Houpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25b8
            hBitmap = GDI::LoadBitmapHandle("Houpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25b0
            hBitmap = GDI::LoadBitmapHandle("Houpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25a8
            hBitmap = GDI::LoadBitmapHandle("Houpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c25a0
            hBitmap = GDI::LoadBitmapHandle("Houpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2598
            hBitmap = GDI::LoadBitmapHandle("Houpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 2:
            // STRING: HYPERX 0x004c2590
            hBitmap = GDI::LoadBitmapHandle("Jampl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2588
            hBitmap = GDI::LoadBitmapHandle("Jampl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2580
            hBitmap = GDI::LoadBitmapHandle("Jampl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2578
            hBitmap = GDI::LoadBitmapHandle("Jampl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2570
            hBitmap = GDI::LoadBitmapHandle("Jampl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2568
            hBitmap = GDI::LoadBitmapHandle("Jampl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2560
            hBitmap = GDI::LoadBitmapHandle("Jampl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2558
            hBitmap = GDI::LoadBitmapHandle("Jampl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2550
            hBitmap = GDI::LoadBitmapHandle("Jampl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2548
            hBitmap = GDI::LoadBitmapHandle("Jampl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 4:
            // STRING: HYPERX 0x004c2540
            hBitmap = GDI::LoadBitmapHandle("Lndpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2538
            hBitmap = GDI::LoadBitmapHandle("Lndpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2530
            hBitmap = GDI::LoadBitmapHandle("Lndpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2528
            hBitmap = GDI::LoadBitmapHandle("Lndpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2520
            hBitmap = GDI::LoadBitmapHandle("Lndpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2518
            hBitmap = GDI::LoadBitmapHandle("Lndpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2510
            hBitmap = GDI::LoadBitmapHandle("Lndpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2508
            hBitmap = GDI::LoadBitmapHandle("Lndpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2500
            hBitmap = GDI::LoadBitmapHandle("Lndpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24f8
            hBitmap = GDI::LoadBitmapHandle("Lndpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 5:
            // STRING: HYPERX 0x004c24f0
            hBitmap = GDI::LoadBitmapHandle("Lospl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24e8
            hBitmap = GDI::LoadBitmapHandle("Lospl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24e0
            hBitmap = GDI::LoadBitmapHandle("Lospl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24d8
            hBitmap = GDI::LoadBitmapHandle("Lospl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24d0
            hBitmap = GDI::LoadBitmapHandle("Lospl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24c8
            hBitmap = GDI::LoadBitmapHandle("Lospl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24c0
            hBitmap = GDI::LoadBitmapHandle("Lospl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24b8
            hBitmap = GDI::LoadBitmapHandle("Lospl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24b0
            hBitmap = GDI::LoadBitmapHandle("Lospl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c24a8
            hBitmap = GDI::LoadBitmapHandle("Lospl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 6:
            // STRING: HYPERX 0x004c24a0
            hBitmap = GDI::LoadBitmapHandle("Mospl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2498
            hBitmap = GDI::LoadBitmapHandle("Mospl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2490
            hBitmap = GDI::LoadBitmapHandle("Mospl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2488
            hBitmap = GDI::LoadBitmapHandle("Mospl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2480
            hBitmap = GDI::LoadBitmapHandle("Mospl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2478
            hBitmap = GDI::LoadBitmapHandle("Mospl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2470
            hBitmap = GDI::LoadBitmapHandle("Mospl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2468
            hBitmap = GDI::LoadBitmapHandle("Mospl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2460
            hBitmap = GDI::LoadBitmapHandle("Mospl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2458
            hBitmap = GDI::LoadBitmapHandle("Mospl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 7:
            // STRING: HYPERX 0x004c2450
            hBitmap = GDI::LoadBitmapHandle("Munpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2448
            hBitmap = GDI::LoadBitmapHandle("Munpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2440
            hBitmap = GDI::LoadBitmapHandle("Munpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2438
            hBitmap = GDI::LoadBitmapHandle("Munpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2430
            hBitmap = GDI::LoadBitmapHandle("Munpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2428
            hBitmap = GDI::LoadBitmapHandle("Munpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2420
            hBitmap = GDI::LoadBitmapHandle("Munpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2418
            hBitmap = GDI::LoadBitmapHandle("Munpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2410
            hBitmap = GDI::LoadBitmapHandle("Munpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2408
            hBitmap = GDI::LoadBitmapHandle("Munpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 8:
            // STRING: HYPERX 0x004c2400
            hBitmap = GDI::LoadBitmapHandle("Nempl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23f8
            hBitmap = GDI::LoadBitmapHandle("Nempl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23f0
            hBitmap = GDI::LoadBitmapHandle("Nempl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23e8
            hBitmap = GDI::LoadBitmapHandle("Nempl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23e0
            hBitmap = GDI::LoadBitmapHandle("Nempl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23d8
            hBitmap = GDI::LoadBitmapHandle("Nempl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23d0
            hBitmap = GDI::LoadBitmapHandle("Nempl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23c8
            hBitmap = GDI::LoadBitmapHandle("Nempl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23c0
            hBitmap = GDI::LoadBitmapHandle("Nempl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23b8
            hBitmap = GDI::LoadBitmapHandle("Nempl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 9:
            // STRING: HYPERX 0x004c23b0
            hBitmap = GDI::LoadBitmapHandle("Newpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23a8
            hBitmap = GDI::LoadBitmapHandle("Newpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c23a0
            hBitmap = GDI::LoadBitmapHandle("Newpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2398
            hBitmap = GDI::LoadBitmapHandle("Newpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2390
            hBitmap = GDI::LoadBitmapHandle("Newpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2388
            hBitmap = GDI::LoadBitmapHandle("Newpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2380
            hBitmap = GDI::LoadBitmapHandle("Newpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2378
            hBitmap = GDI::LoadBitmapHandle("Newpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2370
            hBitmap = GDI::LoadBitmapHandle("Newpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2368
            hBitmap = GDI::LoadBitmapHandle("Newpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 10:
            // STRING: HYPERX 0x004c2360
            hBitmap = GDI::LoadBitmapHandle("Seapl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2358
            hBitmap = GDI::LoadBitmapHandle("Seapl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2350
            hBitmap = GDI::LoadBitmapHandle("Seapl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2348
            hBitmap = GDI::LoadBitmapHandle("Seapl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2340
            hBitmap = GDI::LoadBitmapHandle("Seapl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2338
            hBitmap = GDI::LoadBitmapHandle("Seapl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2330
            hBitmap = GDI::LoadBitmapHandle("Seapl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2328
            hBitmap = GDI::LoadBitmapHandle("Seapl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2320
            hBitmap = GDI::LoadBitmapHandle("Seapl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2318
            hBitmap = GDI::LoadBitmapHandle("Seapl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 11:
            // STRING: HYPERX 0x004c2310
            hBitmap = GDI::LoadBitmapHandle("Tokpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2308
            hBitmap = GDI::LoadBitmapHandle("Tokpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2300
            hBitmap = GDI::LoadBitmapHandle("Tokpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22f8
            hBitmap = GDI::LoadBitmapHandle("Tokpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22f0
            hBitmap = GDI::LoadBitmapHandle("Tokpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22e8
            hBitmap = GDI::LoadBitmapHandle("Tokpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22e0
            hBitmap = GDI::LoadBitmapHandle("Tokpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22d8
            hBitmap = GDI::LoadBitmapHandle("Tokpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22d0
            hBitmap = GDI::LoadBitmapHandle("Tokpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22c8
            hBitmap = GDI::LoadBitmapHandle("Tokpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 12:
            // STRING: HYPERX 0x004c22c0
            hBitmap = GDI::LoadBitmapHandle("Torpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22b8
            hBitmap = GDI::LoadBitmapHandle("Torpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22b0
            hBitmap = GDI::LoadBitmapHandle("Torpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22a8
            hBitmap = GDI::LoadBitmapHandle("Torpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c22a0
            hBitmap = GDI::LoadBitmapHandle("Torpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2298
            hBitmap = GDI::LoadBitmapHandle("Torpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2290
            hBitmap = GDI::LoadBitmapHandle("Torpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2288
            hBitmap = GDI::LoadBitmapHandle("Torpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2280
            hBitmap = GDI::LoadBitmapHandle("Torpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2278
            hBitmap = GDI::LoadBitmapHandle("Torpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 3:
            // STRING: HYPERX 0x004c2270
            hBitmap = GDI::LoadBitmapHandle("Vegpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2268
            hBitmap = GDI::LoadBitmapHandle("Vegpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2260
            hBitmap = GDI::LoadBitmapHandle("Vegpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2258
            hBitmap = GDI::LoadBitmapHandle("Vegpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2250
            hBitmap = GDI::LoadBitmapHandle("Vegpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2248
            hBitmap = GDI::LoadBitmapHandle("Vegpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2240
            hBitmap = GDI::LoadBitmapHandle("Vegpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2238
            hBitmap = GDI::LoadBitmapHandle("Vegpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2230
            hBitmap = GDI::LoadBitmapHandle("Vegpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2228
            hBitmap = GDI::LoadBitmapHandle("Vegpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 13:
            // STRING: HYPERX 0x004c2220
            hBitmap = GDI::LoadBitmapHandle("Warpl01");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2218
            hBitmap = GDI::LoadBitmapHandle("Warpl02");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2210
            hBitmap = GDI::LoadBitmapHandle("Warpl03");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2208
            hBitmap = GDI::LoadBitmapHandle("Warpl04");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c2200
            hBitmap = GDI::LoadBitmapHandle("Warpl05");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c21f8
            hBitmap = GDI::LoadBitmapHandle("Warpl06");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c21f0
            hBitmap = GDI::LoadBitmapHandle("Warpl07");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c21e8
            hBitmap = GDI::LoadBitmapHandle("Warpl08");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c21e0
            hBitmap = GDI::LoadBitmapHandle("Warpl09");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            // STRING: HYPERX 0x004c21d8
            hBitmap = GDI::LoadBitmapHandle("Warpl10");
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->AppendFrame(hBitmap);
            m_apWidgets[H2H_PLAYER_PORTRAITS]->InitAfterLoad();
            break;

        case 100:
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// The guard tests the address of a member, so it can never fail.
// FUNCTION: HYPERX 0x004471a0
void HeadToHeadSetupUIScreen::LoadTeamInfoPanel(int nTeam)
{
    m_apWidgets[H2H_TEAM_NAME]->ClearFrames();
    m_apWidgets[H2H_RATINGS_CARD]->ClearFrames();
    m_apWidgets[H2H_STARTERS_TAB]->ClearFrames();
    m_apWidgets[H2H_SCRUBS_TAB]->ClearFrames();
    m_apWidgets[H2H_TEAM_TEXT]->ClearFrames();
    m_apWidgets[H2H_TEAM_LOGO]->ClearFrames();

    switch (nTeam)
    {
        case 0:
            // STRING: HYPERX 0x004c27a0
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("chiplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2128
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("chi_cb"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            // STRING: HYPERX 0x004c2130
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            // STRING: HYPERX 0x004c2798
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            // STRING: HYPERX 0x004c2790
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2788
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            // STRING: HYPERX 0x004c2780
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            // STRING: HYPERX 0x004c2778
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2770
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxchi"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2768
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("chiplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 1:
            // STRING: HYPERX 0x004c2760
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("houplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2118
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("hou_ab"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2758
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxhou"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2750
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("houplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 2:
            // STRING: HYPERX 0x004c2748
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("jamplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2108
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("jam_es"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2740
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxjam"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2738
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("jamplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 4:
            // STRING: HYPERX 0x004c2730
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("lndplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c20e8
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("lnd_ls"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2728
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxlnd"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2720
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("lndplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 5:
            // STRING: HYPERX 0x004c2718
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("losplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c20d8
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("los_am"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2710
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxlos"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2708
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("losplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 6:
            // STRING: HYPERX 0x004c2700
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("mosplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c20c8
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("mos_ab"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c26f8
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxmos"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c26f0
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("mosplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 7:
            // STRING: HYPERX 0x004c26e8
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("munplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c20b8
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("mun_dj"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c26e0
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxmun"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c26d8
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("munplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 8:
            // STRING: HYPERX 0x004c26d0
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("nemplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c20a8
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("nem_06"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c26c8
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxnem"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c26c0
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("nemplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 9:
            // STRING: HYPERX 0x004c26b8
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("newplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2098
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("new_jn"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c26b0
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxnew"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c26a8
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("newplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 10:
            // STRING: HYPERX 0x004c26a0
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("seaplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2088
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("sea_rk"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2698
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxsea"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2690
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("seaplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 11:
            // STRING: HYPERX 0x004c2688
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("tokplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2078
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("tok_hs"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2680
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxtok"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2678
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("tokplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 12:
            // STRING: HYPERX 0x004c2670
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("torplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2068
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("tor_jh"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2668
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxtor"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2660
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("torplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 3:
            // STRING: HYPERX 0x004c2658
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("vegplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c20f8
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("veg_rw"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2650
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxveg"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2648
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("vegplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        case 13:
            // STRING: HYPERX 0x004c2640
            m_apWidgets[H2H_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("warplsl"));
            m_apWidgets[H2H_TEAM_NAME]->InitAfterLoad();
            // STRING: HYPERX 0x004c2058
            m_apWidgets[H2H_RATINGS_CARD]->AppendFrame(GDI::LoadBitmapHandle("war_aw"));
            m_apWidgets[H2H_RATINGS_CARD]->InitAfterLoad();
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
            m_apWidgets[H2H_STARTERS_TAB]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
            m_apWidgets[H2H_STARTERS_TAB]->InitAfterLoad();
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
            m_apWidgets[H2H_SCRUBS_TAB]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
            m_apWidgets[H2H_SCRUBS_TAB]->InitAfterLoad();
            // STRING: HYPERX 0x004c2638
            m_apWidgets[H2H_TEAM_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Tstxwar"));
            m_apWidgets[H2H_TEAM_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c2630
            m_apWidgets[H2H_TEAM_LOGO]->AppendFrame(GDI::LoadBitmapHandle("warplay"));
            m_apWidgets[H2H_TEAM_LOGO]->InitAfterLoad();
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// FUNCTION: HYPERX 0x00448650
void HeadToHeadSetupUIScreen::RefreshFrames()
{
    m_apWidgets[H2H_VIEW_PANEL]->ClearFrames();
    m_apWidgets[H2H_VIEW_PANEL]->SetDirtyFlag(1);
    m_apWidgets[H2H_VIEW_PANEL]->SetHidden(0);

    switch (m_nSelectionMode)
    {
        case H2H_ARENA_BUTTON:
            switch (g_nDromeArena)
            {
                case 0:
                    // STRING: HYPERX 0x004c2818
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("Drmpchi"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 1:
                    // STRING: HYPERX 0x004c2810
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpHou"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 2:
                    // STRING: HYPERX 0x004c2808
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpJam"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 4:
                    // STRING: HYPERX 0x004c2800
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpLnd"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 5:
                    // STRING: HYPERX 0x004c27f8
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpLos"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 6:
                    // STRING: HYPERX 0x004c27f0
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpMos"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 7:
                    // STRING: HYPERX 0x004c27e8
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpMun"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 8:
                    // STRING: HYPERX 0x004c27e0
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpNem"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 9:
                    // STRING: HYPERX 0x004c27d8
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpNew"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 10:
                    // STRING: HYPERX 0x004c27d0
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpSea"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 11:
                    // STRING: HYPERX 0x004c27c8
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpTok"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 12:
                    // STRING: HYPERX 0x004c27c0
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpTor"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 3:
                    // STRING: HYPERX 0x004c27b8
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpVeg"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                case 13:
                    // STRING: HYPERX 0x004c27b0
                    m_apWidgets[H2H_VIEW_PANEL]->AppendFrame(GDI::LoadBitmapHandle("DrmpWar"));
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;

                default:
                    RELEASE_ASSERT(0);
                    m_apWidgets[H2H_VIEW_PANEL]->InitAfterLoad();
                    break;
            }

            break;

        case H2H_HOME_TEAM_BUTTON:
        case H2H_VISITOR_TEAM_BUTTON:
        {
            const FramePos size = m_apWidgets[H2H_VIEW_PANEL]->GetSize();

            // STRING: HYPERX 0x004c27a8
            m_apWidgets[H2H_VIEW_PANEL]->AppendFrameWithPos(GDI::LoadBitmapHandle("playbkg"), size.x, size.y, 0, 0, 0, 0);
            break;
        }

        default:
            RELEASE_ASSERT(0);
            break;
    }

    m_apWidgets[H2H_VIEW_PANEL]->SetDirtyFlag(1);
    m_apWidgets[H2H_VIEW_PANEL]->SetHidden(0);
}
