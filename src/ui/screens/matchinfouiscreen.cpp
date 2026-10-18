#include "common/compat.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "ui/uiglobals.h"
#include "ui/screen.h"
class MatchInfoUIScreen;
#include "ui/widget.h"
#include "ui/screens/matchinfouiscreen.h"
#include "engine/game.h"

#define MATCHINFO_LAST_WIDGET UI_WIDGET_PAGE
constexpr auto kMatchinfoFirstPeriod = 1;

// RegisterBitmaps claims every bitmap name's STRING marker.
// FUNCTION: HYPERX 0x00435790
MatchInfoUIScreen::MatchInfoUIScreen()
{
    int nFound;

    m_nLastWidget = MATCHINFO_LAST_WIDGET;

    if (g_nMatchInfoBitmapsUnloaded == 1)
    {
        RegisterBitmaps();
    }

    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_FIRST + 4]->SetHidden(1);
    m_apWidgets[UI_WIDGET_OKAY]->SetHidden(1);
    m_apWidgets[UI_WIDGET_CANCEL]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 1]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 2]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_FIRST + 3]->SetHidden(1);
    m_apWidgets[UI_WIDGET_LEFT_ARROW]->SetHidden(1);
    m_apWidgets[UI_WIDGET_RIGHT_ARROW]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE_TAB_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_CONFIRM_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_STAT_PLATE]->SetHidden(1);
    m_apWidgets[UI_WIDGET_PAGE]->SetHidden(1);

    m_apWidgets[MATCHINFO_WIDGET_INFO]->ClearFrames();

    auto* pBitmap = CacheLookup("info", &nFound);
    m_apWidgets[MATCHINFO_WIDGET_INFO]->AddFrameBitmap(pBitmap, nFound);

    pBitmap = CacheLookup("infor", &nFound);
    m_apWidgets[MATCHINFO_WIDGET_INFO]->AddFrameBitmap(pBitmap, nFound);

    pBitmap = CacheLookup("infod", &nFound);
    m_apWidgets[MATCHINFO_WIDGET_INFO]->AddFrameBitmap(pBitmap, nFound);

    m_apWidgets[MATCHINFO_WIDGET_AWARDS]->ClearFrames();

    pBitmap = CacheLookup("award", &nFound);
    m_apWidgets[MATCHINFO_WIDGET_AWARDS]->AddFrameBitmap(pBitmap, nFound);

    pBitmap = CacheLookup("awardr", &nFound);
    m_apWidgets[MATCHINFO_WIDGET_AWARDS]->AddFrameBitmap(pBitmap, nFound);

    pBitmap = CacheLookup("awardd", &nFound);
    m_apWidgets[MATCHINFO_WIDGET_AWARDS]->AddFrameBitmap(pBitmap, nFound);

    if ((g_pGameEngine->IsTournamentMode() != 0 && g_pGameEngine->m_clock.nPeriod == kMatchinfoFirstPeriod) || (g_pGameEngine->IsTournamentMode() != 0 && g_nPrevEngineState == kEstateDromeExterior))
    {
        m_apWidgets[MATCHINFO_WIDGET_SAVE_LOAD]->ClearFrames();

        // STRING: HYPERX 0x004bdddc
        pBitmap = CacheLookup("savlod", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_SAVE_LOAD]->AddFrameBitmap(pBitmap, nFound);

        // STRING: HYPERX 0x004bddd4
        pBitmap = CacheLookup("savlodr", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_SAVE_LOAD]->AddFrameBitmap(pBitmap, nFound);

        // STRING: HYPERX 0x004bddcc
        pBitmap = CacheLookup("savlodd", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_SAVE_LOAD]->AddFrameBitmap(pBitmap, nFound);

        m_apWidgets[MATCHINFO_WIDGET_KIOSK_DONE]->ClearFrames();

        pBitmap = CacheLookup("done", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_KIOSK_DONE]->AddFrameBitmap(pBitmap, nFound);

        pBitmap = CacheLookup("doner", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_KIOSK_DONE]->AddFrameBitmap(pBitmap, nFound);

        pBitmap = CacheLookup("doned", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_KIOSK_DONE]->AddFrameBitmap(pBitmap, nFound);
    }
    else
    {
        m_apWidgets[MATCHINFO_WIDGET_KIOSK_DONE]->SetHidden(1);

        m_apWidgets[MATCHINFO_WIDGET_DONE]->ClearFrames();

        pBitmap = CacheLookup("done", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_DONE]->AddFrameBitmap(pBitmap, nFound);

        pBitmap = CacheLookup("doner", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_DONE]->AddFrameBitmap(pBitmap, nFound);

        pBitmap = CacheLookup("doned", &nFound);
        m_apWidgets[MATCHINFO_WIDGET_DONE]->AddFrameBitmap(pBitmap, nFound);
    }
}

// FUNCTION: HYPERX 0x00435b20
void MatchInfoUIScreen::OnFirstTabClick()
{
    g_nSetupReturnState = kEstateMatchInfo;

    switch (g_nPlayerMode)
    {
        case kPlayerModeExhibition:
            Game::SetEngineState(kEstateLeagueSetup);
            break;

        case kPlayerModeTournament:
            Game::SetEngineState(kEstateChampionshipSetup);
            break;
    }
}

// FUNCTION: HYPERX 0x00435b50
void MatchInfoUIScreen::OnSecondTabClick()
{
    g_nSetupReturnState = kEstateMatchInfo;

    Game::SetEngineState(kEstateAwards);
}

// FUNCTION: HYPERX 0x00435b70
void MatchInfoUIScreen::OnThirdTabClick()
{
    if ((g_pGameEngine->IsTournamentMode() != 0 && g_pGameEngine->m_clock.nPeriod == kMatchinfoFirstPeriod) || (g_pGameEngine->IsTournamentMode() != 0 && g_nPrevEngineState == kEstateDromeExterior))
    {
        UnloadBitmaps();

        Game::SetEngineState(kEstateSaveLoad);

        return;
    }

    UnloadBitmaps();

    Game::SetEngineState(m_nNextState);
}

// FUNCTION: HYPERX 0x00435bd0
void MatchInfoUIScreen::OnExitTabClick()
{
    UnloadBitmaps();

    Game::SetEngineState(m_nNextState);
}

// Nine images in three states each: plain, "r" rollover and "d" depressed.
// FUNCTION: HYPERX 0x00435bf0
void MatchInfoUIScreen::RegisterBitmaps()
{
    // STRING: HYPERX 0x004bde0c
    AddBitmap("info");

    // STRING: HYPERX 0x004bde04
    AddBitmap("infor");

    // STRING: HYPERX 0x004bddfc
    AddBitmap("infod");

    // STRING: HYPERX 0x004bddf4
    AddBitmap("award");

    // STRING: HYPERX 0x004bddec
    AddBitmap("awardr");

    // STRING: HYPERX 0x004bdde4
    AddBitmap("awardd");

    // STRING: HYPERX 0x004bddc4
    AddBitmap("done");

    // STRING: HYPERX 0x004bddbc
    AddBitmap("doner");

    // STRING: HYPERX 0x004bddb4
    AddBitmap("doned");

    // STRING: HYPERX 0x004bde9c
    AddBitmap("roster");

    // STRING: HYPERX 0x004bde94
    AddBitmap("rosterr");

    // STRING: HYPERX 0x004bde8c
    AddBitmap("rosterd");

    // STRING: HYPERX 0x004bde84
    AddBitmap("gamob1");

    // STRING: HYPERX 0x004bde7c
    AddBitmap("gamob1r");

    // STRING: HYPERX 0x004bde74
    AddBitmap("gamob1d");

    AddBitmap("Arena");

    AddBitmap("Arenar");

    AddBitmap("Arenad");

    AddBitmap("Strite");

    AddBitmap("Striter");

    AddBitmap("Strited");

    AddBitmap("Stleft");

    AddBitmap("Stleftr");

    AddBitmap("Stleftd");

    // STRING: HYPERX 0x004bde24
    AddBitmap("gamob2");

    // STRING: HYPERX 0x004bde1c
    AddBitmap("gamob2r");

    // STRING: HYPERX 0x004bde14
    AddBitmap("gamob2d");
    g_nMatchInfoBitmapsUnloaded = 0;
}

// FUNCTION: HYPERX 0x00435d50
void MatchInfoUIScreen::UnloadBitmaps()
{
    // STRING: HYPERX 0x004bde0c
    RemoveBitmap("info");

    // STRING: HYPERX 0x004bde04
    RemoveBitmap("infor");

    // STRING: HYPERX 0x004bddfc
    RemoveBitmap("infod");

    // STRING: HYPERX 0x004bddf4
    RemoveBitmap("award");

    // STRING: HYPERX 0x004bddec
    RemoveBitmap("awardr");

    // STRING: HYPERX 0x004bdde4
    RemoveBitmap("awardd");

    // STRING: HYPERX 0x004bddc4
    RemoveBitmap("done");

    // STRING: HYPERX 0x004bddbc
    RemoveBitmap("doner");

    // STRING: HYPERX 0x004bddb4
    RemoveBitmap("doned");

    // STRING: HYPERX 0x004bde9c
    RemoveBitmap("roster");

    // STRING: HYPERX 0x004bde94
    RemoveBitmap("rosterr");

    // STRING: HYPERX 0x004bde8c
    RemoveBitmap("rosterd");

    // STRING: HYPERX 0x004bde84
    RemoveBitmap("gamob1");

    // STRING: HYPERX 0x004bde7c
    RemoveBitmap("gamob1r");

    // STRING: HYPERX 0x004bde74
    RemoveBitmap("gamob1d");

    RemoveBitmap("Arena");

    RemoveBitmap("Arenar");

    RemoveBitmap("Arenad");

    RemoveBitmap("Strite");

    RemoveBitmap("Striter");

    RemoveBitmap("Strited");

    RemoveBitmap("Stleft");

    RemoveBitmap("Stleftr");

    RemoveBitmap("Stleftd");

    // STRING: HYPERX 0x004bde24
    RemoveBitmap("gamob2");

    // STRING: HYPERX 0x004bde1c
    RemoveBitmap("gamob2r");

    // STRING: HYPERX 0x004bde14
    RemoveBitmap("gamob2d");
    g_nMatchInfoBitmapsUnloaded = 1;
}

int g_nMatchInfoBitmapsUnloaded = 1;
