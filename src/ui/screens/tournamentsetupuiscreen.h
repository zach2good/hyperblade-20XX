#ifndef HYPERBLADE_TOURNAMENTSETUPUISCREEN_H
#define HYPERBLADE_TOURNAMENTSETUPUISCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "engine/game.h"
#include "ui/uiscreen.h"
#include "ui/buttonwidget.h"

// The selected team's page, appended after the chrome.
enum SingleMatchSetupWidget
{
    TOURNAMENT_SETUP_ROSTER = UI_WIDGET_OWN_FIRST,
    TOURNAMENT_SETUP_TEAM_NAME,
    // The starters' or the scrubs' ratings, whichever button is down.
    TOURNAMENT_SETUP_LINEUP_CARD,
    TOURNAMENT_SETUP_STARTERS,
    TOURNAMENT_SETUP_SCRUBS,
    TOURNAMENT_SETUP_TEAM_RATING,
    TOURNAMENT_SETUP_TEAM_EMBLEM
};

// VTABLE: HYPERX 0x004b3fcc
class TournamentSetupUIScreen : public UIScreen
{
public:
    virtual void OnOkayClick();

    // The linker folded this body onto HeadToHeadSetupUIScreen::OnOkayClick, which carries the marker.
    virtual void OnCancelClick()
    {
        Game::SetEngineState(g_nSetupReturnState);
    }

    virtual void OnLeftArrowClick();
    virtual void OnRightArrowClick();
    virtual void RefreshSelection();
    virtual void RefreshFrames();
    virtual void OnOtherButton(int nMsg, unsigned int nId);
    TournamentSetupUIScreen();
    ~TournamentSetupUIScreen();

    void LoadTeamPlayerBitmaps(int nTeam);
    void LoadTeamInfoPanel(int nTeam);
    void ShowStartersCard();
    void ShowScrubsCard();

private:
    int m_nTeamCount;      // 0x474
    int m_nSelection;      // 0x478
    int m_nSelectionLimit; // 0x47c

private:
    int m_anUnused[2];
};

// SYNTHETIC: HYPERX 0x00488410
// TournamentSetupUIScreen::`scalar deleting destructor'

// GLOBAL: HYPERX 0x004ce98c
extern int g_nMatchSetupFlagA;
// GLOBAL: HYPERX 0x004ce990
extern int g_nMatchSetupFlagB;

#endif
