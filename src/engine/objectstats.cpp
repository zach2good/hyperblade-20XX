#include "math/floatbits.h"
#include "common/compat.h"
#include "globals.h"
#include "engineglobals.h"
#include "objectstats.h"
#include "common/staticinit.h"
#include "gameengine.h"
#include "ui/torchbitmaps.h"
#include "ui/saveobject.h"
#include "engine/statids.h"
#include "engine/objectclasses.h"

// The energy switch the physics dialog toggles. src/globals.cpp defines it.
extern unsigned char g_bEnergyOn;

// Set while energy drains towards thirty; one flag for every skater.
// GLOBAL: HYPERX 0x004c55d8
extern unsigned char g_bEnergyDraining;

constexpr auto STATS_COUNT = 0x3f;

constexpr auto SCALE_ONE_BITS = 0x3f800000;

// Everything is five except the first and seventeenth, a hundred, and the sixth, seven.

// FUNCTION: HYPERX 0x004748e0
ObjectStats* ObjectStats::InitDefaults()
{
    int i;

    m_aStats[0] = 100.0f;

    for (i = 1; i < 64; i++)
    {
        m_aStats[i] = 5.0f;
    }

    m_aStats[RATING_SLOT(RATING_SPEED)]  = 7.0f;
    m_aStats[RATING_SLOT(RATING_DAMAGE)] = 100.0f;

    m_flScale      = g_flBaseStatsSentinel;
    m_flSpeedRatio = g_flBaseStatsSentinel;

    return this;
}

// FUNCTION: HYPERX 0x00474930
float ObjectStats::GetWeightedStat(int nIndex)
{
    float flEnergy = m_aStats[0];
    flEnergy *= g_flStatWeightScale;
    flEnergy += 0.25f;

    return flEnergy * m_aStats[nIndex + 1];
}

// The stat index is a word offset from the body's second field, not a table lookup.

// FUNCTION: HYPERX 0x00474950
float ObjectStats::GetScalar(int nWhich) const
{
    return m_aStats[nWhich + 1];
}

// Only eight of the 63 stats are scaled: four cap at twelve and four at fifteen.

// FUNCTION: HYPERX 0x00474960
void ObjectStats::ScaleTo(int nSlot)
{
    float* pStat;
    float  flValue;
    int    nIndex;

    if (nSlot > SLOT_DROMER3)
    {
        return;
    }

    if ((g_bSessionActive == 0) || ((nSlot & 1) == 0) || (FloatBits(m_flScale) != SCALE_ONE_BITS))
    {
        if (g_pGameEngine->IsTournamentMode() != 0)
        {
            m_flScale = (g_pGameEngine->m_pSaveObject->GetBracketPos() - 3.0f) * 0.03f + m_flScale;
            m_flScale = static_cast<float>(g_pGameEngine->m_clock.nPeriod - 1) * 0.05f + m_flScale;
        }
        else
        {
            m_flScale = static_cast<float>(g_pGameEngine->m_clock.nPeriod) * 0.05f + m_flScale;
        }
    }

    if (m_flScale == g_flBaseStatsSentinel)
    {
        return;
    }

    // pStat[STATS_COUNT] is the stat's copy in m_aBackup.
    for (nIndex = 0, pStat = &m_aStats[1]; nIndex < STATS_COUNT; nIndex++, pStat++)
    {
        switch (nIndex)
        {
            case RATING_WEIGHT_CLASS:
            case RATING_STAMINA:
            case RATING_BALANCE:
            case RATING_ROK_HANDLING:
                pStat[STATS_COUNT] = *pStat;

                flValue = *pStat * m_flScale;

                if (12.0f <= flValue || 0.0f <= flValue)
                {
                    if (12.0f < flValue)
                    {
                        flValue = 12.0f;
                    }
                }
                else
                {
                    flValue = 0.0f;
                }

                *pStat = flValue;
                break;

            case RATING_CHECKING:
            case RATING_SPEED:
            case RATING_SHOT_POWER:
            case RATING_OFFENSIVE_ABILITY:
                pStat[STATS_COUNT] = *pStat;

                flValue = *pStat * m_flScale;

                if (15.0f <= flValue || 0.0f <= flValue)
                {
                    if (15.0f < flValue)
                    {
                        flValue = 15.0f;
                    }
                }
                else
                {
                    flValue = 0.0f;
                }

                *pStat = flValue;
                break;
        }
    }
}

// Only eight of the first fourteen are restored; the sentinel in m_flScale marks a restored block.

// FUNCTION: HYPERX 0x00474b30
void ObjectStats::CopyBaseStats()
{
    int i;

    if (m_flScale != g_flBaseStatsSentinel)
    {
        for (i = 0; i < STATS_COUNT; i++)
        {
            switch (i)
            {
                case RATING_WEIGHT_CLASS:
                case RATING_STAMINA:
                case RATING_CHECKING:
                case RATING_BALANCE:
                case RATING_SPEED:
                case RATING_SHOT_POWER:
                case RATING_OFFENSIVE_ABILITY:
                case RATING_ROK_HANDLING:
                    m_aStats[i + 1] = m_aBackup[i];
                    break;

                default:
                    break;
            }
        }
    }

    m_flScale = g_flBaseStatsSentinel;
}

// FUNCTION: HYPERX 0x00474bb0
void ObjectStats::UpdateMomentumAndEnergy()
{
    float flMomentum;
    float flNew;

    if ((flMomentum = (((GetScalar(RATING_STAMINA) - 4.0f) * 0.15f + g_flHalfRokObject) * g_pGameEngine->m_clock.GetDelta() + g_flBaseStatsSentinel) * m_flSpeedRatio) > g_flBaseStatsSentinel)
    {
        flMomentum = g_flBaseStatsSentinel;
    }

    m_flSpeedRatio = flMomentum;

    if (g_bEnergyOn == 0)
    {
        return;
    }

    if (g_bEnergyDraining != 0)
    {
        if (m_aStats[0] < 30.0)
        {
            g_bEnergyDraining = 0;
        }
    }
    else if (m_aStats[0] > 100.0)
    {
        g_bEnergyDraining = 1;
    }

    if (g_bEnergyDraining != 0)
    {
        flNew = m_aStats[0] - (g_flSpeedRateBase - GetScalar(RATING_STAMINA) * 0.15f) * g_pGameEngine->m_clock.GetDelta();

        if (flNew > 100.0)
        {
            flNew = 100.0f;
        }
        else if (flNew < 0.0)
        {
            flNew = g_flZeroRokObject;
        }

        m_aStats[0] = flNew;

        return;
    }

    const float flAccel = g_flSpeedRateBase + g_flHalfRokObject;
    flNew               = (flAccel - GetScalar(RATING_STAMINA) * 0.15f) * g_pGameEngine->m_clock.GetDelta() + m_aStats[0];

    if (flNew > 100.0)
    {
        flNew = 100.0f;
    }
    else if (flNew < 0.0)
    {
        flNew = g_flZeroRokObject;
    }

    m_aStats[0] = flNew;
}

unsigned char g_bEnergyDraining = 1;
