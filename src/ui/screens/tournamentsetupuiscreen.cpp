#include "common/compat.h"
#include "engine/teamsblock.h"
#include "globals.h"
#include "ui/screen.h"
#include "ui/torchbitmaps.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "engine/gameengine.h"
#include "ui/framepos.h"
#include "ui/widget.h"
#include "ui/pausemenu.h"
#include "ui/screens/leaguesetupuiscreen.h"
#include "ui/screens/tournamentsetupuiscreen.h"
#include "ui/loopflipbookwidget.h"
#include "ui/displaywidget.h"
#include "ui/saveobject.h"
#include "graphics/gdi.h"
#include "engine/game.h"
#include "common/stub.h"
#include "common/staticinit.h"

float g_flFifthGameEngine = 0.2f * g_flOneGameEngine;

// FUNCTION: HYPERX 0x00488400
TournamentSetupUIScreen::~TournamentSetupUIScreen()
{
}

// FUNCTION: HYPERX 0x00488430
TournamentSetupUIScreen::TournamentSetupUIScreen()
{
    Widget* pWidget;
    m_nLastWidget = TOURNAMENT_SETUP_TEAM_EMBLEM;

    // STRING: HYPERX 0x004c21d0
    m_hBackground = GDI::LoadBitmapAsBackground("Hviewp", &m_hPalette);

    m_apWidgets[UI_WIDGET_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 4]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 3]->SetHidden(1);
    g_nMatchSetupFlagB = 0;
    g_nMatchSetupFlagA = 0;

    m_nTeamCount      = g_pGameEngine->m_pTeamTable->GetTeamCount();
    m_nSelection      = 1;
    m_nSelectionLimit = 14;

    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);

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
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDepressed(1);
    // STRING: HYPERX 0x004c2138
    m_hBitmap = GDI::LoadBitmapHandle("Chipl01");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    GDI::SafeDeleteBitmap(m_hBitmap);
    pWidget = AppendWidget(new LoopFlipbookWidget(206, 63, m_bitmap.bmWidth, m_bitmap.bmHeight, 1, 1, 19, 0, 19, 19, this, 1, -1));
    for (int i = 0; i < 20; i++)
    {
        m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(0);
    }

    m_apWidgets[TOURNAMENT_SETUP_ROSTER]->SetHidden(1);
    pWidget = AppendWidget(new DisplayWidget(0, 0, 1, 1, this, -1));
    m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->SetHidden(1);
    pWidget = AppendWidget(new DisplayWidget(9, 101, 1, 1, this, -1));
    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->SetHidden(1);
    // STRING: HYPERX 0x004c2130
    m_hBitmap = GDI::LoadBitmapHandle("startr1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, 7, 67, m_bitmap.bmWidth, m_bitmap.bmHeight, 0, 0, 2, 0, 1, 2, this, -1));
    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->SetHidden(1);
    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->SetDepressed(1);
    m_hBitmap = GDI::LoadBitmapHandle("startr1");
    GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
    pWidget = AppendWidget(new ButtonWidget(0, 107, 67, m_bitmap.bmWidth, m_bitmap.bmHeight, 0, 0, 2, 0, 1, 2, this, -1));
    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->SetHidden(1);
    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->SetDepressed(0);
    pWidget = AppendWidget(new DisplayWidget(430, 67, 1, 1, this, -1));
    m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->SetHidden(1);
    pWidget = AppendWidget(new DisplayWidget(580, 9, 1, 1, this, -1));
    m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->SetHidden(1);

    InitAllWidgets();

    g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam((short)g_nDromeTeamHome, 1);

    g_nDromeArena = g_pGameEngine->m_pSaveObject
                        ->WrapToNextValidTeam((short)g_nDromeArena, 1);

    g_nDromeTeamAway = g_pGameEngine->m_pSaveObject
                           ->WrapToNextValidTeam((short)g_nDromeTeamAway, 1);

    RefreshSelection();
}

// FUNCTION: HYPERX 0x00488ab0
void TournamentSetupUIScreen::OnOkayClick()
{
    g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam((short)g_nDromeTeamHome, 1);

    g_pGameEngine->m_pSaveObject->BracketIterReset();

    const int nTeam = g_pGameEngine->m_pSaveObject->BracketIterPeek();

    g_nDromeTeamAway = nTeam;
    g_nDromeArena    = nTeam;

    Game::SetEngineState(g_nSetupReturnState);
}

// FUNCTION: HYPERX 0x00488b10
void TournamentSetupUIScreen::ShowStartersCard()
{
    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->ClearFrames();

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle(g_aTeamCardNames[g_nDromeTeamHome].pszStartersCard));

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->SetDirtyFlag(1);

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->SetHidden(0);

    InitFrame();
}

// FUNCTION: HYPERX 0x00488b70
void TournamentSetupUIScreen::ShowScrubsCard()
{
    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->ClearFrames();

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(
        GDI::LoadBitmapHandle(g_aTeamCardNames[g_nDromeTeamHome].pszScrubsCard));

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->SetDirtyFlag(1);

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->SetHidden(0);

    InitFrame();
}

// Forward tries once per team, backward three times. The guard's final count is never read.
// FUNCTION: HYPERX 0x00488bf0
void TournamentSetupUIScreen::OnRightArrowClick()
{
    int nGuard = m_nTeamCount;

    BuildLockedFlagsArray();

    int nHome = g_nDromeTeamHome;

    int nLocked = 1;

    while (nLocked != 0 && nGuard != 0)
    {
        nHome++;

        g_nDromeTeamHome = nHome;

        if (nHome >= m_nTeamCount)
        {
            nHome            = 0;
            g_nDromeTeamHome = nHome;
        }

        g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam((short)nHome, 1);

        nLocked = 0;

        if (m_nLockedCount != 0)
        {
            nLocked = m_pLockedFlags[g_nDromeTeamHome];
        }

        nHome = g_nDromeTeamHome;

        nGuard--;
    }

    RefreshSelection();
}

// FUNCTION: HYPERX 0x00488c60
void TournamentSetupUIScreen::OnLeftArrowClick()
{
    int nGuard = m_nTeamCount * 3;

    BuildLockedFlagsArray();

    int nHome = g_nDromeTeamHome;

    int nLocked = 1;

    while (nLocked != 0 && nGuard != 0)
    {
        nHome--;

        g_nDromeTeamHome = nHome;

        if (nHome < 0)
        {
            nHome            = m_nTeamCount - 1;
            g_nDromeTeamHome = nHome;
        }

        g_pGameEngine->m_pSaveObject->ValidateSetHomeTeam((short)nHome, 0);

        nLocked = 0;

        if (m_nLockedCount != 0)
        {
            nLocked = m_pLockedFlags[g_nDromeTeamHome];
        }

        nHome = g_nDromeTeamHome;

        nGuard--;
    }

    RefreshSelection();
}

// The displays return silently; an identifier outside the page calls the stub.
// FUNCTION: HYPERX 0x00488cd0
void TournamentSetupUIScreen::OnOtherButton(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case TOURNAMENT_SETUP_ROSTER:
            ResetFrameOnLoopDone(nMsg);
            break;

        case TOURNAMENT_SETUP_TEAM_NAME:
        case TOURNAMENT_SETUP_LINEUP_CARD:
            break;

        case TOURNAMENT_SETUP_STARTERS:
            if (nMsg == WM_LBUTTONUP)
            {
                ShowStartersCard();
            }
            break;

        case TOURNAMENT_SETUP_SCRUBS:
            if (nMsg == WM_LBUTTONUP)
            {
                ShowScrubsCard();
            }
            break;

        case TOURNAMENT_SETUP_TEAM_RATING:
        case TOURNAMENT_SETUP_TEAM_EMBLEM:
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// FUNCTION: HYPERX 0x00488d40
void TournamentSetupUIScreen::RefreshSelection()
{
    RefreshFrames();

    LoadTeamPlayerBitmaps(g_nDromeTeamHome);

    m_apWidgets[TOURNAMENT_SETUP_ROSTER]->SetDirtyFlag(1);
    m_apWidgets[TOURNAMENT_SETUP_ROSTER]->SetCurrentFrame(0);
    m_apWidgets[TOURNAMENT_SETUP_ROSTER]->SetHighlighted(1);
    m_apWidgets[TOURNAMENT_SETUP_ROSTER]->SetDepressed(1);
    m_apWidgets[TOURNAMENT_SETUP_ROSTER]->SetHidden(0);

    LoadTeamInfoPanel(g_nDromeTeamHome);

    m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->SetDirtyFlag(1);
    m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->SetHidden(0);

    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->SetDirtyFlag(1);
    m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->SetHidden(0);

    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->SetDirtyFlag(1);
    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->SetHidden(0);
    m_apWidgets[TOURNAMENT_SETUP_STARTERS]->SetDepressed(1);

    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->SetDirtyFlag(1);
    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->SetHidden(0);
    m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->SetDepressed(0);

    m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->SetDirtyFlag(1);
    m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->SetHidden(0);

    m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->SetDirtyFlag(1);
    m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->SetHidden(0);
}

// The same function as HeadToHeadSetupUIScreen::LoadTeamPlayerBitmaps.
// FUNCTION: HYPERX 0x00488e50
void TournamentSetupUIScreen::LoadTeamPlayerBitmaps(int nTeam)
{
    HBITMAP hBitmap;

    if (m_apWidgets != 0)
    {
        m_apWidgets[TOURNAMENT_SETUP_ROSTER]->ClearFrames();

        switch (nTeam)
        {
            case 0:
                // STRING: HYPERX 0x004c2138
                hBitmap = GDI::LoadBitmapHandle("Chipl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2628
                hBitmap = GDI::LoadBitmapHandle("Chipl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2620
                hBitmap = GDI::LoadBitmapHandle("Chipl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2618
                hBitmap = GDI::LoadBitmapHandle("Chipl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2610
                hBitmap = GDI::LoadBitmapHandle("Chipl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2608
                hBitmap = GDI::LoadBitmapHandle("Chipl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2600
                hBitmap = GDI::LoadBitmapHandle("Chipl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25f8
                hBitmap = GDI::LoadBitmapHandle("Chipl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25f0
                hBitmap = GDI::LoadBitmapHandle("Chipl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25e8
                hBitmap = GDI::LoadBitmapHandle("Chipl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 1:
                // STRING: HYPERX 0x004c25e0
                hBitmap = GDI::LoadBitmapHandle("Houpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25d8
                hBitmap = GDI::LoadBitmapHandle("Houpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25d0
                hBitmap = GDI::LoadBitmapHandle("Houpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25c8
                hBitmap = GDI::LoadBitmapHandle("Houpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25c0
                hBitmap = GDI::LoadBitmapHandle("Houpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25b8
                hBitmap = GDI::LoadBitmapHandle("Houpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25b0
                hBitmap = GDI::LoadBitmapHandle("Houpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25a8
                hBitmap = GDI::LoadBitmapHandle("Houpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c25a0
                hBitmap = GDI::LoadBitmapHandle("Houpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2598
                hBitmap = GDI::LoadBitmapHandle("Houpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 2:
                // STRING: HYPERX 0x004c2590
                hBitmap = GDI::LoadBitmapHandle("Jampl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2588
                hBitmap = GDI::LoadBitmapHandle("Jampl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2580
                hBitmap = GDI::LoadBitmapHandle("Jampl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2578
                hBitmap = GDI::LoadBitmapHandle("Jampl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2570
                hBitmap = GDI::LoadBitmapHandle("Jampl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2568
                hBitmap = GDI::LoadBitmapHandle("Jampl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2560
                hBitmap = GDI::LoadBitmapHandle("Jampl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2558
                hBitmap = GDI::LoadBitmapHandle("Jampl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2550
                hBitmap = GDI::LoadBitmapHandle("Jampl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2548
                hBitmap = GDI::LoadBitmapHandle("Jampl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 4:
                // STRING: HYPERX 0x004c2540
                hBitmap = GDI::LoadBitmapHandle("Lndpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2538
                hBitmap = GDI::LoadBitmapHandle("Lndpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2530
                hBitmap = GDI::LoadBitmapHandle("Lndpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2528
                hBitmap = GDI::LoadBitmapHandle("Lndpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2520
                hBitmap = GDI::LoadBitmapHandle("Lndpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2518
                hBitmap = GDI::LoadBitmapHandle("Lndpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2510
                hBitmap = GDI::LoadBitmapHandle("Lndpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2508
                hBitmap = GDI::LoadBitmapHandle("Lndpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2500
                hBitmap = GDI::LoadBitmapHandle("Lndpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24f8
                hBitmap = GDI::LoadBitmapHandle("Lndpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 5:
                // STRING: HYPERX 0x004c24f0
                hBitmap = GDI::LoadBitmapHandle("Lospl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24e8
                hBitmap = GDI::LoadBitmapHandle("Lospl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24e0
                hBitmap = GDI::LoadBitmapHandle("Lospl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24d8
                hBitmap = GDI::LoadBitmapHandle("Lospl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24d0
                hBitmap = GDI::LoadBitmapHandle("Lospl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24c8
                hBitmap = GDI::LoadBitmapHandle("Lospl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24c0
                hBitmap = GDI::LoadBitmapHandle("Lospl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24b8
                hBitmap = GDI::LoadBitmapHandle("Lospl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24b0
                hBitmap = GDI::LoadBitmapHandle("Lospl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c24a8
                hBitmap = GDI::LoadBitmapHandle("Lospl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 6:
                // STRING: HYPERX 0x004c24a0
                hBitmap = GDI::LoadBitmapHandle("Mospl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2498
                hBitmap = GDI::LoadBitmapHandle("Mospl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2490
                hBitmap = GDI::LoadBitmapHandle("Mospl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2488
                hBitmap = GDI::LoadBitmapHandle("Mospl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2480
                hBitmap = GDI::LoadBitmapHandle("Mospl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2478
                hBitmap = GDI::LoadBitmapHandle("Mospl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2470
                hBitmap = GDI::LoadBitmapHandle("Mospl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2468
                hBitmap = GDI::LoadBitmapHandle("Mospl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2460
                hBitmap = GDI::LoadBitmapHandle("Mospl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2458
                hBitmap = GDI::LoadBitmapHandle("Mospl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 7:
                // STRING: HYPERX 0x004c2450
                hBitmap = GDI::LoadBitmapHandle("Munpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2448
                hBitmap = GDI::LoadBitmapHandle("Munpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2440
                hBitmap = GDI::LoadBitmapHandle("Munpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2438
                hBitmap = GDI::LoadBitmapHandle("Munpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2430
                hBitmap = GDI::LoadBitmapHandle("Munpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2428
                hBitmap = GDI::LoadBitmapHandle("Munpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2420
                hBitmap = GDI::LoadBitmapHandle("Munpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2418
                hBitmap = GDI::LoadBitmapHandle("Munpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2410
                hBitmap = GDI::LoadBitmapHandle("Munpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2408
                hBitmap = GDI::LoadBitmapHandle("Munpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 8:
                // STRING: HYPERX 0x004c2400
                hBitmap = GDI::LoadBitmapHandle("Nempl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23f8
                hBitmap = GDI::LoadBitmapHandle("Nempl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23f0
                hBitmap = GDI::LoadBitmapHandle("Nempl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23e8
                hBitmap = GDI::LoadBitmapHandle("Nempl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23e0
                hBitmap = GDI::LoadBitmapHandle("Nempl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23d8
                hBitmap = GDI::LoadBitmapHandle("Nempl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23d0
                hBitmap = GDI::LoadBitmapHandle("Nempl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23c8
                hBitmap = GDI::LoadBitmapHandle("Nempl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23c0
                hBitmap = GDI::LoadBitmapHandle("Nempl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23b8
                hBitmap = GDI::LoadBitmapHandle("Nempl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 9:
                // STRING: HYPERX 0x004c23b0
                hBitmap = GDI::LoadBitmapHandle("Newpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23a8
                hBitmap = GDI::LoadBitmapHandle("Newpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c23a0
                hBitmap = GDI::LoadBitmapHandle("Newpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2398
                hBitmap = GDI::LoadBitmapHandle("Newpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2390
                hBitmap = GDI::LoadBitmapHandle("Newpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2388
                hBitmap = GDI::LoadBitmapHandle("Newpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2380
                hBitmap = GDI::LoadBitmapHandle("Newpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2378
                hBitmap = GDI::LoadBitmapHandle("Newpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2370
                hBitmap = GDI::LoadBitmapHandle("Newpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2368
                hBitmap = GDI::LoadBitmapHandle("Newpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 10:
                // STRING: HYPERX 0x004c2360
                hBitmap = GDI::LoadBitmapHandle("Seapl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2358
                hBitmap = GDI::LoadBitmapHandle("Seapl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2350
                hBitmap = GDI::LoadBitmapHandle("Seapl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2348
                hBitmap = GDI::LoadBitmapHandle("Seapl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2340
                hBitmap = GDI::LoadBitmapHandle("Seapl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2338
                hBitmap = GDI::LoadBitmapHandle("Seapl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2330
                hBitmap = GDI::LoadBitmapHandle("Seapl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2328
                hBitmap = GDI::LoadBitmapHandle("Seapl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2320
                hBitmap = GDI::LoadBitmapHandle("Seapl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2318
                hBitmap = GDI::LoadBitmapHandle("Seapl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 11:
                // STRING: HYPERX 0x004c2310
                hBitmap = GDI::LoadBitmapHandle("Tokpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2308
                hBitmap = GDI::LoadBitmapHandle("Tokpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2300
                hBitmap = GDI::LoadBitmapHandle("Tokpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22f8
                hBitmap = GDI::LoadBitmapHandle("Tokpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22f0
                hBitmap = GDI::LoadBitmapHandle("Tokpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22e8
                hBitmap = GDI::LoadBitmapHandle("Tokpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22e0
                hBitmap = GDI::LoadBitmapHandle("Tokpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22d8
                hBitmap = GDI::LoadBitmapHandle("Tokpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22d0
                hBitmap = GDI::LoadBitmapHandle("Tokpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22c8
                hBitmap = GDI::LoadBitmapHandle("Tokpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 12:
                // STRING: HYPERX 0x004c22c0
                hBitmap = GDI::LoadBitmapHandle("Torpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22b8
                hBitmap = GDI::LoadBitmapHandle("Torpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22b0
                hBitmap = GDI::LoadBitmapHandle("Torpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22a8
                hBitmap = GDI::LoadBitmapHandle("Torpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c22a0
                hBitmap = GDI::LoadBitmapHandle("Torpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2298
                hBitmap = GDI::LoadBitmapHandle("Torpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2290
                hBitmap = GDI::LoadBitmapHandle("Torpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2288
                hBitmap = GDI::LoadBitmapHandle("Torpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2280
                hBitmap = GDI::LoadBitmapHandle("Torpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2278
                hBitmap = GDI::LoadBitmapHandle("Torpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 3:
                // STRING: HYPERX 0x004c2270
                hBitmap = GDI::LoadBitmapHandle("Vegpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2268
                hBitmap = GDI::LoadBitmapHandle("Vegpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2260
                hBitmap = GDI::LoadBitmapHandle("Vegpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2258
                hBitmap = GDI::LoadBitmapHandle("Vegpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2250
                hBitmap = GDI::LoadBitmapHandle("Vegpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2248
                hBitmap = GDI::LoadBitmapHandle("Vegpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2240
                hBitmap = GDI::LoadBitmapHandle("Vegpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2238
                hBitmap = GDI::LoadBitmapHandle("Vegpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2230
                hBitmap = GDI::LoadBitmapHandle("Vegpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2228
                hBitmap = GDI::LoadBitmapHandle("Vegpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 13:
                // STRING: HYPERX 0x004c2220
                hBitmap = GDI::LoadBitmapHandle("Warpl01");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2218
                hBitmap = GDI::LoadBitmapHandle("Warpl02");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2210
                hBitmap = GDI::LoadBitmapHandle("Warpl03");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2208
                hBitmap = GDI::LoadBitmapHandle("Warpl04");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c2200
                hBitmap = GDI::LoadBitmapHandle("Warpl05");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c21f8
                hBitmap = GDI::LoadBitmapHandle("Warpl06");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c21f0
                hBitmap = GDI::LoadBitmapHandle("Warpl07");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c21e8
                hBitmap = GDI::LoadBitmapHandle("Warpl08");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c21e0
                hBitmap = GDI::LoadBitmapHandle("Warpl09");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                // STRING: HYPERX 0x004c21d8
                hBitmap = GDI::LoadBitmapHandle("Warpl10");
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->AppendFrame(hBitmap);
                m_apWidgets[TOURNAMENT_SETUP_ROSTER]->InitAfterLoad();
                break;

            case 100:
                break;

            default:
                RELEASE_ASSERT(0);
                break;
        }
    }
}

// The same function as HeadToHeadSetupUIScreen::LoadTeamInfoPanel.
// FUNCTION: HYPERX 0x0048a9d0
void TournamentSetupUIScreen::LoadTeamInfoPanel(int nTeam)
{
    if (m_apWidgets != 0)
    {
        m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->ClearFrames();
        m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->ClearFrames();
        m_apWidgets[TOURNAMENT_SETUP_STARTERS]->ClearFrames();
        m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->ClearFrames();
        m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->ClearFrames();
        m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->ClearFrames();

        switch (nTeam)
        {
            case 0:
                // STRING: HYPERX 0x004c27a0
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("chiplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2128
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("chi_cb"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                // STRING: HYPERX 0x004c2130
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                // STRING: HYPERX 0x004c2798
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                // STRING: HYPERX 0x004c2790
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2788
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                // STRING: HYPERX 0x004c2780
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                // STRING: HYPERX 0x004c2778
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2770
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxchi"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2768
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("chiplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 1:
                // STRING: HYPERX 0x004c2760
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("houplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2118
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("hou_ab"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2758
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxhou"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2750
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("houplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 2:
                // STRING: HYPERX 0x004c2748
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("jamplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2108
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("jam_es"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2740
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxjam"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2738
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("jamplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 4:
                // STRING: HYPERX 0x004c2730
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("lndplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c20e8
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("lnd_ls"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2728
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxlnd"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2720
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("lndplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 5:
                // STRING: HYPERX 0x004c2718
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("losplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c20d8
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("los_am"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2710
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxlos"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2708
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("losplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 6:
                // STRING: HYPERX 0x004c2700
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("mosplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c20c8
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("mos_ab"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c26f8
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxmos"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c26f0
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("mosplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 7:
                // STRING: HYPERX 0x004c26e8
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("munplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c20b8
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("mun_dj"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c26e0
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxmun"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c26d8
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("munplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 8:
                // STRING: HYPERX 0x004c26d0
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("nemplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c20a8
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("nem_06"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c26c8
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxnem"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c26c0
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("nemplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 9:
                // STRING: HYPERX 0x004c26b8
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("newplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2098
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("new_jn"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c26b0
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxnew"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c26a8
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("newplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 10:
                // STRING: HYPERX 0x004c26a0
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("seaplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2088
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("sea_rk"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2698
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxsea"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2690
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("seaplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 11:
                // STRING: HYPERX 0x004c2688
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("tokplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2078
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("tok_hs"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2680
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxtok"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2678
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("tokplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 12:
                // STRING: HYPERX 0x004c2670
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("torplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2068
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("tor_jh"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2668
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxtor"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2660
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("torplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 3:
                // STRING: HYPERX 0x004c2658
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("vegplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c20f8
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("veg_rw"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2650
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxveg"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2648
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("vegplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            case 13:
                // STRING: HYPERX 0x004c2640
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->AppendFrame(GDI::LoadBitmapHandle("warplsl"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_NAME]->InitAfterLoad();
                // STRING: HYPERX 0x004c2058
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->AppendFrame(GDI::LoadBitmapHandle("war_aw"));
                m_apWidgets[TOURNAMENT_SETUP_LINEUP_CARD]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr1"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr2"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->AppendFrame(GDI::LoadBitmapHandle("startr3"));
                m_apWidgets[TOURNAMENT_SETUP_STARTERS]->InitAfterLoad();
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts1"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts2"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->AppendFrame(GDI::LoadBitmapHandle("starts3"));
                m_apWidgets[TOURNAMENT_SETUP_SCRUBS]->InitAfterLoad();
                // STRING: HYPERX 0x004c2638
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->AppendFrame(GDI::LoadBitmapHandle("Tstxwar"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_RATING]->InitAfterLoad();
                // STRING: HYPERX 0x004c2630
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->AppendFrame(GDI::LoadBitmapHandle("warplay"));
                m_apWidgets[TOURNAMENT_SETUP_TEAM_EMBLEM]->InitAfterLoad();
                break;

            default:
                RELEASE_ASSERT(0);
                break;
        }
    }
}

// FUNCTION: HYPERX 0x0048be80
void TournamentSetupUIScreen::RefreshFrames()
{
    m_apWidgets[UI_WIDGET_PAGE]->ClearFrames();

    m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE]->SetHidden(0);

    const FramePos size = m_apWidgets[UI_WIDGET_PAGE]->GetSize();

    // STRING: HYPERX 0x004c27a8
    m_apWidgets[UI_WIDGET_PAGE]->AppendFrameWithPos(GDI::LoadBitmapHandle("playbkg"), size.x, size.y, 0, 0, 0, 0);

    m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);

    m_apWidgets[UI_WIDGET_PAGE]->SetHidden(0);
}

TeamCardNames g_aTeamCardNames[14] = {
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
