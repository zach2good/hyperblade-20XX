#ifndef HYPERBLADE_SYSTEMSYNCWORD_H
#define HYPERBLADE_SYSTEMSYNCWORD_H

// The host's clock is paused.
#define SYSTEM_SYNC_PAUSED 0x80000000

// MSG_SYNC_SYSTEM's argument: the home and away scores, the period's elapsed time in tenths of a
// second, and the pause mark.
struct SystemSyncWord
{
    unsigned int nWord;

    unsigned int GetPauseMark() const
    {
        return nWord & SYSTEM_SYNC_PAUSED;
    }

    int GetElapsed() const
    {
        return (nWord >> 16) & 0x7fff;
    }

    int GetAwayScore() const
    {
        return (nWord >> 8) & 0xff;
    }

    int GetHomeScore() const
    {
        return nWord & 0xff;
    }

    // Every field but the pause mark. Away is last: the client reads it first, right to left.
    static int Pack(int nElapsed, int nHomeScore, int nAwayScore)
    {
        return (((nAwayScore & 0xff) | (nElapsed << 8)) << 8) | (nHomeScore & 0xff);
    }
};

#endif
