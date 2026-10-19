#include "common/compat.h"
#include "planvm/agendaitemids.h"
#include "planvm/agendaitem.h"
#include "globals.h"
#include "common/pooledclasses.h"
#include "engine/gameengine.h"
#include "physics/physicsbody.h"
#include "engine/devicector.h"
#include "engine/objectstats.h"
#include "math/vector.h"
#include "planvm/cslideonrail.h"
#include "math/spline.h"
#include "physics/collisionellipsoid.h"
#include "math/mathops.h"
#include "common/pooledownerroot.h"

// FUNCTION: HYPERX 0x0040cee0
CSlideOnRail::CSlideOnRail()
: m_pSpline(std::make_unique<Spline>())
{
    m_pSpline->InitSegment(g_arenaEllipsoid.GetRadiusX(), g_arenaEllipsoid.GetRadiusY());
}

// FUNCTION: HYPERX 0x0040cfa0
CSlideOnRail::~CSlideOnRail() = default;

// FUNCTION: HYPERX 0x0040d010
void CSlideOnRail::Bind(EngineObject* pOwner, GameBehaviorObject*)
{
    m_pNext  = nullptr;
    m_wKind  = Agenda_CSlideOnRail;
    m_bReady = 0;
    m_pOwner = pOwner;

    m_pVelocity         = &pOwner->GetPhysicsBody()->vel;
    m_pPosition         = &pOwner->GetPhysicsBody()->pos;
    m_pPrevPos          = &pOwner->GetPhysicsBody()->prevPos;
    m_pVelDir           = &pOwner->GetPhysicsBody()->velDir;
    m_pSurfNormal       = &pOwner->GetPhysicsBody()->surfNormal;
    m_flStartY          = m_pPosition->y;
    m_pflSpeed          = &pOwner->GetPhysicsBody()->flSpeed;
    m_pPlayer           = pOwner->GetPhysicsBody()->pPlayer;
    m_pNetTarget        = &pOwner->GetPhysicsBody()->netTarget;
    m_pNetTargetPending = &pOwner->GetPhysicsBody()->bNetTargetPending;
    m_pPrevTick         = &pOwner->GetPhysicsBody()->nPrevTick;
}

// FUNCTION: HYPERX 0x0040d0d0
void CSlideOnRail::OnListChanged()
{
    if (m_bReady == 0)
    {
        m_pNext             = nullptr;
        m_pOwner            = nullptr;
        m_pVelocity         = nullptr;
        m_pPosition         = nullptr;
        m_pPrevPos          = nullptr;
        m_pVelDir           = nullptr;
        m_pSurfNormal       = nullptr;
        m_pflSpeed          = nullptr;
        m_pPlayer           = nullptr;
        m_pNetTarget        = nullptr;
        m_pNetTargetPending = nullptr;
        m_bReady            = 1;

        g_apAgendaItemPools[m_wKind]->Insert(this);
    }
}

// The first of the two writes to the normal's z is dead in the original too.
// FUNCTION: HYPERX 0x0040d110
void CSlideOnRail::Process(int nArg)
{
    m_pSurfNormal->z = g_flZeroAgendaItem;
    m_pSurfNormal->x = g_flZeroAgendaItem;
    m_pSurfNormal->z = g_flOneAgendaItem;

    *m_pPrevPos = *m_pPosition;

    m_pSpline->UpdatePositionVelocity(m_pPosition,
                                      m_pVelocity);

    m_pPosition->z = -g_flOneAgendaItem;

    m_pVelocity->x = m_pPosition->x - m_pPrevPos->x;
    m_pVelocity->y = m_pPosition->y - m_pPrevPos->y;
    m_pVelocity->z = m_pPosition->z - m_pPrevPos->z;

    *m_pVelDir = *m_pVelocity;

    Math::Vec3Normalize(m_pVelDir);

    Vec3::ScaleTo(m_pVelDir, *m_pflSpeed, m_pVelocity);

    auto vScaled = *m_pVelocity;

    Vec3::ScaleTo(m_pVelocity, g_pGameEngine->m_clock.GetInvDelta(), &vScaled);

    m_pOwner->GetPhysicsBody()->step = vScaled;

    if (m_pNext != nullptr)
    {
        m_pNext->Process(nArg);
    }
}

// A pending net target moves the object by its own step over the frame before the rail takes it.

// FUNCTION: HYPERX 0x0040d240
void CSlideOnRail::Release(int nArg)
{
    float flDelta;
    Vec3  vScaled;
    Vec3  vSum;

    m_pSurfNormal->y = g_flZeroAgendaItem;
    m_pSurfNormal->x = g_flZeroAgendaItem;
    m_pSurfNormal->z = g_flOneAgendaItem;

    auto* const pPosition = m_pPosition;
    auto* const pPrevious = m_pPrevPos;

    pPrevious->x = pPosition->x;
    pPrevious->y = pPosition->y;
    pPrevious->z = pPosition->z;

    if (*m_pNetTargetPending != 0)
    {
        *m_pNetTargetPending = 0;

        flDelta = g_pGameEngine->m_clock.flDelta;

        auto* const pTracked = m_pOwner->GetPhysicsBody();

        Vec3::ScaleTo(&pTracked->step, flDelta, &vScaled);

        const Vec3* const pBase = m_pNetTarget;
        vSum.x                  = vScaled.x + pBase->x;
        vSum.y                  = vScaled.y + pBase->y;
        vSum.z                  = vScaled.z + pBase->z;

        m_pVelocity->x = (vSum.x - m_pPosition->x) * g_flHalfAgendaItem;
        m_pVelocity->y = (vSum.y - m_pPosition->y) * g_flHalfAgendaItem;
        m_pVelocity->z = (vSum.z - m_pPosition->z) * g_flHalfAgendaItem;

        m_pSpline->UpdatePositionVelocity(m_pPosition,
                                          m_pVelocity);
    }
    else
    {
        m_pSpline->UpdatePositionVelocity(m_pPosition,
                                          m_pVelocity);

        m_pVelocity->x = m_pPosition->x - m_pPrevPos->x;
        m_pVelocity->y = m_pPosition->y - m_pPrevPos->y;
        m_pVelocity->z = m_pPosition->z - m_pPrevPos->z;
    }

    const Vec3* const pVelocity  = m_pVelocity;
    auto* const       pDirection = m_pVelDir;

    pDirection->x = pVelocity->x;
    pDirection->y = pVelocity->y;
    pDirection->z = pVelocity->z;

    Math::Vec3Normalize(m_pVelDir);

    Vec3::ScaleTo(m_pVelDir, *m_pflSpeed, m_pVelocity);

    if (m_pNext != nullptr)
    {
        m_pNext->Release(nArg);
    }
}
