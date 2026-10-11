#ifndef HYPERBLADE_MATCHSTATS_H
#define HYPERBLADE_MATCHSTATS_H

#include "engine/statids.h"

// 49 floats indexed by StatID; a different block from the ObjectStats ratings at 0xd20.
class MatchStats
{
public:
    MatchStats();

    ~MatchStats()
    {
    }

    // Zeroes all forty nine and answers with itself.
    MatchStats* ZeroInit();

    float m_aStat[MATCH_STAT_COUNT]; // 0x000

    float GetAt(int nIndex) const;
    float SetAt(int nIndex, float flValue);

    // Most ids bump only their own slot; a few recompute a percentage or close an interval.
    float AccumulateStat(int nStatId, int nSlot);

    float StartTimer(int nIndex);
    float StopTimer(int nIndex);

    // One stat as a percentage of another.
    float Percent(int nPart, int nWhole) const
    {
        return (m_aStat[nPart] / m_aStat[nWhole]) * 100.0f;
    }
};

inline MatchStats::MatchStats()
{
    ZeroInit();
}

#endif
