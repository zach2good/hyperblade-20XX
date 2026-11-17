// The game clips the demo's viewer promised: each goal keeps the instant replay's last seconds,
// up to the goal and a little after, for the Game Clips scene to play back.

#ifndef HYPERBLADE_GAMECLIPS_H
#define HYPERBLADE_GAMECLIPS_H

#include "engine/framepacing.h"
#include <string>
#include <vector>

namespace GameClips
{

struct Clip
{
    std::string                           strTitle;
    std::vector<FramePacing::ReplayFrame> frames;
};

// A goal by the dromer in nScorerSlot; the clip is cut a moment later.
void OnGoal(int nScorerSlot);

// Once a tick, to cut a pending clip.
void Tick();

// A new match forgets the last one's clips.
void Clear();

const std::vector<Clip>& All();

} // namespace GameClips

#endif
