#include "engine/timeout.h"
#include "common/stringtable.h"
#include "engine/gameengine.h"
#include "engine/messagequeue.h"
#include "engine/objectclasses.h"
#include "engine/playerobject.h"
#include "engine/statids.h"
#include "globals.h"
#include "planvm/damagegate.h"
#include <algorithm>
#include <array>

namespace
{

constexpr auto kStringTimeOut = 0x4a;

// How long the rest counts for, on the bench's recovery clock.
constexpr auto kRestMs = 20000.0f;

constexpr auto kMessageMs = 2000;

std::array<bool, 2> s_abUsed = {};

} // namespace

bool TimeOut::Call(int nTeam)
{
    if (g_pGameEngine == nullptr || g_pGameEngine->m_clock.nPauseMark == 1 || s_abUsed[static_cast<size_t>(nTeam)])
    {
        return false;
    }

    s_abUsed[static_cast<size_t>(nTeam)] = true;

    // A reserve's rate, from GameEngine::SubstitutePlayer.
    const auto flRested = kRestMs / (13500.0f / DamageGate());

    for (int nSlot = nTeam; nSlot < kPlayerSlotCount; nSlot += 2)
    {
        auto& flHealth = g_pGameEngine->GetPlayer(nSlot)->m_stats.m_aStats[RATING_SLOT(kRatingDamage)];

        flHealth = (std::min)(flHealth + flRested, 100.0f);
    }

    g_pMessageQueue->Push(StringTable::Find(kStringTimeOut), 0, kMessageMs);

    return true;
}

void TimeOut::Reset()
{
    s_abUsed = {};
}
