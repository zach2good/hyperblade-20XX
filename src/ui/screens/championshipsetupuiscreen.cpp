#include "common/compat.h"
#include <algorithm>
#include "engine/teamsblock.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "ui/screen.h"
#include "graphics/gdi.h"
#include "ui/torchbitmaps.h"
#include "engine/gameengine.h"
#include "ui/pausemenu.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "ui/screens/leaguesetupuiscreen.h"
#include "ui/buttonwidget.h"
#include "ui/widget.h"
#include "ui/screens/championshipsetupuiscreen.h"
#include "ui/loopflipbookwidget.h"
#include "ui/displaywidget.h"
#include "ui/saveobject.h"
#include <cstdio>
#include "ui/statcolumns.h"
#include "engine/game.h"
#include "common/stub.h"

// Only the stats and tree tab names carry STRING markers here; the rest are claimed elsewhere.

#define CHAMP_INITIAL_LAST_WIDGET CHAMP_WIDGET_ROUND_TORCH
#define CHAMP_GRID_LAST           (CHAMP_WIDGET_STAT_GRID + kChampGridRows * kChampGridColumns - 1)
constexpr auto kChampBracketCount = 12;
constexpr auto kChampGridRows     = 10;
constexpr auto kChampGridColumns  = 5;

// FUNCTION: HYPERX 0x00457710
ChampionshipSetupUIScreen::ChampionshipSetupUIScreen()
{
    Widget* pWidget;
    int     nFlag;
    int     i;

    m_nPrevState   = g_nPrevEngineState;
    m_nLastWidget  = CHAMP_INITIAL_LAST_WIDGET;
    m_nTeamSlot    = 0;
    m_nArena       = 0;
    m_nObstacleSet = 0;
    m_nUnusedF     = 0;

    const int nTeams = g_pGameEngine->m_pTeamTable->GetTeamCount();

    m_nArenaCount       = nTeams;
    m_nUnusedE          = 0xe;
    m_nObstacleSetCount = 0xe;
    m_nTeamSlotCount    = nTeams * 4;
    m_nUnusedG          = 3;
    m_bShowHomeTeam     = 1;

    StepHomeSelection(1);
    StepArenaSelection(1);

    m_apWidgets[TEAMSETUP_WIDGET_ROSTER]->ClearFrames();
    m_apWidgets[TEAMSETUP_WIDGET_ROSTER]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_ROSTER]->SetDirtyFlag(1);
    GDI::Bitmap* pBitmap = CacheLookup("roster", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_ROSTER]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("rosterr", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_ROSTER]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("rosterd", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_ROSTER]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[TEAMSETUP_WIDGET_STATS]->ClearFrames();
    m_apWidgets[TEAMSETUP_WIDGET_STATS]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_STATS]->SetDirtyFlag(1);
    // STRING: HYPERX 0x004c3238
    pBitmap = CacheLookup("stats", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_STATS]->AddFrameBitmap(pBitmap, nFlag);
    // STRING: HYPERX 0x004c3230
    pBitmap = CacheLookup("statsr", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_STATS]->AddFrameBitmap(pBitmap, nFlag);
    // STRING: HYPERX 0x004c3228
    pBitmap = CacheLookup("statsd", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_STATS]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[TEAMSETUP_WIDGET_ARENA]->ClearFrames();
    m_apWidgets[TEAMSETUP_WIDGET_ARENA]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_ARENA]->SetDirtyFlag(1);
    pBitmap = CacheLookup("Arena", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_ARENA]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Arenar", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_ARENA]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Arenad", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_ARENA]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[TEAMSETUP_WIDGET_GAME_OBJECTS]->ClearFrames();
    m_apWidgets[TEAMSETUP_WIDGET_GAME_OBJECTS]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_GAME_OBJECTS]->SetDirtyFlag(1);
    pBitmap = CacheLookup("gamob2", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_GAME_OBJECTS]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("gamob2r", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_GAME_OBJECTS]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("gamob2d", &nFlag);
    m_apWidgets[TEAMSETUP_WIDGET_GAME_OBJECTS]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[CHAMP_WIDGET_TREE]->ClearFrames();
    m_apWidgets[CHAMP_WIDGET_TREE]->SetHidden(0);
    m_apWidgets[CHAMP_WIDGET_TREE]->SetDirtyFlag(1);
    // STRING: HYPERX 0x004c4198
    pBitmap = CacheLookup("tree", &nFlag);
    m_apWidgets[CHAMP_WIDGET_TREE]->AddFrameBitmap(pBitmap, nFlag);
    // STRING: HYPERX 0x004c4190
    pBitmap = CacheLookup("treer", &nFlag);
    m_apWidgets[CHAMP_WIDGET_TREE]->AddFrameBitmap(pBitmap, nFlag);
    // STRING: HYPERX 0x004c4188
    pBitmap = CacheLookup("treed", &nFlag);
    m_apWidgets[CHAMP_WIDGET_TREE]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->ClearFrames();
    pBitmap = CacheLookup("2five3", &nFlag);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("2five4", &nFlag);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("2five5", &nFlag);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("2gray6", &nFlag);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("2five1", &nFlag);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("2five2", &nFlag);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("2five3", &nFlag);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetDepressed(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDepressed(1);

    m_apWidgets[UI_WIDGET_LEFT_ARROW]->ClearFrames();
    pBitmap = CacheLookup("Stleft", &nFlag);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Stleftr", &nFlag);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Stleftd", &nFlag);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->ClearFrames();
    pBitmap = CacheLookup("Strite", &nFlag);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Striter", &nFlag);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AddFrameBitmap(pBitmap, nFlag);
    pBitmap = CacheLookup("Strited", &nFlag);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->AddFrameBitmap(pBitmap, nFlag);

    m_apWidgets[UI_WIDGET_PAGE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);

    GDI::Bitmap* const hBitmap = GDI::LoadBitmapHandle("armor01");
    GDI::GetBitmapDimensions(hBitmap, &m_bitmap);
    GDI::SafeDeleteBitmap(hBitmap);

    AppendWidget(new LoopFlipbookWidget(0x114, 0x60, m_bitmap.nWidth, m_bitmap.nHeight, 1, 1, 0xe, 0, 0xe, 0xe, this, 1, -1));

    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(nullptr);

    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->SetHidden(1);

    AppendWidget(new DisplayWidget(0, 0, 0, 0, this, -1));
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->SetHidden(1);

    AppendWidget(new DisplayWidget(0x14, 0x46, 0, 0, this, -1));
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->SetHidden(1);

    AppendWidget(new DisplayWidget(0x2a, 6, 0, 0, this, -1));
    m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->SetHidden(1);

    AppendWidget(new DisplayWidget(0x16f, 0x41, 0, 0, this, -1));
    m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->SetHidden(1);

    AppendWidget(new DisplayWidget(0x5f, 0x41, 0, 0, this, -1));
    m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->SetHidden(1);

    m_apWidgets[MATCHINFO_WIDGET_INFO]->SetDepressed(1);

    AppendWidget(new DisplayWidget(g_aChampHeaderCells[0].nX, g_aChampHeaderCells[0].nY, 0, 0, this, -1));
    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->SetHidden(1);

    AppendWidget(new DisplayWidget(g_aChampHeaderCells[1].nX, g_aChampHeaderCells[1].nY, 0, 0, this, -1));
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(nullptr);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->SetHidden(1);

    AppendWidget(new DisplayWidget(g_aChampHeaderCells[2].nX, g_aChampHeaderCells[2].nY, 0, 0, this, -1));
    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->AppendFrame(nullptr);
    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->SetHidden(1);

    g_pGameEngine->SyncPlayerDataToTeam();

    m_nLastGridWidget = CHAMP_GRID_LAST;
    m_nLastWidget     = CHAMP_GRID_LAST;

    for (i = 0; i < kChampGridRows; i++)
    {
        for (int j = 0; j < kChampGridColumns; j++)
        {
            pWidget = AppendWidget(new DisplayWidget(g_aChampGridCells[i * kChampGridColumns + j].nX,
                                                     g_aChampGridCells[i * kChampGridColumns + j].nY,
                                                     0,
                                                     0,
                                                     this,
                                                     -1));
            pWidget->SetDirtyFlag(1);
        }
    }

    m_nLastWidget += kChampBracketCount;

    for (i = 0; i < kChampBracketCount; i++)
    {
        pWidget = AppendWidget(new DisplayWidget(g_aChampBracketCells[i].nX,
                                                 g_aChampBracketCells[i].nY,
                                                 0,
                                                 0,
                                                 this,
                                                 -1));
        pWidget->SetDirtyFlag(1);
    }

    if (m_nPrevState == kEstateJumbotron)
    {
        OnFifthPageTabClick();
        m_apWidgets[CHAMP_WIDGET_TREE]->SetDepressed(1);
        g_bChampFromJumbotron = 1;
    }

    InitAllWidgets();
}

// FUNCTION: HYPERX 0x004581d0
void ChampionshipSetupUIScreen::RefreshTabs()
{
    m_apWidgets[MATCHINFO_WIDGET_INFO]->SetDirtyFlag(1);
    m_apWidgets[MATCHINFO_WIDGET_AWARDS]->SetDirtyFlag(1);
    m_apWidgets[MATCHINFO_WIDGET_DONE]->SetDirtyFlag(1);
    m_apWidgets[MATCHINFO_WIDGET_KIOSK_DONE]->SetDirtyFlag(1);

    if (m_nSelectionMode != CHAMP_WIDGET_TREE)
    {
        m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);
    }
}

// FUNCTION: HYPERX 0x00458210
void ChampionshipSetupUIScreen::OnExitTabClick()
{
    HideBracketTorches();
    HideStatsPanel();

    UnloadBitmaps();

    if (g_bChampFromJumbotron != 0)
    {
        g_bChampFromJumbotron = 0;

        Game::SetEngineState(kEstateDromeExterior);
        return;
    }

    Game::SetEngineState(m_nNextState);
}

// The arena selector refreshes the stats display and does not show the bracket cells.
// FUNCTION: HYPERX 0x00458260
void ChampionshipSetupUIScreen::OnFirstPageTabClick()
{
    HideBracketTorches();

    HideStatsPanel();

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDirtyFlag(1);

    m_nSelectionMode = TEAMSETUP_WIDGET_ROSTER;

    RefreshSelection();
}

// FUNCTION: HYPERX 0x004582c0
void ChampionshipSetupUIScreen::OnSecondPageTabClick()
{
    HideBracketTorches();

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDirtyFlag(1);

    m_nSelectionMode = TEAMSETUP_WIDGET_STATS;

    RefreshSelection();

    RefreshStatsDisplay();
}

// FUNCTION: HYPERX 0x00458320
void ChampionshipSetupUIScreen::OnThirdPageTabClick()
{
    HideBracketTorches();

    HideStatsPanel();

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDirtyFlag(1);

    m_nSelectionMode = TEAMSETUP_WIDGET_ARENA;

    RefreshSelection();
}

// FUNCTION: HYPERX 0x00458380
void ChampionshipSetupUIScreen::OnFourthPageTabClick()
{
    HideBracketTorches();

    HideStatsPanel();

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(0);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetDirtyFlag(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(0);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetDirtyFlag(1);

    m_nSelectionMode = TEAMSETUP_WIDGET_GAME_OBJECTS;

    RefreshSelection();
}

// FUNCTION: HYPERX 0x004583e0
void ChampionshipSetupUIScreen::OnFifthPageTabClick()
{
    HideStatsPanel();

    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(1);

    m_nSelectionMode = CHAMP_WIDGET_TREE;

    RefreshSelection();

    LoadBracketTorches();
}

// FUNCTION: HYPERX 0x00458420
void ChampionshipSetupUIScreen::OnLeftArrowClick()
{
    short nTeam;

    switch (m_nSelectionMode)
    {
        case TEAMSETUP_WIDGET_ROSTER:
            if (--m_nTeamSlot < 0)
            {
                m_nTeamSlot = m_nTeamSlotCount - 1;
            }

            nTeam = g_pGameEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(m_nTeamSlot / 4), 0);

            m_nTeamSlot = m_nTeamSlot % 4 + nTeam * 4;

            StepHomeSelection(0);

            RefreshSelection();
            break;

        case TEAMSETUP_WIDGET_STATS:
            m_bShowHomeTeam = !m_bShowHomeTeam;

            RefreshSelection();

            RefreshStatsDisplay();
            break;

        case TEAMSETUP_WIDGET_ARENA:
            if (--m_nArena < 0)
            {
                m_nArena = m_nArenaCount - 1;
            }

            nTeam = g_pGameEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(m_nArena), 0);

            m_nArena = nTeam;

            StepArenaSelection(0);

            RefreshSelection();
            break;

        case TEAMSETUP_WIDGET_GAME_OBJECTS:
            if (--m_nObstacleSet < 0)
            {
                m_nObstacleSet = m_nObstacleSetCount - 1;
            }

            RefreshSelection();
            break;
    }
}

// FUNCTION: HYPERX 0x00458560
void ChampionshipSetupUIScreen::OnRightArrowClick()
{
    short nTeam;

    switch (m_nSelectionMode)
    {
        case TEAMSETUP_WIDGET_ROSTER:
            m_nTeamSlot++;

            if (m_nTeamSlot >= m_nTeamSlotCount)
            {
                m_nTeamSlot = 0;
            }

            nTeam = g_pGameEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(m_nTeamSlot / 4), 1);

            m_nTeamSlot = m_nTeamSlot % 4 + nTeam * 4;

            StepHomeSelection(1);

            RefreshSelection();
            break;

        case TEAMSETUP_WIDGET_STATS:
            m_bShowHomeTeam = !m_bShowHomeTeam;

            RefreshSelection();

            RefreshStatsDisplay();
            break;

        case TEAMSETUP_WIDGET_ARENA:
            m_nArena++;

            if (m_nArena >= m_nArenaCount)
            {
                m_nArena = 0;
            }

            nTeam = g_pGameEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(m_nArena), 1);

            m_nArena = nTeam;

            StepArenaSelection(1);

            RefreshSelection();
            break;

        case TEAMSETUP_WIDGET_GAME_OBJECTS:
            m_nObstacleSet++;

            if (m_nObstacleSet >= m_nObstacleSetCount)
            {
                m_nObstacleSet = 0;
            }

            RefreshSelection();
            break;
    }
}

// The object text falls to the stub where its neighbours are silent.
// FUNCTION: HYPERX 0x004586b0
void ChampionshipSetupUIScreen::OnOtherButton(int nMsg, unsigned int nId)
{
    switch (nId)
    {
        case TEAMSETUP_WIDGET_OBJECT_ANIMATION:
            ResetFrameOnLoopDone(nMsg);
            break;

        case TEAMSETUP_WIDGET_OBJECT_TITLE:
        case TEAMSETUP_WIDGET_TEAM_TITLE:
        case TEAMSETUP_WIDGET_DEFENDER_CHART:
        case TEAMSETUP_WIDGET_STRIKER_CHART:
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// A column outside the range calls the stub.
// FUNCTION: HYPERX 0x00458700
void ChampionshipSetupUIScreen::RefreshSelection()
{
    RefreshFrames();

    switch (m_nSelectionMode)
    {
        case TEAMSETUP_WIDGET_ROSTER:
            LoadObstacleFrames(0);
            LoadTeamBitmaps(1);
            break;

        case TEAMSETUP_WIDGET_GAME_OBJECTS:
            LoadObstacleFrames(1);
            LoadTeamBitmaps(0);
            break;

        case TEAMSETUP_WIDGET_STATS:
        case TEAMSETUP_WIDGET_ARENA:
        case CHAMP_WIDGET_TREE:
            LoadObstacleFrames(0);
            LoadTeamBitmaps(0);
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }
}

// FUNCTION: HYPERX 0x00458770
void ChampionshipSetupUIScreen::LoadTeamBitmaps(int nShow)
{
    if (nShow != 0 && m_nTeamSlot % 4 == 0)
    {
        m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->ClearFrames();
        m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->ClearFrames();
        m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->ClearFrames();

        switch (m_nTeamSlot / 4)
        {
            case 0:
                // STRING: HYPERX 0x004c2f28
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Chiplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3318
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("chisub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3310
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("chisub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 1:
                // STRING: HYPERX 0x004c2f20
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Houplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3308
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("housub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3300
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("housub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 2:
                // STRING: HYPERX 0x004c2f18
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Jamplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c32f8
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("jamsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c32f0
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("jamsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 4:
                // STRING: HYPERX 0x004c2f10
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Lndplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c32e8
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("lndsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c32e0
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("lndsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 5:
                // STRING: HYPERX 0x004c2f08
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Losplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c32d8
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("lossub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c32d0
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("lossub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 6:
                // STRING: HYPERX 0x004c2f00
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Mosplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c32c8
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("mossub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c32c0
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("mossub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 7:
                // STRING: HYPERX 0x004c2ef8
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Munplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c32b8
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("munsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c32b0
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("munsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 8:
                // STRING: HYPERX 0x004c2ef0
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Nemplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c32a8
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("nemsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c32a0
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("nemsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 9:
                // STRING: HYPERX 0x004c2ee8
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Newplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3298
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("newsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3290
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("newsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 10:
                // STRING: HYPERX 0x004c2ee0
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Seaplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3288
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("seasub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3280
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("seasub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 11:
                // STRING: HYPERX 0x004c2ed8
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Tokplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3278
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("toksub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3270
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("toksub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 12:
                // STRING: HYPERX 0x004c2ed0
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Torplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3268
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("torsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3260
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("torsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 3:
                // STRING: HYPERX 0x004c2ec8
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Vegplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3258
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("vegsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3250
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("vegsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            case 13:
                // STRING: HYPERX 0x004c2ec0
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("Warplcd"));
                m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->InitAfterLoad();

                // STRING: HYPERX 0x004c3248
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->AppendFrame(GDI::LoadBitmapHandle("warsub1"));
                m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->InitAfterLoad();

                // STRING: HYPERX 0x004c3240
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->AppendFrame(GDI::LoadBitmapHandle("warsub2"));
                m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->InitAfterLoad();
                break;

            default:
                RELEASE_ASSERT(0);
                break;
        }

        m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->SetDirtyFlag(1);
        m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->SetHidden(0);
        m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->SetDirtyFlag(1);
        m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->SetHidden(0);
        m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->SetDirtyFlag(1);
        m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->SetHidden(0);
    }
    else
    {
        m_apWidgets[TEAMSETUP_WIDGET_TEAM_TITLE]->SetHidden(1);
        m_apWidgets[TEAMSETUP_WIDGET_DEFENDER_CHART]->SetHidden(1);
        m_apWidgets[TEAMSETUP_WIDGET_STRIKER_CHART]->SetHidden(1);
    }
}

// The switch reads m_nObstacleSet; the argument only picks between the two halves.
// FUNCTION: HYPERX 0x00459170
void ChampionshipSetupUIScreen::LoadObstacleFrames(int nBuild)
{
    if (nBuild == 0)
    {
        m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->SetHidden(1);
        m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->SetHidden(1);
        m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->SetHidden(1);

        return;
    }

    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->ClearFrames();
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->ClearFrames();
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->ClearFrames();

    switch (m_nObstacleSet)
    {
        case 0:
            // STRING: HYPERX 0x004c3a80
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdsta"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3a78
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objstar"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3a70
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn01"));
            // STRING: HYPERX 0x004c3a68
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn02"));
            // STRING: HYPERX 0x004c3a60
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn03"));
            // STRING: HYPERX 0x004c3a58
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn04"));
            // STRING: HYPERX 0x004c3a50
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn05"));
            // STRING: HYPERX 0x004c3a48
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn06"));
            // STRING: HYPERX 0x004c3a40
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn07"));
            // STRING: HYPERX 0x004c3a38
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn08"));
            // STRING: HYPERX 0x004c3a30
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn09"));
            // STRING: HYPERX 0x004c3a28
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn10"));
            // STRING: HYPERX 0x004c3a20
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn11"));
            // STRING: HYPERX 0x004c3a18
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn12"));
            // STRING: HYPERX 0x004c3a10
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn13"));
            // STRING: HYPERX 0x004c3a08
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn14"));
            // STRING: HYPERX 0x004c3a00
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("strgn15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 1:
            // STRING: HYPERX 0x004c39f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdrok"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c39f0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objrok"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c39e8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok01"));
            // STRING: HYPERX 0x004c39e0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok02"));
            // STRING: HYPERX 0x004c39d8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok03"));
            // STRING: HYPERX 0x004c39d0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok04"));
            // STRING: HYPERX 0x004c39c8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok05"));
            // STRING: HYPERX 0x004c39c0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok06"));
            // STRING: HYPERX 0x004c39b8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok07"));
            // STRING: HYPERX 0x004c39b0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok08"));
            // STRING: HYPERX 0x004c39a8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok09"));
            // STRING: HYPERX 0x004c39a0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok10"));
            // STRING: HYPERX 0x004c3998
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok11"));
            // STRING: HYPERX 0x004c3990
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok12"));
            // STRING: HYPERX 0x004c3988
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok13"));
            // STRING: HYPERX 0x004c3980
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok14"));
            // STRING: HYPERX 0x004c3978
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("rok15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 2:
            // STRING: HYPERX 0x004c3970
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdhur"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3968
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objhurd"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3960
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl01"));
            // STRING: HYPERX 0x004c3958
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl02"));
            // STRING: HYPERX 0x004c3950
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl03"));
            // STRING: HYPERX 0x004c3948
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl04"));
            // STRING: HYPERX 0x004c3940
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl05"));
            // STRING: HYPERX 0x004c3938
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl06"));
            // STRING: HYPERX 0x004c3930
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl07"));
            // STRING: HYPERX 0x004c3928
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl08"));
            // STRING: HYPERX 0x004c3920
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl09"));
            // STRING: HYPERX 0x004c3918
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl10"));
            // STRING: HYPERX 0x004c3910
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl11"));
            // STRING: HYPERX 0x004c3908
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl12"));
            // STRING: HYPERX 0x004c3900
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl13"));
            // STRING: HYPERX 0x004c38f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl14"));
            // STRING: HYPERX 0x004c38f0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("hurdl15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 3:
            // STRING: HYPERX 0x004c38e8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdram"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c38e0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objramp"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c38d8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp01"));
            // STRING: HYPERX 0x004c38d0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp02"));
            // STRING: HYPERX 0x004c38c8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp03"));
            // STRING: HYPERX 0x004c38c0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp04"));
            // STRING: HYPERX 0x004c38b8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp05"));
            // STRING: HYPERX 0x004c38b0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp06"));
            // STRING: HYPERX 0x004c38a8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp07"));
            // STRING: HYPERX 0x004c38a0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp08"));
            // STRING: HYPERX 0x004c3898
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp09"));
            // STRING: HYPERX 0x004c3890
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp10"));
            // STRING: HYPERX 0x004c3888
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp11"));
            // STRING: HYPERX 0x004c3880
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp12"));
            // STRING: HYPERX 0x004c3878
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp13"));
            // STRING: HYPERX 0x004c3870
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp14"));
            // STRING: HYPERX 0x004c3868
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("ramp15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 4:
            // STRING: HYPERX 0x004c3860
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdtur"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3858
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objturn"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3850
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns01"));
            // STRING: HYPERX 0x004c3848
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns02"));
            // STRING: HYPERX 0x004c3840
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns03"));
            // STRING: HYPERX 0x004c3838
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns04"));
            // STRING: HYPERX 0x004c3830
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns05"));
            // STRING: HYPERX 0x004c3828
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns06"));
            // STRING: HYPERX 0x004c3820
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns07"));
            // STRING: HYPERX 0x004c3818
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns08"));
            // STRING: HYPERX 0x004c3810
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns09"));
            // STRING: HYPERX 0x004c3808
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns10"));
            // STRING: HYPERX 0x004c3800
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns11"));
            // STRING: HYPERX 0x004c37f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns12"));
            // STRING: HYPERX 0x004c37f0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns13"));
            // STRING: HYPERX 0x004c37e8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns14"));
            // STRING: HYPERX 0x004c37e0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("turns15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 5:
            // STRING: HYPERX 0x004c37d8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdarm"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c37d0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objarm"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c31f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor01"));
            // STRING: HYPERX 0x004c37c8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor02"));
            // STRING: HYPERX 0x004c37c0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor03"));
            // STRING: HYPERX 0x004c37b8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor04"));
            // STRING: HYPERX 0x004c37b0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor05"));
            // STRING: HYPERX 0x004c37a8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor06"));
            // STRING: HYPERX 0x004c37a0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor07"));
            // STRING: HYPERX 0x004c3798
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor08"));
            // STRING: HYPERX 0x004c3790
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor09"));
            // STRING: HYPERX 0x004c3788
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor10"));
            // STRING: HYPERX 0x004c3780
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor11"));
            // STRING: HYPERX 0x004c3778
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor12"));
            // STRING: HYPERX 0x004c3770
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor13"));
            // STRING: HYPERX 0x004c3768
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor14"));
            // STRING: HYPERX 0x004c3760
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("armor15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 6:
            // STRING: HYPERX 0x004c3758
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdmin"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3750
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objmine"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3748
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine01"));
            // STRING: HYPERX 0x004c3740
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine02"));
            // STRING: HYPERX 0x004c3738
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine03"));
            // STRING: HYPERX 0x004c3730
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine04"));
            // STRING: HYPERX 0x004c3728
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine05"));
            // STRING: HYPERX 0x004c3720
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine06"));
            // STRING: HYPERX 0x004c3718
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine07"));
            // STRING: HYPERX 0x004c3710
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine08"));
            // STRING: HYPERX 0x004c3708
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine09"));
            // STRING: HYPERX 0x004c3700
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine10"));
            // STRING: HYPERX 0x004c36f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine11"));
            // STRING: HYPERX 0x004c36f0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine12"));
            // STRING: HYPERX 0x004c36e8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine13"));
            // STRING: HYPERX 0x004c36e0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine14"));
            // STRING: HYPERX 0x004c36d8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("mine15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 7:
            // STRING: HYPERX 0x004c36d0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdswr"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c36c8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objswar"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c36c0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf01"));
            // STRING: HYPERX 0x004c36b8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf02"));
            // STRING: HYPERX 0x004c36b0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf03"));
            // STRING: HYPERX 0x004c36a8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf04"));
            // STRING: HYPERX 0x004c36a0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf05"));
            // STRING: HYPERX 0x004c3698
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf06"));
            // STRING: HYPERX 0x004c3690
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf07"));
            // STRING: HYPERX 0x004c3688
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf08"));
            // STRING: HYPERX 0x004c3680
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf09"));
            // STRING: HYPERX 0x004c3678
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf10"));
            // STRING: HYPERX 0x004c3670
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf11"));
            // STRING: HYPERX 0x004c3668
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf12"));
            // STRING: HYPERX 0x004c3660
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf13"));
            // STRING: HYPERX 0x004c3658
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf14"));
            // STRING: HYPERX 0x004c3650
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("swarf15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 8:
            // STRING: HYPERX 0x004c3648
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdgre"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3640
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objgrnd"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3638
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd01"));
            // STRING: HYPERX 0x004c3630
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd02"));
            // STRING: HYPERX 0x004c3628
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd03"));
            // STRING: HYPERX 0x004c3620
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd04"));
            // STRING: HYPERX 0x004c3618
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd05"));
            // STRING: HYPERX 0x004c3610
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd06"));
            // STRING: HYPERX 0x004c3608
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd07"));
            // STRING: HYPERX 0x004c3600
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd08"));
            // STRING: HYPERX 0x004c35f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd09"));
            // STRING: HYPERX 0x004c35f0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd10"));
            // STRING: HYPERX 0x004c35e8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd11"));
            // STRING: HYPERX 0x004c35e0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd12"));
            // STRING: HYPERX 0x004c35d8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd13"));
            // STRING: HYPERX 0x004c35d0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd14"));
            // STRING: HYPERX 0x004c35c8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("grnrd15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 9:
            // STRING: HYPERX 0x004c35c0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdred"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c35b8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objrrnd"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c35b0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd01"));
            // STRING: HYPERX 0x004c35a8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd02"));
            // STRING: HYPERX 0x004c35a0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd03"));
            // STRING: HYPERX 0x004c3598
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd04"));
            // STRING: HYPERX 0x004c3590
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd05"));
            // STRING: HYPERX 0x004c3588
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd06"));
            // STRING: HYPERX 0x004c3580
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd07"));
            // STRING: HYPERX 0x004c3578
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd08"));
            // STRING: HYPERX 0x004c3570
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd09"));
            // STRING: HYPERX 0x004c3568
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd10"));
            // STRING: HYPERX 0x004c3560
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd11"));
            // STRING: HYPERX 0x004c3558
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd12"));
            // STRING: HYPERX 0x004c3550
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd13"));
            // STRING: HYPERX 0x004c3548
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd14"));
            // STRING: HYPERX 0x004c3540
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("redrd15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 10:
            // STRING: HYPERX 0x004c3538
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdkbr"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3530
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objkbr"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3528
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk01"));
            // STRING: HYPERX 0x004c3520
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk02"));
            // STRING: HYPERX 0x004c3518
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk03"));
            // STRING: HYPERX 0x004c3510
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk04"));
            // STRING: HYPERX 0x004c3508
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk05"));
            // STRING: HYPERX 0x004c3500
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk06"));
            // STRING: HYPERX 0x004c34f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk07"));
            // STRING: HYPERX 0x004c34f0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk08"));
            // STRING: HYPERX 0x004c34e8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk09"));
            // STRING: HYPERX 0x004c34e0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk10"));
            // STRING: HYPERX 0x004c34d8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk11"));
            // STRING: HYPERX 0x004c34d0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk12"));
            // STRING: HYPERX 0x004c34c8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk13"));
            // STRING: HYPERX 0x004c34c0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk14"));
            // STRING: HYPERX 0x004c34b8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilrk15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 11:
            // STRING: HYPERX 0x004c34b0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdkil"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c34a8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objkbc"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c34a0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba01"));
            // STRING: HYPERX 0x004c3498
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba02"));
            // STRING: HYPERX 0x004c3490
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba03"));
            // STRING: HYPERX 0x004c3488
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba04"));
            // STRING: HYPERX 0x004c3480
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba05"));
            // STRING: HYPERX 0x004c3478
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba06"));
            // STRING: HYPERX 0x004c3470
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba07"));
            // STRING: HYPERX 0x004c3468
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba08"));
            // STRING: HYPERX 0x004c3460
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba09"));
            // STRING: HYPERX 0x004c3458
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba10"));
            // STRING: HYPERX 0x004c3450
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba11"));
            // STRING: HYPERX 0x004c3448
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba12"));
            // STRING: HYPERX 0x004c3440
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba13"));
            // STRING: HYPERX 0x004c3438
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba14"));
            // STRING: HYPERX 0x004c3430
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("kilba15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 12:
            // STRING: HYPERX 0x004c3428
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdmul"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3420
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objmult"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3418
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi01"));
            // STRING: HYPERX 0x004c3410
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi02"));
            // STRING: HYPERX 0x004c3408
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi03"));
            // STRING: HYPERX 0x004c3400
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi04"));
            // STRING: HYPERX 0x004c33f8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi05"));
            // STRING: HYPERX 0x004c33f0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi06"));
            // STRING: HYPERX 0x004c33e8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi07"));
            // STRING: HYPERX 0x004c33e0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi08"));
            // STRING: HYPERX 0x004c33d8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi09"));
            // STRING: HYPERX 0x004c33d0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi10"));
            // STRING: HYPERX 0x004c33c8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi11"));
            // STRING: HYPERX 0x004c33c0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi12"));
            // STRING: HYPERX 0x004c33b8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi13"));
            // STRING: HYPERX 0x004c33b0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi14"));
            // STRING: HYPERX 0x004c33a8
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("multi15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        case 13:
            // STRING: HYPERX 0x004c33a0
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->AppendFrame(GDI::LoadBitmapHandle("objdlas"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->InitAfterLoad();
            // STRING: HYPERX 0x004c3398
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->AppendFrame(GDI::LoadBitmapHandle("Objlas"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->InitAfterLoad();
            // STRING: HYPERX 0x004c3390
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur01"));
            // STRING: HYPERX 0x004c3388
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur02"));
            // STRING: HYPERX 0x004c3380
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur03"));
            // STRING: HYPERX 0x004c3378
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur04"));
            // STRING: HYPERX 0x004c3370
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur05"));
            // STRING: HYPERX 0x004c3368
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur06"));
            // STRING: HYPERX 0x004c3360
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur07"));
            // STRING: HYPERX 0x004c3358
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur08"));
            // STRING: HYPERX 0x004c3350
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur09"));
            // STRING: HYPERX 0x004c3348
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur10"));
            // STRING: HYPERX 0x004c3340
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur11"));
            // STRING: HYPERX 0x004c3338
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur12"));
            // STRING: HYPERX 0x004c3330
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur13"));
            // STRING: HYPERX 0x004c3328
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur14"));
            // STRING: HYPERX 0x004c3320
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->AppendFrame(GDI::LoadBitmapHandle("lzhur15"));
            m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->InitAfterLoad();
            break;

        default:
            RELEASE_ASSERT(0);
            break;
    }

    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->SetDirtyFlag(1);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->SetCurrentFrame(0);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->SetHighlighted(1);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->SetDepressed(1);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_ANIMATION]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->SetDirtyFlag(1);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TITLE]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->SetDirtyFlag(1);
    m_apWidgets[TEAMSETUP_WIDGET_OBJECT_TEXT]->SetHidden(0);
}

// Mode eight's index / 4 picks a team and % 4 a picture.

#define GROUP 4

// GLOBAL: HYPERX 0x004c3f80
extern const char* g_pszTournamentBackground;

// FUNCTION: HYPERX 0x0045af00
void ChampionshipSetupUIScreen::RefreshFrames()
{
    int nIndex;

    m_apWidgets[UI_WIDGET_PAGE]->ClearFrames();

    switch (m_nSelectionMode)
    {
        case TEAMSETUP_WIDGET_ROSTER:
            nIndex = m_nTeamSlot;

            switch (nIndex / GROUP)
            {
                case 0:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            // STRING: HYPERX 0x004c3c58
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3c50
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcchipe"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3c48
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcchicb"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3c40
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcchihv"));

                            break;
                    }

                    break;

                case 1:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3c38
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pchoujw"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3c30
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pchoukr"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3c28
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pchoumg"));

                            break;
                    }

                    break;

                case 2:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3c20
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcjamjx"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3c18
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcjamnb"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3c10
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcjamhs"));

                            break;
                    }

                    break;

                case 3:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3c08
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcvegjf"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3c00
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcvegvo"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3bf8
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcvegbr"));

                            break;
                    }

                    break;

                case 4:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3bf0
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pclndnm"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3be8
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pclndmp"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3be0
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pclnddm"));

                            break;
                    }

                    break;

                case 5:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3bd8
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pclosjv"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3bd0
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pclosam"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3bc8
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pclosde"));

                            break;
                    }

                    break;

                case 6:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3bc0
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcmosik"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3bb8
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcmosas"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3bb0
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcmosfm"));

                            break;
                    }

                    break;

                case 7:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3ba8
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcmunzm"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3ba0
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcmunkw"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3b98
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcmunhb"));

                            break;
                    }

                    break;

                case 8:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3b90
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcnems"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3b88
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcnemd"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3b80
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcnemg"));

                            break;
                    }

                    break;

                case 9:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3b78
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcnewbj"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3b70
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcnewjr"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3b68
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcnewtg"));

                            break;
                    }

                    break;

                case 10:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3b60
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcseasv"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3b58
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcseatk"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3b50
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcseatm"));

                            break;
                    }

                    break;

                case 11:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3b48
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pctokht"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3b40
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pctokhs"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3b38
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pctokor"));

                            break;
                    }

                    break;

                case 12:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3b30
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pctorsk"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3b28
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pctorjh"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3b20
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pctorrz"));

                            break;
                    }

                    break;

                case 13:
                    switch (nIndex % GROUP)
                    {
                        case 0:
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Pcrost"));

                            break;

                        case 1:
                            // STRING: HYPERX 0x004c3b18
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcwarvw"));

                            break;

                        case 2:
                            // STRING: HYPERX 0x004c3b10
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcwarws"));

                            break;

                        case 3:
                            // STRING: HYPERX 0x004c3b08
                            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("pcwarjm"));

                            break;
                    }

                    break;

                default:
                    RELEASE_ASSERT(0);

                    break;
            }

            break;

        case TEAMSETUP_WIDGET_STATS:
            // STRING: HYPERX 0x004c3b00
            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("curgame"));

            break;

        case TEAMSETUP_WIDGET_ARENA:
            switch (m_nArena)
            {
                case 0:
                    // STRING: HYPERX 0x004c3af8
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("Drmlchi"));

                    break;

                case 1:
                    // STRING: HYPERX 0x004c3af0
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlHou"));

                    break;

                case 2:
                    // STRING: HYPERX 0x004c3ae8
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlJam"));

                    break;

                case 4:
                    // STRING: HYPERX 0x004c3ae0
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlLnd"));

                    break;

                case 5:
                    // STRING: HYPERX 0x004c3ad8
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlLos"));

                    break;

                case 6:
                    // STRING: HYPERX 0x004c3ad0
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlMos"));

                    break;

                case 7:
                    // STRING: HYPERX 0x004c3ac8
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlMun"));

                    break;

                case 8:
                    // STRING: HYPERX 0x004c3ac0
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlNem"));

                    break;

                case 9:
                    // STRING: HYPERX 0x004c3ab8
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlNew"));

                    break;

                case 10:
                    // STRING: HYPERX 0x004c3ab0
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlSea"));

                    break;

                case 11:
                    // STRING: HYPERX 0x004c3aa8
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlTok"));

                    break;

                case 12:
                    // STRING: HYPERX 0x004c3aa0
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlTor"));

                    break;

                case 3:
                    // STRING: HYPERX 0x004c3a98
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlVeg"));

                    break;

                case 13:
                    // STRING: HYPERX 0x004c3a90
                    m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("DrmlWar"));

                    break;
                default:
                    RELEASE_ASSERT(0);

                    break;
            }

            break;

        case TEAMSETUP_WIDGET_GAME_OBJECTS:
            // STRING: HYPERX 0x004c3a88
            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle("objdes"));

            break;

        case CHAMP_WIDGET_TREE:
            m_apWidgets[UI_WIDGET_PAGE]->AppendFrame(GDI::LoadBitmapHandle(g_pszTournamentBackground));

            break;

        default:
            RELEASE_ASSERT(0);

            break;
    }

    InitAllWidgets();

    m_apWidgets[UI_WIDGET_PAGE]->SetDirtyFlag(1);
}

// FUNCTION: HYPERX 0x0045bc40
void ChampionshipSetupUIScreen::UpdatePlayerRows()
{
    char             szText[256];
    const TeamBlock* pTeam;
    int              nPlayer;

    for (int nStat = kMatchStatTotalPoints; nStat < kMatchStatCount; nStat++)
    {
        int nTotal = 0;

        if (m_bShowHomeTeam != 0)
        {
            pTeam = g_pGameEngine->m_pTeamA;
        }
        else
        {
            pTeam = g_pGameEngine->m_pTeamB;
        }

        const int      nRow = (nStat - kMatchStatTotalPoints) * kChampGridColumns;
        Widget** const pRow = &m_apWidgets[nRow];

        for (nPlayer = 0; nPlayer < kTeamRowCount; nPlayer++)
        {
            const float*  pStats  = pTeam->aStatRows[nPlayer].aStat;
            Widget* const pWidget = pRow[CHAMP_WIDGET_STAT_GRID + nPlayer];

            if (nStat == kMatchStatTimeOfPoss)
            {
                const int nTime = static_cast<int>(pStats[nStat] * 0.001f);

                StatColumns::FormatClock(szText, static_cast<short>(nTime));

                nTotal += nTime;
            }
            else if (nStat == kMatchStatShootingPct)
            {
                // STRING: HYPERX 0x004c3c70
                sprintf(szText, "%.2f%%", pStats[nStat]);

                if (pStats[kMatchStatShotsOnGoal] > 0.0f)
                {
                }
            }
            else
            {
                // STRING: HYPERX 0x004c3c68
                sprintf(szText, "%2hd", static_cast<short>(pStats[nStat]));

                nTotal += static_cast<int>(pStats[nStat]);
            }

            pWidget->SetText(szText);
        }

        if (nStat == kMatchStatShootingPct)
        {
            int nAttempts = 0;
            int nMade     = 0;

            for (nPlayer = 0; nPlayer < kTeamRowCount; nPlayer++)
            {
                nAttempts += static_cast<int>(pTeam->aStatRows[nPlayer].aStat[kMatchStatShotsOnGoal]);
                nMade += static_cast<int>(pTeam->aStatRows[nPlayer].aStat[kMatchStatTotalGoals]);
            }

            if (nAttempts > 0)
            {
                sprintf(szText, "%.2f%%", static_cast<float>(nMade) / nAttempts * 100.0f);
            }
            else
            {
                // STRING: HYPERX 0x004c3c60
                sprintf(szText, "0.00%%");
            }
        }
        else if (nStat == kMatchStatTimeOfPoss)
        {
            StatColumns::FormatClock(szText, static_cast<short>(nTotal));
        }
        else
        {
            sprintf(szText, "%2hd", static_cast<short>(nTotal));
        }

        pRow[CHAMP_WIDGET_STAT_GRID + kTeamRowCount]->SetText(szText);
    }
}

// FUNCTION: HYPERX 0x0045bf00
void ChampionshipSetupUIScreen::HideStatsPanel()
{
    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->SetHidden(1);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->SetDirtyFlag(0);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->SetHidden(1);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->SetDirtyFlag(0);

    for (int nRow = 0; nRow < kChampGridRows; nRow++)
    {
        for (int nColumn = 0; nColumn < kChampGridColumns; nColumn++)
        {
            Widget* const pCell = m_apWidgets[CHAMP_WIDGET_STAT_GRID + nRow * kChampGridColumns + nColumn];

            pCell->SetHidden(1);
            pCell->SetDirtyFlag(0);
        }
    }
}

// FUNCTION: HYPERX 0x0045bf70
void ChampionshipSetupUIScreen::RefreshStatsDisplay()
{
    short nTeam;

    UpdatePlayerRows();

    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->SetDirtyFlag(1);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->SetHidden(0);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->SetDirtyFlag(1);
    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->ClearFrames();
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->ClearFrames();

    if (m_bShowHomeTeam != 0)
    {
        nTeam = g_pGameEngine->m_nTeamHomeIndex;
    }
    else
    {
        nTeam = g_pGameEngine->m_nTeamAwayIndex;
    }

    switch (nTeam)
    {
        case 0:
            // STRING: HYPERX 0x004c3d68
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbchi"));
            // STRING: HYPERX 0x004c3d60
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgachi"));
            break;

        case 1:
            // STRING: HYPERX 0x004c3d58
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbhou"));
            // STRING: HYPERX 0x004c3d50
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgahou"));
            break;

        case 2:
            // STRING: HYPERX 0x004c3d48
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbjam"));
            // STRING: HYPERX 0x004c3d40
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgajam"));
            break;

        case 4:
            // STRING: HYPERX 0x004c3d38
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgblnd"));
            // STRING: HYPERX 0x004c3d30
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgalnd"));
            break;

        case 5:
            // STRING: HYPERX 0x004c3d28
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgblos"));
            // STRING: HYPERX 0x004c3d20
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgalos"));
            break;

        case 6:
            // STRING: HYPERX 0x004c3d18
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbmos"));
            // STRING: HYPERX 0x004c3d10
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgamos"));
            break;

        case 7:
            // STRING: HYPERX 0x004c3d08
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbmun"));
            // STRING: HYPERX 0x004c3d00
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgamun"));
            break;

        case 9:
            // STRING: HYPERX 0x004c3cf8
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbnew"));
            // STRING: HYPERX 0x004c3cf0
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crganew"));
            break;

        case 10:
            // STRING: HYPERX 0x004c3ce8
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbsea"));
            // STRING: HYPERX 0x004c3ce0
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgasea"));
            break;

        case 11:
            // STRING: HYPERX 0x004c3cd8
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbtok"));
            // STRING: HYPERX 0x004c3cd0
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgatok"));
            break;

        case 12:
            // STRING: HYPERX 0x004c3cc8
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbtor"));
            // STRING: HYPERX 0x004c3cc0
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgator"));
            break;

        case 3:
            // STRING: HYPERX 0x004c3cb8
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbveg"));
            // STRING: HYPERX 0x004c3cb0
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgaveg"));
            break;

        case 13:
            // STRING: HYPERX 0x004c3ca8
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbwar"));
            // STRING: HYPERX 0x004c3ca0
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crgawar"));
            break;

        case 8:
            // STRING: HYPERX 0x004c3c98
            m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->AppendFrame(GDI::LoadBitmapHandle("crgbnem"));
            // STRING: HYPERX 0x004c3c90
            m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->AppendFrame(GDI::LoadBitmapHandle("crganem"));
            break;
    }

    m_apWidgets[TEAMSETUP_WIDGET_STATS_PLAYER_NAMES]->InitAfterLoad();
    m_apWidgets[TEAMSETUP_WIDGET_STATS_TEAM_TITLE]->InitAfterLoad();

    for (int nRow = 0; nRow < kChampGridRows; nRow++)
    {
        for (int nColumn = 0; nColumn < kChampGridColumns; nColumn++)
        {
            Widget* const pWidget = m_apWidgets[CHAMP_WIDGET_STAT_GRID + nRow * kChampGridColumns + nColumn];

            pWidget->SetHidden(0);
            pWidget->SetDirtyFlag(1);
        }
    }

    m_bRepaintBackground = 1;

    InitFrame();
}

// FUNCTION: HYPERX 0x0045c3c0
void ChampionshipSetupUIScreen::LoadBracketTorches()
{
    Widget* pTorch;
    int     nEntry;
    int     i;

    const int nHomeTeam = g_pGameEngine->m_pSaveObject->GetHomeTeam();
    const int nCount    = g_pGameEngine->m_pSaveObject->GetBracketPos();

    g_pGameEngine->m_pSaveObject->BracketIterReset();

    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->ClearFrames();
    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->AppendFrame(GDI::LoadBitmapHandle(g_aTorchBitmaps[nHomeTeam].pszWon));
    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->SetHidden(0);
    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->SetDirtyFlag(1);

    for (i = 0; i < (std::min)(nCount, kChampBracketCount); i++)
    {
        nEntry = g_pGameEngine->m_pSaveObject->BracketIterPeek();

        g_pGameEngine->m_pSaveObject->BracketIterAdvance();

        pTorch = m_apWidgets[m_nLastGridWidget + 1 + i];

        pTorch->ClearFrames();
        pTorch->AppendFrame(GDI::LoadBitmapHandle(g_aTorchBitmaps[nEntry].pszLost));
        pTorch->SetHidden(0);
        pTorch->SetDirtyFlag(1);
    }

    for (i = nCount; i < kChampBracketCount; i++)
    {
        nEntry = g_pGameEngine->m_pSaveObject->BracketIterPeek();

        g_pGameEngine->m_pSaveObject->BracketIterAdvance();

        pTorch = m_apWidgets[m_nLastGridWidget + 1 + i];

        pTorch->ClearFrames();
        pTorch->AppendFrame(GDI::LoadBitmapHandle(g_aTorchBitmaps[nEntry].pszPending));
        pTorch->SetHidden(0);
        pTorch->SetDirtyFlag(1);
    }

    g_pGameEngine->m_pSaveObject->SetBracketPos(static_cast<short>(nCount));
    g_pGameEngine->m_pSaveObject->BracketIterPeek();

    m_bTorchesLoaded = 1;

    InitFrame();
}

// The page index is a widget offset, already multiplied by the caller.
// FUNCTION: HYPERX 0x0045c590
void ChampionshipSetupUIScreen::HideBracketTorches()
{
    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->SetHidden(1);
    m_apWidgets[CHAMP_WIDGET_ROUND_TORCH]->SetDirtyFlag(0);

    for (int nCell = 0; nCell < kChampBracketCount; nCell++)
    {
        Widget* const pCell = m_apWidgets[nCell + m_nLastGridWidget + 1];

        pCell->SetHidden(1);
        pCell->SetDirtyFlag(0);
    }

    m_bTorchesLoaded = 0;
}

ConfigPoint g_aChampHeaderCells[3] = { { 225, 62 }, { 171, 311 }, { 162, 2 } };

ConfigPoint g_aChampGridCells[50] = {
    { 245, 94 }, { 309, 94 }, { 382, 94 }, { 447, 94 }, { 510, 94 }, { 245, 115 }, { 309, 115 }, { 382, 115 }, { 447, 115 }, { 510, 115 }, { 245, 136 }, { 309, 136 }, { 382, 136 }, { 447, 136 }, { 510, 136 }, { 245, 157 }, { 309, 157 }, { 382, 157 }, { 447, 157 }, { 510, 157 }, { 245, 178 }, { 309, 178 }, { 382, 178 }, { 447, 178 }, { 510, 178 }, { 245, 199 }, { 309, 199 }, { 382, 199 }, { 447, 199 }, { 510, 199 }, { 245, 220 }, { 309, 220 }, { 382, 220 }, { 447, 220 }, { 510, 220 }, { 245, 241 }, { 309, 241 }, { 382, 241 }, { 447, 241 }, { 510, 241 }, { 245, 262 }, { 309, 262 }, { 382, 262 }, { 447, 262 }, { 510, 262 }, { 245, 283 }, { 309, 283 }, { 382, 283 }, { 447, 283 }, { 510, 283 }
};

ConfigPoint g_aChampBracketCells[13] = {
    { 44, 294 }, { 183, 294 }, { 323, 294 }, { 464, 294 }, { 112, 249 }, { 253, 249 }, { 392, 249 }, { 184, 204 }, { 323, 204 }, { 184, 159 }, { 323, 159 }, { 253, 114 }, { 253, 69 }
};

// STRING: HYPERX 0x004c4180
const char* g_pszTournamentBackground = "tournbk";

// Fourteen teams, not thirteen rounds.
TorchBitmaps g_aTorchBitmaps[14] = {
    { "torchi1", "torchi2", "torchi3" },
    { "torhou1", "torhou2", "torhou3" },
    { "torjam1", "torsta2", "torjam3" },
    { "torveg1", "torveg2", "torveg3" },
    { "torlnd1", "torlnd2", "torlnd3" },
    { "torlos1", "torlos2", "torlos3" },
    { "tormos1", "tormos2", "tormos3" },
    { "tormun1", "tormun2", "tormun3" },
    { "tornem1", "tornem2", "tornem3" },
    { "tornew1", "tornew2", "tornew3" },
    { "torsea1", "torsea2", "torsea3" },
    { "tortok1", "tortok2", "tortok3" },
    { "tortor1", "tortor2", "tortor3" },
    { "torwar1", "torwar2", "torwar3" }
};
