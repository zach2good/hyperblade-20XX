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

// Open from MATCH_STAT_POSS_START to MATCH_STAT_POSS_STOP; src/globals.cpp defines it.
extern char g_bStatIntervalOpen;

// FUNCTION: HYPERX 0x004918d0
float MatchStats::AccumulateStat(int nStatId, int)
{
    switch (nStatId)
    {
        case MATCH_STAT_1X_GOAL:
            m_aStat[MATCH_STAT_1X_GOAL]++;
            m_aStat[MATCH_STAT_TOTAL_GOALS]++;
            m_aStat[MATCH_STAT_TOTAL_POINTS]++;
            m_aStat[MATCH_STAT_SHOOTING_PCT] =
                (m_aStat[MATCH_STAT_TOTAL_GOALS] / m_aStat[MATCH_STAT_SHOTS_ON_GOAL]) * 100.0f;

            break;

        case MATCH_STAT_2X_GOAL:
            m_aStat[MATCH_STAT_2X_GOAL]++;
            m_aStat[MATCH_STAT_TOTAL_GOALS]++;
            m_aStat[MATCH_STAT_TOTAL_POINTS] += 2.0f;
            m_aStat[MATCH_STAT_SHOOTING_PCT] =
                (m_aStat[MATCH_STAT_TOTAL_GOALS] / m_aStat[MATCH_STAT_SHOTS_ON_GOAL]) * 100.0f;

            break;

        case MATCH_STAT_4X_GOAL:
            m_aStat[MATCH_STAT_4X_GOAL]++;
            m_aStat[MATCH_STAT_TOTAL_GOALS]++;
            m_aStat[MATCH_STAT_TOTAL_POINTS] += 4.0f;
            m_aStat[MATCH_STAT_SHOOTING_PCT] =
                (m_aStat[MATCH_STAT_TOTAL_GOALS] / m_aStat[MATCH_STAT_SHOTS_ON_GOAL]) * 100.0f;

            break;

        case MATCH_STAT_SHOTS_ON_GOAL:
            m_aStat[MATCH_STAT_SHOTS_ON_GOAL]++;
            m_aStat[MATCH_STAT_SHOOTING_PCT] =
                (m_aStat[MATCH_STAT_TOTAL_GOALS] / m_aStat[MATCH_STAT_SHOTS_ON_GOAL]) * 100.0f;

            break;

        case MATCH_STAT_POSS_START:
            m_aStat[MATCH_STAT_POSS_START] = (float)g_pGameEngine->m_clock.nTickMs;

            g_bStatIntervalOpen = 1;

            break;

        case MATCH_STAT_POSS_STOP:
            if (g_bStatIntervalOpen != 0)
            {
                const auto flNow = (float)g_pGameEngine->m_clock.nTickMs;

                m_aStat[MATCH_STAT_POSS_STOP] = flNow;
                m_aStat[MATCH_STAT_TIME_OF_POSS] =
                    (flNow - m_aStat[MATCH_STAT_POSS_START]) + m_aStat[MATCH_STAT_TIME_OF_POSS];

                g_bStatIntervalOpen = 0;

                break;
            }

            break;

        case MATCH_STAT_RANDOMIZERS:
            m_aStat[MATCH_STAT_RANDOMIZERS]++;

            break;

        case MATCH_STAT_KILLBALLS:
            m_aStat[MATCH_STAT_KILLBALLS]++;

            break;

        case MATCH_STAT_INJURIES:
            m_aStat[MATCH_STAT_INJURIES]++;

            break;

        case MATCH_STAT_FATALITIES:
            m_aStat[MATCH_STAT_FATALITIES]++;

            break;

        case MATCH_STAT_ASSISTS:
            if ((float)g_pGameEngine->m_clock.nTickMs - 15000.0f < m_aStat[MATCH_STAT_TOP_STAMP_END])
            {
                m_aStat[MATCH_STAT_ASSISTS]++;
            }

            if ((float)g_pGameEngine->m_clock.nTickMs - 1000.0f < m_aStat[MATCH_STAT_TOP_STAMP_END])
            {
                m_aStat[MATCH_STAT_ONE_TIMERS]++;

                break;
            }

            break;

        case MATCH_STAT_GOALS:
            m_aStat[MATCH_STAT_GOALS]++;

            m_aStat[MATCH_STAT_SHOOTING] = Percent(MATCH_STAT_GOALS, MATCH_STAT_SHOTS);

            break;

        case MATCH_STAT_PASSES_COMPLETED:
            m_aStat[MATCH_STAT_PASSES_COMPLETED]++;

            m_aStat[MATCH_STAT_PASSING] =
                Percent(MATCH_STAT_PASSES_COMPLETED, MATCH_STAT_PASSES_ATTEMPTED);

            break;

        case MATCH_STAT_2_POINT_COMPLETED:
            m_aStat[MATCH_STAT_2_POINT_COMPLETED]++;

            m_aStat[MATCH_STAT_2_POINT_SCORES] =
                Percent(MATCH_STAT_PASSES_COMPLETED, MATCH_STAT_PASSES_ATTEMPTED);

            break;

        case MATCH_STAT_SHORT_HANDED_COMPLETED:
            m_aStat[MATCH_STAT_SHORT_HANDED_COMPLETED]++;

            m_aStat[MATCH_STAT_2_POINT_SCORES] =
                Percent(MATCH_STAT_SHORT_HANDED_COMPLETED, MATCH_STAT_SHORT_HANDED_ATTEMPTS);

            break;

        case MATCH_STAT_POWER_PLAY_COMPLETED:
            m_aStat[MATCH_STAT_POWER_PLAY_COMPLETED]++;

            m_aStat[MATCH_STAT_2_POINT_SCORES] =
                Percent(MATCH_STAT_POWER_PLAY_COMPLETED, MATCH_STAT_POWER_PLAY_ATTEMPTS);

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
        m_aStat[nIndex] = (float)g_pGameEngine->m_clock.nTickMs;
    }

    return m_aStat[nIndex];
}

// Every index records the clock; four also close an interval into a running total.

// FUNCTION: HYPERX 0x00491cd0
float MatchStats::StopTimer(int nIndex)
{
    const auto flNow = (float)g_pGameEngine->m_clock.nTickMs;

    m_aStat[nIndex] = flNow;

    switch (nIndex)
    {
        case MATCH_STAT_TOP_STAMP_END:
            m_aStat[MATCH_STAT_TIME_OF_POSSESSION] =
                m_aStat[MATCH_STAT_TOP_STAMP_END] - m_aStat[MATCH_STAT_TOP_STAMP_START] + m_aStat[MATCH_STAT_TIME_OF_POSSESSION];
            m_aStat[MATCH_STAT_TOP_STAMP_START] = 0.0f;
            return m_aStat[nIndex];

        case MATCH_STAT_TOR_STAMP_END:
            m_aStat[MATCH_STAT_TIME_ON_RAIL] =
                m_aStat[MATCH_STAT_TOR_STAMP_END] - m_aStat[MATCH_STAT_TOR_STAMP_START] + m_aStat[MATCH_STAT_TIME_ON_RAIL];
            m_aStat[MATCH_STAT_TOR_STAMP_START] = 0.0f;
            return m_aStat[nIndex];

        case MATCH_STAT_HT_STAMP_END:
            m_aStat[MATCH_STAT_HANG_TIME] =
                m_aStat[MATCH_STAT_HT_STAMP_END] - m_aStat[MATCH_STAT_HT_STAMP_START] + m_aStat[MATCH_STAT_HANG_TIME];
            m_aStat[MATCH_STAT_HT_STAMP_START] = 0.0f;
            return m_aStat[nIndex];

        case MATCH_STAT_PPT_STAMP_END:
            m_aStat[MATCH_STAT_POWER_PLAY_TIME] =
                m_aStat[MATCH_STAT_PPT_STAMP_END] - m_aStat[MATCH_STAT_PPT_STAMP_START] + m_aStat[MATCH_STAT_POWER_PLAY_TIME];
            m_aStat[MATCH_STAT_PPT_STAMP_START] = 0.0f;
            return m_aStat[nIndex];

        default:
            break;
    }

    return flNow;
}
