#include "engine/oneonone.h"
#include "engine/engineobject.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/playerobject.h"
#include "engine/statids.h"
#include "engine/teamblock.h"
#include "globals.h"
#include <algorithm>
#include <array>

namespace
{

constexpr auto kTeams = 2;

bool s_bOn = false;

// The slot each team sits out this period, or kSlotNone.
std::array<int, kTeams> s_anParked = { kSlotNone, kSlotNone };

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
    s_anParked.fill(kSlotNone);

    if (!s_bOn || g_bSessionActive != 0 || g_pGameEngine->IsExhibitionMode() == 0)
    {
        return;
    }

    for (int nTeam = 0; nTeam < kTeams; nTeam++)
    {
        // The second dromer sits out, or the first when the human is on the second.
        const auto nSlot = [&]
        {
            if (g_pGameEngine->m_nViewedSlot == nTeam + 2)
            {
                return nTeam;
            }

            return nTeam + 2;
        }();

        auto* const pPlayer = g_pGameEngine->GetPlayer(nSlot);

        // Set here rather than through ModifyDamage, which the difficulty scales short of nought.
        pPlayer->m_stats.m_aStats[RATING_SLOT(kRatingDamage)] = 0.0f;
        pPlayer->m_nFlags |= kEngineObjectHidden;
        pPlayer->RunPlanFrom("Sideln", "SideLined");

        s_anParked[static_cast<size_t>(nTeam)] = nSlot;
    }
}

bool OneOnOne::Parked(int nSlot)
{
    return nSlot != kSlotNone && std::ranges::contains(s_anParked, nSlot);
}

void OneOnOne::Unpark()
{
    for (const auto nSlot : s_anParked)
    {
        if (nSlot == kSlotNone)
        {
            continue;
        }

        auto* const pPlayer = g_pGameEngine->GetPlayer(nSlot);

        pPlayer->m_stats.m_aStats[RATING_SLOT(kRatingDamage)] = 100.0f;
        pPlayer->m_nFlags &= ~kEngineObjectHidden;

        auto& team = TeamOf(nSlot);

        // Slots 0 and 1 hold seat 0, slots 2 and 3 seat 1.
        team.aRows[team.aSeat[nSlot >> 1]].aStat[kRatingDamage] = 100.0f;
    }

    s_anParked.fill(kSlotNone);
}
