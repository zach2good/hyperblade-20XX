// The simulation steps thirty times a second, and the scene draws as often as the loop runs. A tick
// queues the camera views it would have drawn, and each frame replays them with every bone and
// camera blended between the last two ticks.

#ifndef HYPERBLADE_FRAMEPACING_H
#define HYPERBLADE_FRAMEPACING_H

#include "math/vec3.h"
#include <array>
#include <vector>

class CameraObject;
class EngineObject;
class MeshBone;

namespace FramePacing
{

constexpr auto kTickMs = 1000.0f / 30.0f;

// One object's world bones as a tick drew them, three rows of four.
using Rows = std::array<float, 12>;

struct ReplayObject
{
    EngineObject*     pObject = nullptr;
    std::vector<Rows> bones;
    bool              bVisible = false;
};

// One drawn tick: every object's bones and visibility, and the main camera.
struct ReplayFrame
{
    std::vector<ReplayObject> objects;
    std::array<float, 16>     view = {};
    Vec3                      eye  = {};
};

// A copy of the instant replay's last eight seconds, oldest first.
std::vector<ReplayFrame> RecentFrames();

void BeginTick();

// For the debug menu: ticks run in the last whole second, and how far the ticks' own time ran
// ahead of the wall clock over it.
int   TicksPerSecond();
float DriftMs();

// Called where ViewIt drew: keeps what it pushed, and the bones and camera as they stand, beside
// the last tick's.
void RecordView(CameraObject& camera);

// Runs the ticks the time since the last call owes, and returns how far the clock is into the
// next one, from 0 to 1.
float RunTicks();

// Draws the queued views at flAlpha between the last two ticks, then puts the last tick back.
void DrawViews(float flAlpha);

// Forgets the recorded views' pointers into bones about to be freed, as a model swap mid-tick frees them.
void DropBones(const MeshBone* pBones, int nCount);

// Forgets every recorded view and pose, for when the objects they point at go away.
void Reset();

// Pauses and replays the last eight seconds; false in a network match, paused, or before a second.
bool StartReplay();

// Ends the replay at the next tick.
void StopReplay();

bool Replaying();

} // namespace FramePacing

#endif
