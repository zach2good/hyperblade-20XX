#ifndef HYPERBLADE_HEADTOHEADSETUPUISCREEN_H
#define HYPERBLADE_HEADTOHEADSETUPUISCREEN_H

#include "common/compat.h"
#include "globals.h"
#include "engine/game.h"
#include "ui/uiscreen.h"

// UIScreen::InitWidgets builds slots 0 to 22; the constructor appends the rest in order.
// m_nSelectionMode holds the column button last pressed: home team, arena or visitor team.
enum HeadToHeadSetupWidgetId
{
    H2H_HOME_TEAM_BUTTON    = UI_WIDGET_PAGE_TAB_FIRST,
    H2H_ARENA_BUTTON        = UI_WIDGET_PAGE_TAB_FIRST + 1,
    H2H_VISITOR_TEAM_BUTTON = UI_WIDGET_PAGE_TAB_FIRST + 2,
    // The chosen drome's plan in the arena column, the player backdrop in the team columns.
    H2H_VIEW_PANEL       = UI_WIDGET_PAGE,
    H2H_PLAYER_PORTRAITS = UI_WIDGET_OWN_FIRST,
    H2H_TEAM_NAME        = 24,
    // The starters' or the scrubs' ratings, whichever tab was last pressed.
    H2H_RATINGS_CARD = 25,
    H2H_STARTERS_TAB = 26,
    H2H_SCRUBS_TAB   = 27,
    H2H_TEAM_TEXT    = 28,
    H2H_TEAM_LOGO    = 29,
    H2H_LAST_WIDGET  = 29
};

// VTABLE: HYPERX 0x004b3190
class HeadToHeadSetupUIScreen : public UIScreen
{
public:
    void OnFirstPageTabClick() override;
    void OnSecondPageTabClick() override;
    void OnThirdPageTabClick() override;

    // FUNCTION: HYPERX 0x00444b30
    void OnOkayClick() override
    {
        Game::SetEngineState(g_nSetupReturnState);
    }

    // The linker folded this body onto OnOkayClick, which carries the marker.
    void OnCancelClick() override
    {
        Game::SetEngineState(g_nSetupReturnState);
    }

    void OnLeftArrowClick() override;
    void OnRightArrowClick() override;
    void RefreshSelection() override;
    void RefreshFrames() override;
    void OnOtherButton(int nMsg, unsigned int nId) override;
    HeadToHeadSetupUIScreen();
    ~HeadToHeadSetupUIScreen() override;

    void LoadTeamPlayerBitmaps(int nTeam);
    void LoadTeamInfoPanel(int nTeam);

    void StepHomeSelection(int nStep);
    void StepAwaySelection(int nStep);
    void StepArenaSelection(int nStep);

    void ShowStartersCard();

    void ShowScrubsCard();

private:
    int m_nTeamCount;      // 0x474
    int m_nSelection;      // 0x478
    int m_nSelectionLimit; // 0x47c
};

// SYNTHETIC: HYPERX 0x004441a0
// HeadToHeadSetupUIScreen::`scalar deleting destructor'

#endif
