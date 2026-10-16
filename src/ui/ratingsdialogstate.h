// Twenty bytes: player short, category int, value float, and two string arrays.

#ifndef HYPERBLADE_RATINGSDIALOGSTATE_H
#define HYPERBLADE_RATINGSDIALOGSTATE_H

struct RatingsDialogState
{
    short  nPlayer;       // 0x00
    int    nCategory;     // 0x04
    float  flValue;       // 0x08
    char** ppNames;       // 0x0c
    char** ppGoalieNames; // 0x10
};

#endif
