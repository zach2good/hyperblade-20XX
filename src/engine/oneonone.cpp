#include "engine/oneonone.h"
#include "engine/engineobject.h"
#include "engine/gameengine.h"
#include "engine/playerobject.h"
#include "engine/statids.h"
#include "engine/teamblock.h"
#include "globals.h"
#include <array>

namespace
{

constexpr auto kFirstParked = 2;
constexpr auto kParkedCount = 2;

bool s_bOn = false;

std::array<bool, kParkedCount> s_abParked = {};

TeamBlock& TeamOf(int nSlot)
{
    if ((nSlot & 1) != 0)
    {
        return *g_pGameEngine->m_pTeamB;
    }

    return *g_pGameEngine->m_pTeamA;
}

} // namespace

bool OneOnOne::On()
{
    return s_bOn;
}

void OneOnOne::Set(bool bOn)
{
    s_bOn = bOn;
}

void OneOnOne::ParkSeconds()
{
    s_abParked = {};

    if (!s_bOn || g_bSessionActive != 0 || g_pGameEngine->IsExhibitionMode() == 0)
    {
        return;
    }

    for (int n = 0; n < kParkedCount; n++)
    {
        const auto nSlot = kFirstParked + n;

        auto* const pPlayer = g_pGameEngine->GetPlayer(nSlot);

        // A human never sits out: the camera may follow either dromer at the start.
        if (nSlot == g_pGameEngine->m_nViewedSlot)
        {
            continue;
        }

        // Set here rather than through ModifyDamage, which the difficulty scales short of nought.
        pPlayer->m_stats.m_aStats[RATING_SLOT(kRatingDamage)] = 0.0f;
        pPlayer->m_nFlags |= kEngineObjectHidden;
        pPlayer->RunPlanFrom("Sideln", "SideLined");

        s_abParked[static_cast<size_t>(n)] = true;
    }
}

bool OneOnOne::Parked(int nSlot)
{
    const auto n = nSlot - kFirstParked;

    return n >= 0 && n < kParkedCount && s_abParked[static_cast<size_t>(n)];
}

void OneOnOne::Unpark()
{
    for (int n = 0; n < kParkedCount; n++)
    {
        if (!s_abParked[static_cast<size_t>(n)])
        {
            continue;
        }

        const auto nSlot = kFirstParked + n;

        auto* const pPlayer = g_pGameEngine->GetPlayer(nSlot);

        pPlayer->m_stats.m_aStats[RATING_SLOT(kRatingDamage)] = 100.0f;
        pPlayer->m_nFlags &= ~kEngineObjectHidden;

        auto& team = TeamOf(nSlot);

        team.aRows[team.aSeat[1]].aStat[kRatingDamage] = 100.0f;
    }

    s_abParked = {};
}
