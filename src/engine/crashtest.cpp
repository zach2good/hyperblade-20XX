#include "engine/crashtest.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/playerobject.h"
#include "globals.h"
#include "math/vec3.h"
#include "physics/physicsbody.h"
#include "planvm/plancond.h"
#include <cmath>
#include <format>
#include <numbers>

namespace
{

// A staged run's hits count for this long; later ones are ordinary play.
constexpr auto kWindow = Clock::Milliseconds{ 10000 };

short               s_nMine = -1;
short               s_nHis  = -1;
Clock::Milliseconds s_stagedAt{};

CrashTest::Hit s_lastHit;

// The clock position on a dial whose 12 o'clock is vForward, flat on the floor.
Vec3 ClockDirection(const Vec3& vForward, float flClock)
{
    const auto flAngle = flClock * std::numbers::pi_v<float> / 6.0f;
    const auto flSin   = std::sin(flAngle);
    const auto flCos   = std::cos(flAngle);

    return Vec3{ vForward.x * flCos + vForward.y * flSin, vForward.y * flCos - vForward.x * flSin, 0.0f };
}

void Launch(short nSlot, const Vec3& vMeet, const Vec3& vForward, const CrashTest::Approach& approach)
{
    const auto vFrom = ClockDirection(vForward, approach.flClock);

    auto* const pBody = g_pGameEngine->GetPlayer(nSlot)->GetPhysicsBody();

    pBody->pos     = Vec3{ vMeet.x + vFrom.x * approach.flDistance, vMeet.y + vFrom.y * approach.flDistance, vMeet.z };
    pBody->prevPos = pBody->pos;
    pBody->velDir  = Vec3{ -vFrom.x, -vFrom.y, 0.0f };
    pBody->vel     = Vec3{ -vFrom.x * approach.flSpeed, -vFrom.y * approach.flSpeed, 0.0f };
    pBody->flSpeed = approach.flSpeed;
}

const char* AttackName(int nAttack)
{
    switch (nAttack)
    {
        case kAttackPunch:
            return "High, punch";

        case kAttackClothesLineL:
        case kAttackClothesLineR:
            return "High, clothesline";

        case kAttackShoulderCheckL:
        case kAttackShoulderCheckR:
            return "Middle, shoulder check";

        case kAttackHipCheck:
            return "Middle, hip check";

        case kAttackLegSweepL:
        case kAttackLegSweepR:
            return "Low, leg sweep";

        default:
            return "Unknown";
    }
}

} // namespace

bool CrashTest::Stage(const Approach& mine, const Approach& his)
{
    if (g_pGameEngine == nullptr || g_pGameEngine->m_nViewedSlot < kSlotDromer0 || g_pGameEngine->m_nViewedSlot > kSlotDromer3)
    {
        return false;
    }

    const auto nMine = g_pGameEngine->m_nViewedSlot;

    const Vec3 vMeet = *g_pGameEngine->GetPlayer(nMine)->GetPositionPtr();

    // The opponents are the two dromer slots of the other parity.
    const auto nHis = [&]
    {
        const auto nFirst  = static_cast<short>((nMine & 1) ^ 1);
        const auto nSecond = static_cast<short>(nFirst + 2);

        const auto distance = [&](short nSlot)
        {
            const auto* const pPos = g_pGameEngine->GetPlayer(nSlot)->GetPositionPtr();

            return std::hypot(pPos->x - vMeet.x, pPos->y - vMeet.y);
        };

        if (distance(nSecond) < distance(nFirst))
        {
            return nSecond;
        }

        return nFirst;
    }();

    const auto vForward = [&]
    {
        auto vFacing = g_pGameEngine->GetPlayer(nMine)->GetPhysicsBody()->velDir;

        vFacing.z = 0.0f;

        const auto flLength = std::hypot(vFacing.x, vFacing.y);

        if (flLength < 0.001f)
        {
            return Vec3{ 0.0f, 1.0f, 0.0f };
        }

        return Vec3{ vFacing.x / flLength, vFacing.y / flLength, 0.0f };
    }();

    Launch(nMine, vMeet, vForward, mine);
    Launch(nHis, vMeet, vForward, his);

    s_nMine    = nMine;
    s_nHis     = nHis;
    s_stagedAt = g_pGameEngine->m_clock.tick;
    s_lastHit  = {};

    return true;
}

void CrashTest::RecordAttack(short nVictim, short nAttacker, int nAttack)
{
    if ((nVictim != s_nMine && nVictim != s_nHis) || g_pGameEngine->m_clock.tick - s_stagedAt > kWindow)
    {
        return;
    }

    const auto& vVictim   = *g_pGameEngine->GetPlayer(nVictim)->GetPositionPtr();
    const auto& vAttacker = *g_pGameEngine->GetSlot(nAttacker)->GetPositionPtr();
    const auto& vFacing   = g_pGameEngine->GetPlayer(nVictim)->GetPhysicsBody()->velDir;

    // The attacker's clock position around the victim, 12 o'clock dead ahead.
    const auto flAhead = atan2f(vFacing.x, vFacing.y);
    const auto flTo    = atan2f(vAttacker.x - vVictim.x, vAttacker.y - vVictim.y);
    const auto flClock = std::fmod(std::fmod((flTo - flAhead) * 6.0f / std::numbers::pi_v<float>, 12.0f) + 12.0f, 12.0f);

    const auto nHour = [&]
    {
        const auto nRounded = static_cast<int>(std::lround(flClock)) % 12;

        if (nRounded == 0)
        {
            return 12;
        }

        return nRounded;
    }();

    s_lastHit = Hit{ AttackName(nAttack), std::format("Dromer {}, {} o'clock", nVictim, nHour) };
}

const CrashTest::Hit& CrashTest::LastHit()
{
    return s_lastHit;
}
