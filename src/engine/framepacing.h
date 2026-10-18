// The simulation steps thirty times a second, and the scene draws as often as the loop runs. A tick
// queues the camera views it would have drawn, and each frame replays them with every bone and
// camera blended between the last two ticks.

#ifndef HYPERBLADE_FRAMEPACING_H
#define HYPERBLADE_FRAMEPACING_H

class CameraObject;
class MeshBone;

namespace FramePacing
{

constexpr auto kTickMs = 1000.0f / 30.0f;

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

} // namespace FramePacing

#endif
