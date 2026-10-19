#ifndef HYPERBLADE_CHAMPIONSHIPSETUPUISCREEN_H
#define HYPERBLADE_CHAMPIONSHIPSETUPUISCREEN_H

#include "common/compat.h"
#include "ui/screens/teamsetupuiscreen.h"

class ButtonWidget;

enum ChampionshipSetupWidget
{
    CHAMP_WIDGET_TREE = 12,
    // The current round's torch, beside the bracket.
    CHAMP_WIDGET_ROUND_TORCH = 31,
    // The stats page: a row per statistic, a column per player and a total.
    CHAMP_WIDGET_STAT_GRID = 32
};

// The constructor at 0x00457710 calls MatchInfoUIScreen's at 0x00435790.
// VTABLE: HYPERX 0x004b33e4
class ChampionshipSetupUIScreen : public TeamSetupUIScreen
{
public:
    void RefreshSelection() override;
    void RefreshFrames() override;
    // Everything UIScreen::OnWidgetMessage's jump table does not answer by identifier.
    void OnOtherButton(int nMsg, unsigned int nId) override;
    void RefreshTabs() override;

    // The linker folded this empty body onto Stub::Assert, which carries the marker.
    void OnFirstTabClick() override
    {
    }

    void OnExitTabClick() override;
    void OnFirstPageTabClick() override;
    void OnSecondPageTabClick() override;
    void OnThirdPageTabClick() override;
    void OnFourthPageTabClick() override;
    void OnFifthPageTabClick() override;
    void OnLeftArrowClick() override;
    void OnRightArrowClick() override;

    ChampionshipSetupUIScreen();

    void LoadBracketTorches();

    void HideBracketTorches();
    void HideStatsPanel();

    // Home, away, home career and away career, in that order.
    void StepStatsView(int nStep);
    void RefreshStatsDisplay();
    void UpdatePlayerRows();
    void LoadTeamBitmaps(int nShow);

    void LoadObstacleFrames(int nBuild);

private:
    int  m_nUnusedF;      // 0x494
    int  m_nUnusedG;      // 0x498
    char m_bShowHomeTeam; // 0x49c, set for the home team's stats, clear for the away team's

    // Set while the grid shows the totals kept across Gauntlet matches.
    char    m_bShowCareer  = 0;
    Widget* m_pCareerLabel = nullptr;
    int     m_nLastGridWidget; // 0x4a0
    char    m_bTorchesLoaded;  // 0x4a4, set to 1 by championshipsetupuiscreen.cpp, cleared as a byte
private:
    // Coming from the jumbotron makes the constructor restore the bracket.
    int m_nPrevState; // 0x4a8
};

#endif
