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
    virtual void RefreshSelection();
    virtual void RefreshFrames();
    // Everything UIScreen::OnWidgetMessage's jump table does not answer by identifier.
    virtual void OnOtherButton(int nMsg, unsigned int nId);
    virtual void RefreshTabs();

    // The linker folded this empty body onto Stub::Assert, which carries the marker.
    virtual void OnFirstTabClick()
    {
    }

    virtual void OnExitTabClick();
    virtual void OnFirstPageTabClick();
    virtual void OnSecondPageTabClick();
    virtual void OnThirdPageTabClick();
    virtual void OnFourthPageTabClick();
    virtual void OnFifthPageTabClick();
    virtual void OnLeftArrowClick();
    virtual void OnRightArrowClick();

    ChampionshipSetupUIScreen();

    void LoadBracketTorches();

    void HideBracketTorches();
    void HideStatsPanel();
    void RefreshStatsDisplay();
    void UpdatePlayerRows();
    void LoadTeamBitmaps(int nShow);

    void LoadObstacleFrames(int nBuild);

private:
    int  m_nUnusedF;        // 0x494
    int  m_nUnusedG;        // 0x498
    char m_bShowHomeTeam;   // 0x49c, set for the home team's stats, clear for the away team's
    int  m_nLastGridWidget; // 0x4a0
    char m_bTorchesLoaded;  // 0x4a4, set to 1 by championshipsetupuiscreen.cpp, cleared as a byte
private:
    // Coming from the jumbotron makes the constructor restore the bracket.
    int m_nPrevState; // 0x4a8
};

#endif
