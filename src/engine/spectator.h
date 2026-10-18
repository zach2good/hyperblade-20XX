// Bot spectator mode: the player's dromer runs on the AI plans, and the main camera flies free.

#ifndef HYPERBLADE_SPECTATOR_H
#define HYPERBLADE_SPECTATOR_H

#include "math/vec3.h"
#include <optional>

namespace Spectator
{

// Slot 0 runs the Defense plan, as slot 1 does for the other team, instead of the keyboard's Human
// plan. Changing it mid-match swaps the plan at once; a network session refuses it.
bool Autopilot();
void SetAutopilot(bool bOn);

// The main camera follows the keyboard, mouse and pad instead of its tracking plan.
bool FreeCamera();
void SetFreeCamera(bool bOn);

// While either is on, the player's keys reach nothing but the match and global keys.
bool HoldsInput();

struct View
{
    Vec3 eye;
    Vec3 dir;
};

// The free camera's view for this tick, or nothing while it is off. The main camera's own eye and
// direction seed it each time it starts.
std::optional<View> FreeView(const Vec3& eye, const Vec3& dir);

} // namespace Spectator

#endif
