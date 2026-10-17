#ifndef HYPERBLADE_LEAGUESETUPUISCREEN_H
#define HYPERBLADE_LEAGUESETUPUISCREEN_H

#include "ui/screens/teamsetupuiscreen.h"

#include "ui/buttonwidget.h"
class TeamSetupUIScreen;

// The stats page: a row per statistic, a column per player and a total.
enum LeagueSetupWidget
{
    LEAGUE_WIDGET_STAT_GRID = 31
};

// The constructor at 0x00452f80 calls MatchInfoUIScreen's at 0x00435790.
// VTABLE: HYPERX 0x004b3370
class LeagueSetupUIScreen : public TeamSetupUIScreen
{
public:
    LeagueSetupUIScreen();

    void RefreshObjectBitmaps(int nBuild);

    void RefreshSelection() override;
    void RefreshFrames() override;
    // Everything UIScreen::OnWidgetMessage's jump table does not answer by identifier.
    void OnOtherButton(int nMsg, unsigned int nId) override;

    // The linker folded this empty body onto Stub::Assert, which carries the marker.
    void OnFirstTabClick() override
    {
    }

    void OnExitTabClick() override;
    void OnFirstPageTabClick() override;
    void OnSecondPageTabClick() override;
    void OnThirdPageTabClick() override;
    void OnFourthPageTabClick() override;
    void OnLeftArrowClick() override;
    void OnRightArrowClick() override;

    void HideStatsPanel();

    void RefreshTeamDisplay();
    void UpdateStatText();
    void LoadTeamBitmaps(int nShow);

private:
    char m_bShowHomeTeam; // 0x494, set for the home team's stats, clear for the away team's
};

#endif
