#include "engine/gameclips.h"
#include "engine/gameengine.h"
#include "engine/teamblock.h"
#include <format>
#include <optional>

namespace
{

// Two seconds of the celebration after the goal.
constexpr auto kTicksAfterGoal = 60;

// ponytail: each clip holds eight seconds of every bone, a few megabytes; keep the latest ten.
// Store clips compressed or on disk if a match ever needs more.
constexpr auto kMaxClips = 10;

struct Pending
{
    std::string strTitle;
    int         nTicksLeft = 0;
};

auto s_clips   = std::vector<GameClips::Clip>();
auto s_pending = std::optional<Pending>();
auto s_nGoals  = 0;

} // namespace

void GameClips::OnGoal(int nScorerSlot)
{
    const auto& team = [&]() -> const TeamBlock&
    {
        if ((nScorerSlot & 1) != 0)
        {
            return *g_pGameEngine->m_pTeamB;
        }

        return *g_pGameEngine->m_pTeamA;
    }();

    s_nGoals++;
    s_pending = Pending{ std::format("Goal {}: {}, period {}", s_nGoals, team.GetName(), g_pGameEngine->m_clock.nPeriod), kTicksAfterGoal };
}

void GameClips::Tick()
{
    if (!s_pending || --s_pending->nTicksLeft > 0)
    {
        return;
    }

    auto frames = FramePacing::RecentFrames();

    // A simulation draws nothing, so it has nothing to keep.
    if (!frames.empty())
    {
        if (s_clips.size() == kMaxClips)
        {
            s_clips.erase(s_clips.begin());
        }

        s_clips.push_back(Clip{ std::move(s_pending->strTitle), std::move(frames) });
    }

    s_pending.reset();
}

void GameClips::Clear()
{
    s_clips.clear();
    s_pending.reset();
    s_nGoals = 0;
}

const std::vector<GameClips::Clip>& GameClips::All()
{
    return s_clips;
}
