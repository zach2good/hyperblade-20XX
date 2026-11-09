// The Crash Test dialog's engine side: it sets two dromers on a collision course and records the
// attack that follows. Neither release opened the dialog; this is a port of what its fields ask.

#ifndef HYPERBLADE_CRASHTEST_H
#define HYPERBLADE_CRASHTEST_H

#include <string>

namespace CrashTest
{

// A dromer's run in to the meeting point: its speed, how far out it starts, and the side it comes
// from as a clock position, 12 o'clock being the way the viewed dromer faces.
struct Approach
{
    float flSpeed    = 0.0f;
    float flDistance = 0.0f;
    float flClock    = 0.0f;
};

struct Hit
{
    std::string strLevel;
    std::string strZone;
};

// Sends the viewed dromer and the nearest opponent at the point the viewed dromer stands on.
// False when no match is running or the camera follows no dromer.
bool Stage(const Approach& mine, const Approach& his);

// Called by PlanCond::AttackedAnim as an attack reaches nVictim.
void RecordAttack(short nVictim, short nAttacker, int nAttack);

// The last attack on either staged dromer since the last Stage, empty until one lands.
const Hit& LastHit();

} // namespace CrashTest

#endif
