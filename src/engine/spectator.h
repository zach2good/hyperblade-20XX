// Bot spectator mode: every dromer runs on the AI plans, and the main camera flies free.

#ifndef HYPERBLADE_SPECTATOR_H
#define HYPERBLADE_SPECTATOR_H

#include "math/vec3.h"
#include <optional>

namespace Spectator
{

// Every dromer runs the AI plans: a dromer sent to the keyboard's Human plan runs Defense instead.
// On while the setting is on or the Tab side view is up. Changing it mid-match swaps the plans at
// once, handing control back to the dromer the camera follows; a network session refuses it.
bool Autopilot();

// The debug menu's and -b's setting, which holds whatever the camera does.
bool AutopilotSetting();
void SetAutopilot(bool bOn);

// Tab spectates from the side view, and the next Tab hands control back, wherever the camera has
// gone in between. Answers whether the camera should take the Tab too.
bool ToggleSideView();

// The main camera says each view whether it is in the side view.
void FollowCamera(bool bSideView);

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
