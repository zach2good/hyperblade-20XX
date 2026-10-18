// The match stat array: the block cleared, one element read or written, and the plan's stat updates.

#include "common/compat.h"
#include "engine/matchstats.h"
#include "objectclasses.h"
#include "globals.h"
#include "engineglobals.h"
#include "gameengine.h"
#include "engine/statids.h"
#include <cstring>

// FUNCTION: HYPERX 0x00491880
MatchStats* MatchStats::ZeroInit()
{
    memset(m_aStat, 0, sizeof(m_aStat));

    return this;
}

// FUNCTION: HYPERX 0x004918a0
float MatchStats::GetAt(int nIndex) const
{
    return m_aStat[nIndex];
}

// FUNCTION: HYPERX 0x004918b0
float MatchStats::SetAt(int nIndex, float flValue)
{
    m_aStat[nIndex] = flValue;

    return flValue;
}

// Open from kMatchStatPossStart to kMatchStatPossStop; src/globals.cpp defines it.
extern char g_bStatIntervalOpen;

// FUNCTION: HYPERX 0x004918d0
float MatchStats::AccumulateStat(int nStatId, int)
{
    switch (nStatId)
    {
        case kMatchStat1xGoal:
            m_aStat[kMatchStat1xGoal]++;
            m_aStat[kMatchStatTotalGoals]++;
            m_aStat[kMatchStatTotalPoints]++;
            m_aStat[kMatchStatShootingPct] =
                (m_aStat[kMatchStatTotalGoals] / m_aStat[kMatchStatShotsOnGoal]) * 100.0f;

            break;

        case kMatchStat2xGoal:
            m_aStat[kMatchStat2xGoal]++;
            m_aStat[kMatchStatTotalGoals]++;
            m_aStat[kMatchStatTotalPoints] += 2.0f;
            m_aStat[kMatchStatShootingPct] =
                (m_aStat[kMatchStatTotalGoals] / m_aStat[kMatchStatShotsOnGoal]) * 100.0f;

            break;

        case kMatchStat4xGoal:
            m_aStat[kMatchStat4xGoal]++;
            m_aStat[kMatchStatTotalGoals]++;
            m_aStat[kMatchStatTotalPoints] += 4.0f;
            m_aStat[kMatchStatShootingPct] =
                (m_aStat[kMatchStatTotalGoals] / m_aStat[kMatchStatShotsOnGoal]) * 100.0f;

            break;

        case kMatchStatShotsOnGoal:
            m_aStat[kMatchStatShotsOnGoal]++;
            m_aStat[kMatchStatShootingPct] =
                (m_aStat[kMatchStatTotalGoals] / m_aStat[kMatchStatShotsOnGoal]) * 100.0f;

            break;

        case kMatchStatPossStart:
            m_aStat[kMatchStatPossStart] = static_cast<float>(g_pGameEngine->m_clock.nTickMs);

            g_bStatIntervalOpen = 1;

            break;

        case kMatchStatPossStop:
            if (g_bStatIntervalOpen != 0)
            {
                const auto flNow = static_cast<float>(g_pGameEngine->m_clock.nTickMs);

                m_aStat[kMatchStatPossStop] = flNow;
                m_aStat[kMatchStatTimeOfPoss] =
                    (flNow - m_aStat[kMatchStatPossStart]) + m_aStat[kMatchStatTimeOfPoss];

                g_bStatIntervalOpen = 0;

                break;
            }

            break;

        case kMatchStatRandomizers:
            m_aStat[kMatchStatRandomizers]++;

            break;

        case kMatchStatKillballs:
            m_aStat[kMatchStatKillballs]++;

            break;

        case kMatchStatInjuries:
            m_aStat[kMatchStatInjuries]++;

            break;

        case kMatchStatFatalities:
            m_aStat[kMatchStatFatalities]++;

            break;

        case kMatchStatAssists:
            if (static_cast<float>(g_pGameEngine->m_clock.nTickMs) - 15000.0f < m_aStat[kMatchStatTopStampEnd])
            {
                m_aStat[kMatchStatAssists]++;
            }

            if (static_cast<float>(g_pGameEngine->m_clock.nTickMs) - 1000.0f < m_aStat[kMatchStatTopStampEnd])
            {
                m_aStat[kMatchStatOneTimers]++;

                break;
            }

            break;

        case kMatchStatGoals:
            m_aStat[kMatchStatGoals]++;

            m_aStat[kMatchStatShooting] = Percent(kMatchStatGoals, kMatchStatShots);

            break;

        case kMatchStatPassesCompleted:
            m_aStat[kMatchStatPassesCompleted]++;

            m_aStat[kMatchStatPassing] =
                Percent(kMatchStatPassesCompleted, kMatchStatPassesAttempted);

            break;

        case kMatchStat2PointCompleted:
            m_aStat[kMatchStat2PointCompleted]++;

            m_aStat[kMatchStat2PointScores] =
                Percent(kMatchStatPassesCompleted, kMatchStatPassesAttempted);

            break;

        case kMatchStatShortHandedCompleted:
            m_aStat[kMatchStatShortHandedCompleted]++;

            m_aStat[kMatchStat2PointScores] =
                Percent(kMatchStatShortHandedCompleted, kMatchStatShortHandedAttempts);

            break;

        case kMatchStatPowerPlayCompleted:
            m_aStat[kMatchStatPowerPlayCompleted]++;

            m_aStat[kMatchStat2PointScores] =
                Percent(kMatchStatPowerPlayCompleted, kMatchStatPowerPlayAttempts);

            break;

        default:
            m_aStat[nStatId]++;

            break;
    }

    return m_aStat[nStatId];
}

// FUNCTION: HYPERX 0x00491c90
float MatchStats::StartTimer(int nIndex)
{
    if (m_aStat[nIndex] == 0.0f)
    {
        m_aStat[nIndex] = static_cast<float>(g_pGameEngine->m_clock.nTickMs);
    }

    return m_aStat[nIndex];
}

// Every index records the clock; four also close an interval into a running total.

// FUNCTION: HYPERX 0x00491cd0
float MatchStats::StopTimer(int nIndex)
{
    const auto flNow = static_cast<float>(g_pGameEngine->m_clock.nTickMs);

    m_aStat[nIndex] = flNow;

    switch (nIndex)
    {
        case kMatchStatTopStampEnd:
            m_aStat[kMatchStatTimeOfPossession] =
                m_aStat[kMatchStatTopStampEnd] - m_aStat[kMatchStatTopStampStart] + m_aStat[kMatchStatTimeOfPossession];
            m_aStat[kMatchStatTopStampStart] = 0.0f;
            return m_aStat[nIndex];

        case kMatchStatTorStampEnd:
            m_aStat[kMatchStatTimeOnRail] =
                m_aStat[kMatchStatTorStampEnd] - m_aStat[kMatchStatTorStampStart] + m_aStat[kMatchStatTimeOnRail];
            m_aStat[kMatchStatTorStampStart] = 0.0f;
            return m_aStat[nIndex];

        case kMatchStatHtStampEnd:
            m_aStat[kMatchStatHangTime] =
                m_aStat[kMatchStatHtStampEnd] - m_aStat[kMatchStatHtStampStart] + m_aStat[kMatchStatHangTime];
            m_aStat[kMatchStatHtStampStart] = 0.0f;
            return m_aStat[nIndex];

        case kMatchStatPptStampEnd:
            m_aStat[kMatchStatPowerPlayTime] =
                m_aStat[kMatchStatPptStampEnd] - m_aStat[kMatchStatPptStampStart] + m_aStat[kMatchStatPowerPlayTime];
            m_aStat[kMatchStatPptStampStart] = 0.0f;
            return m_aStat[nIndex];

        default:
            break;
    }

    return flNow;
}
