#ifndef HYPERBLADE_POSSESSIONSYNCWORD_H
#define HYPERBLADE_POSSESSIONSYNCWORD_H

// A dromer carries the Rok.
constexpr auto POSSESSION_SYNC_DROMER_CARRIER = 0x80000000;

// MSG_SYNC_POSSESSION's argument: one team row's possession time in hundreds and shots on goal,
// the row, and the team.
struct PossessionSyncWord
{
    unsigned int nWord;

    unsigned int GetAwayTeam() const
    {
        return nWord & 0x40000000;
    }

    int GetTeam() const
    {
        return (nWord >> 30) & 1;
    }

    int GetRow() const
    {
        return (nWord >> 28) & 3;
    }

    int GetShotsOnGoal() const
    {
        return (nWord >> 16) & 0xff;
    }

    unsigned int GetPossessionTime() const
    {
        return nWord & 0xffff;
    }

    static int PackTeam(int nTeam)
    {
        return nTeam << 30;
    }

    // The row and its two stats.
    static int Pack(int nRow, int nShotsOnGoal, unsigned int nPossessionTime)
    {
        return (((nShotsOnGoal & 0xff) | (nRow << 12)) << 16) | static_cast<int>(nPossessionTime & 0xffff);
    }
};

#endif
