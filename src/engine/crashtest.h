// The Crash Test dialog, which neither release opened: two dromers on a collision course, and the hit.

#ifndef HYPERBLADE_CRASHTEST_H
#define HYPERBLADE_CRASHTEST_H

#include <string>

namespace CrashTest
{

// A run in to the meeting point, its side a clock position with 12 the way the viewed dromer faces.
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

// The viewed dromer and the nearest opponent; false without a match or a followed dromer.
bool Stage(const Approach& mine, const Approach& his);

// Called by PlanCond::AttackedAnim as an attack reaches nVictim.
void RecordAttack(short nVictim, short nAttacker, int nAttack);

// The last attack on either staged dromer since the last Stage, empty until one lands.
const Hit& LastHit();

} // namespace CrashTest

#endif
