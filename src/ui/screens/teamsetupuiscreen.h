#ifndef HYPERBLADE_TEAMSETUPUISCREEN_H
#define HYPERBLADE_TEAMSETUPUISCREEN_H

#include "common/compat.h"
#include "ui/screens/matchinfouiscreen.h"

// The button row as both setup screens fill it, and the widgets each appends in the same order.
enum TeamSetupWidget
{
    TEAMSETUP_WIDGET_ROSTER             = 8,
    TEAMSETUP_WIDGET_STATS              = 9,
    TEAMSETUP_WIDGET_ARENA              = 10,
    TEAMSETUP_WIDGET_GAME_OBJECTS       = 11,
    TEAMSETUP_WIDGET_OBJECT_ANIMATION   = 23,
    TEAMSETUP_WIDGET_OBJECT_TITLE       = 24,
    TEAMSETUP_WIDGET_OBJECT_TEXT        = 25,
    TEAMSETUP_WIDGET_TEAM_TITLE         = 26,
    TEAMSETUP_WIDGET_DEFENDER_CHART     = 27,
    TEAMSETUP_WIDGET_STRIKER_CHART      = 28,
    TEAMSETUP_WIDGET_STATS_PLAYER_NAMES = 29,
    TEAMSETUP_WIDGET_STATS_TEAM_TITLE   = 30
};

// Owns 0x478 to 0x493; it declares no virtual, so neither setup screen's slot order moves.
class TeamSetupUIScreen : public MatchInfoUIScreen
{
public:
    void StepHomeSelection(int nDirection);
    void StepArenaSelection(int nDirection);

    // A remainder means the selection is between teams, and the bitmaps are hidden.
    int m_nTeamSlot;   // 0x478
    int m_nArenaCount; // 0x47c
    // Not scaled by four.
    int m_nArena;            // 0x480
    int m_nUnusedE;          // 0x484, fourteen, the same figure as the obstacle set count
    int m_nObstacleSet;      // 0x488
    int m_nObstacleSetCount; // 0x48c
    int m_nTeamSlotCount;    // 0x490
};

#endif
