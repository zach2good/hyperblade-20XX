#include "common/compat.h"
#include "engine/devicector.h"
#include "physicsbody.h"
#include "common/staticinit.h"
#include "math/vector.h"
#include "engine/gameengine.h"
#include "engine/arenastate.h"
#include "engine/objectdef.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "planvm/planvm.h"
#include "engine/objectclasses.h"
#include "engine/objectstats.h"
class CollisionPlane;
#include "banking.h"
#include "math/transformobject.h"
#include "common/pooledclasses.h"
#include "graphics/meshbone.h"
#include <math.h>
#include "engine/playerobject.h"
#include "app/gameapp.h"
#include "collisionplane.h"
#include "engine/engineobject.h"
#include "engine/cameraobject.h"
#include "collisionobj.h"
#include "engine/objectkind.h"
#include "engine/statids.h"
#include "engine/objectphase.h"
#include "planvm/behaviorids.h"

float g_flUnusedPhysicsBodyA = g_flZeroPhysicsBody;
float g_flUnusedPhysicsBodyB = g_flZeroPhysicsBody;
float g_flUnusedPhysicsBodyC = g_flZeroPhysicsBody;

// The two vectors of nought this initialises from.
// GLOBAL: HYPERX 0x004b1610
extern const Vec3 g_vecZeroVelocity;
// GLOBAL: HYPERX 0x004b1620
extern const Vec3 g_vecZeroPosition;

#define BODY_RING_LENGTH 4

// EngineObject::EngineObject allocates 0x150 bytes and calls this; nothing else zeroes it.
// FUNCTION: HYPERX 0x00407520
PhysicsBody::PhysicsBody()
{
    pOwner  = 0;
    pTarget = 0;

    flSpeed            = g_flZeroPhysicsBody;
    flForwardAccel     = g_flZeroPhysicsBody;
    flForwardSpeedDt   = g_flZeroPhysicsBody;
    flBaseForwardSpeed = g_flZeroPhysicsBody;
    flAccel            = g_flZeroPhysicsBody;

    smoothVel     = g_vecZeroVelocity;
    vel           = g_vecZeroVelocity;
    velDir        = g_vecZeroVelocity;
    velDirLatched = g_vecZeroVelocity;

    pos     = g_vecZeroPosition;
    prevPos = g_vecZeroPosition;

    pAttach = 0;
    pShell  = 0;

    surfNormal = g_vecZeroVelocity;

    flGravDeltaVel = g_flZeroPhysicsBody;
    flGravAccel    = g_flZeroPhysicsBody;

    bankForce = g_vecZeroVelocity;

    flElasticity = g_flZeroPhysicsBody;

    axis = g_vecZeroVelocity;

    for (int nIndex = 0; nIndex < BODY_RING_LENGTH; nIndex++)
    {
        aRingVelHistory[nIndex] = g_vecZeroVelocity;
        aRingVelSmooth[nIndex]  = g_vecZeroVelocity;
    }

    nRingIndex = 0;

    step      = g_vecZeroVelocity;
    netTarget = g_vecZeroPosition;

    nHeldBindings     = 0;
    nPrevHeldBindings = 0;

    flFacing     = g_flZeroPhysicsBody;
    flSteerScale = g_flZeroPhysicsBody;
    flAccelDt    = g_flZeroPhysicsBody;

    flFriction = g_flZeroPhysicsBody;

    bNetTargetPending = 0;

    pPlayer = 0;
}

PhysicsBody::~PhysicsBody()
{
}

// Degrees to radians, as a double.
#define DEGREES_TO_RADIANS 0.0174532925

extern const float g_flGravity;

// Speed and acceleration are premultiplied by the frame delta here; the heading is in degrees.
// FUNCTION: HYPERX 0x00407780
void PhysicsBody::BindToObject(EngineObject* pOwnerObject, ObjectDef* pDefRec)
{
    pOwner = pOwnerObject;

    prevPos = pDefRec->m_vPos;
    pos     = pDefRec->m_vPos;

    flSpeed = 0.0f;

    pShell = g_pGameEngine->m_pArena->m_pEllipsoid;

    flFriction         = pDefRec->m_flFriction;
    flElasticity       = pDefRec->m_flElasticity;
    flForwardAccel     = 0.0f;
    flBaseForwardSpeed = pDefRec->m_flSpeed;
    flAccel            = pDefRec->m_flAccel;

    flForwardSpeedDt = g_pGameEngine->m_clock.flDelta * pDefRec->m_flSpeed;
    flAccelDt        = g_pGameEngine->m_clock.flDeltaSquared * pDefRec->m_flAccel;

    flSteerScale   = g_flOnePhysicsReset;
    flGravAccel    = g_flGravity;
    flGravDeltaVel = g_pGameEngine->GetGravityDt();

    if (pDefRec->m_flAzimuth < g_flZeroPhysicsBody)
    {
        pDefRec->m_flAzimuth = g_flDegreesPerTurn + pDefRec->m_flAzimuth;
    }

    velDir.x = (float)cos(pDefRec->m_flAzimuth * DEGREES_TO_RADIANS);
    velDir.y = (float)-sin(pDefRec->m_flAzimuth * DEGREES_TO_RADIANS);
    flFacing = (float)(pDefRec->m_flAzimuth * DEGREES_TO_RADIANS);
}

// Six of the fields are skipped for the main camera.

// FUNCTION: HYPERX 0x00407890
void PhysicsBody::ResetSpeeds()
{
    ResetDynamics();

    if (pOwner->GetSlot() != CAMERA_MAIN_SLOT)
    {
        velDir.x = g_flZeroPhysicsBody;
        velDir.y = g_flOnePhysicsReset;
        velDir.z = g_flZeroPhysicsBody;
        vel.x    = g_flZeroPhysicsBody;
        vel.y    = g_flZeroPhysicsBody;
        vel.z    = g_flZeroPhysicsBody;
    }

    smoothVel.x = g_flZeroPhysicsBody;
    smoothVel.y = g_flZeroPhysicsBody;
    smoothVel.z = g_flZeroPhysicsBody;
    axis.x      = g_flZeroPhysicsBody;
    axis.y      = g_flZeroPhysicsBody;
    axis.z      = g_flZeroPhysicsBody;
}

// Seeded from the frame's own delta, so a reset mid-frame carries that frame's timing.
// FUNCTION: HYPERX 0x00407920
void PhysicsBody::ResetDynamics()
{
    flSpeed        = g_flZeroPhysicsBody;
    flForwardAccel = g_flZeroPhysicsBody;

    flForwardSpeedDt = g_pGameEngine->m_clock.flDelta * flBaseForwardSpeed;
    flAccelDt        = g_pGameEngine->m_clock.flDeltaSquared * flAccel;
}

// FUNCTION: HYPERX 0x00407960
void PhysicsBody::PreUpdateTick()
{
    const float flStep = g_pGameEngine->m_clock.flDelta * g_pGameEngine->m_clock.flPrevInvDelta;
    flSpeed            = flStep * flSpeed;

    vel.ScaleInPlace(flStep);

    flForwardAccel = g_pGameEngine->m_clock.flDeltaSquared * g_pGameEngine->m_clock.flPrevInvDeltaSquared * flForwardAccel;

    flGravDeltaVel = g_pGameEngine->m_clock.flDeltaSquared * flGravAccel;

    flForwardSpeedDt = g_pGameEngine->m_clock.flDelta * flBaseForwardSpeed;

    flAccelDt = g_pGameEngine->m_clock.flDeltaSquared * flAccel;
}

// The cleanup and speed update run whatever the phase; phases outside 1 to 3 report nothing.

// FUNCTION: HYPERX 0x004079f0
void PhysicsBody::PerFrameTick()
{
    PreUpdateTick();

    switch (pOwner->GetPhase())
    {
        case OBJECT_PHASE_LOCAL:
        case OBJECT_PHASE_BUILDING:
            if (pTarget != 0)
            {
                pTarget->Process(0);
            }
            break;

        case OBJECT_PHASE_NETWORK:
            pOwner->ProcessReceivedPacket();

            if (pTarget != 0)
            {
                pTarget->Release(0);
            }
            break;
    }

    pOwner->GetPlanState()->PostUpdateCleanup();

    PlayerObject* const pObject = (PlayerObject*)pOwner;

    if (pObject->m_nKind == KIND_DROMER)
    {
        pObject->m_stats.UpdateMomentumAndEnergy();
    }
}

#define SHOOT_DRIVE_TAIL_MS 1000

#define AXIS_FLOOR_IDLE 0.005

#define TURN_SCALE 0.072f
#define TURN_LIMIT 0.2f

// Whether Shoot was held last frame, and the tick it was released.
// GLOBAL: HYPERX 0x004cb3a9
extern unsigned char g_bMoveAxesShootHeld;
// GLOBAL: HYPERX 0x004cb3ac
extern unsigned int g_nMoveAxesShootReleaseTick;

// The stop binding, published for the two steering blocks and cleared by each.
// GLOBAL: HYPERX 0x004cb3a8
extern unsigned char g_bMoveAxesStopHeld;

inline float PhysicsBody::GetAxisX() const
{
    return axis.x;
}

// FUNCTION: HYPERX 0x00407a60
void PhysicsBody::TakeSurfaceNormal(const Vec3* pPoint)
{
    Vec3* const pNormal = pShell->NormalAtPoint(pPoint);

    if (pNormal == 0)
    {
        // STRING: HYPERX 0x004bb3bc
        g_gameApp.Fatal("BAD SURFACE NORMAL - InitOnSurface B!");
    }

    surfNormal = *pNormal;
}

// FUNCTION: HYPERX 0x00407ab0
void PhysicsBody::ResetOnSurface()
{
    Vec3 vOrigin;

    ResetDynamics();

    const float flHeight = pShell->GetZ(pos.x, pos.y);

    prevPos.z = flHeight;
    pos.z     = flHeight;

    Vec3* const pNormal = pShell->NormalAtPoint(&pos);

    if (pNormal == 0)
    {
        // STRING: HYPERX 0x004bb3bc
        g_gameApp.Fatal("BAD SURFACE NORMAL - InitOnSurface B!");
    }

    surfNormal = *pNormal;

    velDir.x = g_flOnePhysicsReset;
    velDir.y = g_flZeroPhysicsBody;

    vOrigin.x = g_flZeroPhysicsBody;
    vOrigin.y = g_flZeroPhysicsBody;
    vOrigin.z = g_flZeroPhysicsBody;

    CollisionObj plane(&surfNormal, &vOrigin, g_flZeroPhysicsBody, g_flOnePhysicsReset);

    const float flZ = plane.GetZ(velDir.x, velDir.y);

    velDirLatched.x = velDir.x;
    velDirLatched.y = velDir.y;
    velDir.z        = flZ;
    velDirLatched.z = flZ;

    if (!Math::Vec3Normalize(&velDir))
    {
        // STRING: HYPERX 0x004bb3e4
        g_gameApp.Fatal("Normalizing error: InitOnSurface routine!");
    }

    vel.x = GetSpeed() * velDir.x;
    vel.y = flSpeed * velDir.y;
    vel.z = GetSpeed() * velDir.z;

    smoothVel.x = g_flZeroPhysicsBody;
    smoothVel.y = g_flZeroPhysicsBody;
    smoothVel.z = g_flZeroPhysicsBody;
    axis.x      = g_flZeroPhysicsBody;
    axis.y      = g_flZeroPhysicsBody;
    axis.z      = g_flZeroPhysicsBody;
    flFacing    = g_flZeroPhysicsBody;
}

// Holding Shoot drives the throttle, and the drive lasts a second past the release.

// FUNCTION: HYPERX 0x00407c40
void PhysicsBody::UpdateMovementAxes()
{
    float flPush;
    float flPull;
    float flCap;
    float flDot;
    float flCross;
    float flLimit;
    short nHeld;

    flPush = (((PlayerObject*)pOwner)->m_stats.GetWeightedStat(RATING_BALANCE) * 0.14f + 0.5f) * flAccelDt * g_flForceIncrement;

    const float flClock = g_pGameEngine->m_clock.flDeltaSquared;

    flPull = (((PlayerObject*)pOwner)->m_stats.GetWeightedStat(RATING_BALANCE) * 0.12f + 0.4f) * flSpeed * flClock;

    flCap = (((PlayerObject*)pOwner)->m_stats.GetWeightedStat(RATING_BALANCE) * 0.06f + 0.2f) * flAccelDt * g_flMaxAcceleration;

    if ((nHeldBindings & HELD_SHOOT) != 0)
    {
        axis.x               = flPush + axis.x;
        g_bMoveAxesShootHeld = 1;
    }
    else if (g_bMoveAxesShootHeld != 0)
    {
        g_nMoveAxesShootReleaseTick = g_pGameEngine->m_clock.nTickMs;
        g_bMoveAxesShootHeld        = 0;
    }

    if (g_nMoveAxesShootReleaseTick != 0)
    {
        axis.x = flPush + axis.x;

        if (g_pGameEngine->m_clock.nTickMs - g_nMoveAxesShootReleaseTick > SHOOT_DRIVE_TAIL_MS)
        {
            g_nMoveAxesShootReleaseTick = 0;
        }
    }

    nHeld = nHeldBindings;

    if (nHeld == 0 && nPrevHeldBindings != 0)
    {
        axis.y            = g_flZeroPhysicsBody;
        axis.x            = g_flZeroPhysicsBody;
        nPrevHeldBindings = 0;

        return;
    }

    if (nHeldBindings == 0 && nPrevHeldBindings == 0)
    {
        if (fabs(axis.x) > AXIS_FLOOR_IDLE)
        {
            axis.x = g_flMovementAxisDecay * axis.x;
        }
        else
        {
            axis.x = g_flZeroPhysicsBody;
        }

        if (fabs(axis.y) > AXIS_FLOOR_IDLE)
        {
            axis.y = g_flMovementAxisDecay * axis.y;
        }
        else
        {
            axis.y = g_flZeroPhysicsBody;
        }

        return;
    }

    if ((nHeld & (HELD_THROTTLE | HELD_SHOOT)) == 0)
    {
        if (fabs(axis.x) > 0.01)
        {
            axis.x = g_flMovementAxisDecay * axis.x;
        }
        else
        {
            axis.x = g_flZeroPhysicsBody;
        }
    }

    if ((nHeld & HELD_ROTATE) == 0)
    {
        if (fabs(axis.y) > 0.01)
        {
            axis.y = g_flMovementAxisDecay * axis.y;
        }
        else
        {
            axis.y = g_flZeroPhysicsBody;
        }
    }

    const short nAxis = (short)(nHeld & HELD_MOVEMENT);

    if (nAxis != 0)
    {
        if (((unsigned char)nAxis & HELD_MOVEMENT) == HELD_FORWARD)
        {
            const Vec3* pRowA = &velDir;
            const Vec3* pRowB = &velDirLatched;

            flDot = g_flOnePhysicsReset - (pRowB->x * GetVelDirX() + (pRowB->y * pRowA->y + pRowB->z * pRowA->z));

            if ((flCross = pRowB->y * pRowA->x - pRowB->x * pRowA->y) < -0.001)
            {
                axis.y = (float)fabs(flDot) * flPush + axis.y;
            }
            else if (flCross > 0.0001f)
            {
                axis.y -= (float)fabs(flDot) * flPush;
            }
        }

        if ((nAxis & HELD_FORWARD) != 0)
        {
            axis.x = flPush + axis.x;
        }

        g_bMoveAxesStopHeld = (unsigned char)(((unsigned char)nAxis >> 1) & 1);

        const float flTurn = g_flNetTracking * flSpeed * TURN_SCALE;

        if ((nAxis & HELD_ROTATE_LEFT) != 0)
        {
            if (g_bMoveAxesStopHeld != 0)
            {
                axis.y = flTurn * -4.0f;
            }
            else if (-flTurn < axis.y)
            {
                axis.y = -flTurn;
            }

            g_bMoveAxesStopHeld = 0;

            const float flDownY = axis.y - flPull;
            axis.y              = flDownY;

            if (flDownY < (flLimit = flSpeed * -TURN_LIMIT))
            {
                axis.y = flLimit;
            }
        }

        if ((nAxis & HELD_ROTATE_RIGHT) != 0)
        {
            if (g_bMoveAxesStopHeld != 0)
            {
                axis.y = flTurn * 4.0f;
            }
            else if (axis.y < flTurn)
            {
                axis.y = flTurn;
            }

            g_bMoveAxesStopHeld = 0;

            const float flUpY = flPull + axis.y;
            axis.y            = flUpY;

            if (flUpY > (flLimit = flSpeed * TURN_LIMIT))
            {
                axis.y = flLimit;
            }
        }
    }

    const double dAbs = fabs(axis.x);

    if (dAbs < flCap)
    {
        axis.x = GetAxisX();
    }
    else
    {
        axis.x = (float)(axis.x / dAbs) * flCap;
    }
}

// Never written; the marker is in src/common/behobjmonitorkeyboard.cpp.
extern int g_bTwoPlayers;

// The threshold on the steering scale, a double.
#define STEER_SCALE_THRESHOLD 1.000001

// The cross product and normalise run even when a second player skips the force.
// FUNCTION: HYPERX 0x00408080
void PhysicsBody::ApplySteeringForce()
{
    Vec3 vLateral;

    const Vec3* pNormal = &surfNormal;
    const Vec3* pDir    = &velDir;

    vLateral.x = pNormal->z * pDir->y - pNormal->y * pDir->z;
    vLateral.y = pDir->z * pNormal->x - pNormal->z * pDir->x;
    vLateral.z = pNormal->y * pDir->x - pDir->y * pNormal->x;

    Math::Vec3Normalize(&vLateral);

    if (g_bTwoPlayers == 0)
    {
        if (flSteerScale > STEER_SCALE_THRESHOLD)
        {
            smoothVel.x = (vLateral.x * axis.y + velDir.x) * flSteerScale + smoothVel.x;
            smoothVel.y = (vLateral.y * axis.y + velDir.y) * flSteerScale + smoothVel.y;
            smoothVel.z = (vLateral.z * axis.y + velDir.z) * flSteerScale + smoothVel.z;
        }
        else
        {
            smoothVel.x = vLateral.x * axis.y + velDir.x * axis.x + smoothVel.x;
            smoothVel.y = vLateral.y * axis.y + velDir.y * axis.x + smoothVel.y;
            smoothVel.z = velDir.z * axis.x + vLateral.z * axis.y + smoothVel.z;
        }
    }
}

// src/common/staticinit.h carries the marker.
extern const float g_flZeroPhysicsBody;

// Below this speed the heading is left alone.
// GLOBAL: HYPERX 0x004b1600
extern const float g_flMinVelDirSpeed;

// Projects along the previous call's normal, so the result depends on the last call.
// FUNCTION: HYPERX 0x00408190
void PhysicsBody::InitOnSurface(int)
{
    Vec3 vNextPos;

    prevPos = pos;

    // A second name keeps the normal's copy ahead of the speed's loads (0x00408202).
    Vec3* const pSurfNormal = &surfNormal;

    vNextPos.x = prevPos.x + vel.x;
    vNextPos.y = prevPos.y + vel.y;
    vNextPos.z = prevPos.z + vel.z;

    pShell->RayIntersection(pSurfNormal, &vNextPos, &pos);

    Vec3* const pNormal = pShell->NormalAtPoint(&pos);

    if (pNormal == 0)
    {
        // STRING: HYPERX 0x004bb3bc
        g_gameApp.Fatal("BAD SURFACE NORMAL - InitOnSurface B!");
    }

    *pSurfNormal = *pNormal;

    const float flVelZ   = vel.z;
    const float flVelY   = vel.y;
    const float flVelX   = vel.x;
    const float flLength = (float)sqrt(flVelX * flVelX + flVelY * flVelY + flVelZ * flVelZ);
    flSpeed              = flLength;

    if (flLength > g_flMinVelDirSpeed)
    {
        const float flScale = g_flOnePhysicsReset / flLength;
        velDir.x            = flScale * vel.x;
        velDir.y            = flScale * vel.y;
        velDir.z            = flScale * vel.z;
    }

    velDirLatched = velDir;

    smoothVel.x = smoothVel.y = smoothVel.z = g_flZeroPhysicsBody;
    bankForce.x = bankForce.y = bankForce.z = g_flZeroPhysicsBody;
}

// A step over fifty snaps; the heading sum starts with y at one, so it never divides by nought.
// FUNCTION: HYPERX 0x004082c0
void PhysicsBody::IntegrateMotion()
{
    Vec3 vSmooth;
    Vec3 vDir;

    int i;

    vSmooth.x = g_flZeroPhysicsBody;
    vSmooth.y = g_flZeroPhysicsBody;
    vSmooth.z = g_flZeroPhysicsBody;
    vDir.x    = g_flZeroPhysicsBody;
    vDir.y    = g_flOnePhysicsReset;
    vDir.z    = g_flZeroPhysicsBody;

    // The client writes x through EDI and y and z by field (0x004082f3 to 0x00408319).
    Vec3* const pMoveDir = &move.dir;

    pMoveDir->x = move.target.x - pos.x;
    move.dir.y  = move.target.y - pos.y;
    move.dir.z  = move.target.z - pos.z;

    float       flZ    = pMoveDir->z;
    float       flY    = pMoveDir->y;
    float       flX    = pMoveDir->x;
    const float flRoot = (float)sqrt(flX * flX + flY * flY + flZ * flZ);

    Math::Vec3Normalize(pMoveDir);

    if (flRoot > 50.0)
    {
        pos = move.target;
    }

    float flStep = g_pGameEngine->m_clock.GetDeltaSquared() * g_flBandwidth;
    flAccelDt    = flStep;
    flStep *= flRoot;

    aRingVelSmooth[nRingIndex].x = flStep * pMoveDir->x;
    aRingVelSmooth[nRingIndex].y = flStep * move.dir.y;
    aRingVelSmooth[nRingIndex].z = flStep * move.dir.z;

    for (i = 0; i < BODY_RING_LENGTH; i++)
    {
        vSmooth.x += aRingVelSmooth[i].x;
        vSmooth.y += aRingVelSmooth[i].y;
        vSmooth.z += aRingVelSmooth[i].z;
    }

    Vec3::ScaleTo(&vSmooth, 0.25f, &smoothVel);

    flZ              = step.z;
    flY              = step.y;
    flX              = step.x;
    flForwardSpeedDt = (float)sqrt(flX * flX + flY * flY + flZ * flZ) * g_pGameEngine->m_clock.flDelta * 1.232f;

    vel.x = vel.x + smoothVel.x;
    vel.y = smoothVel.y + vel.y;
    vel.z = smoothVel.z + vel.z;

    InitOnSurface(0);

    if (flSpeed > g_flMinVelDirSpeed)
    {
        aRingVelHistory[nRingIndex].x = vel.x;
        aRingVelHistory[nRingIndex].y = vel.y;
        aRingVelHistory[nRingIndex].z = vel.z;
    }

    for (i = 0; i < BODY_RING_LENGTH; i++)
    {
        vDir.x += aRingVelHistory[i].x;
        vDir.y += aRingVelHistory[i].y;
        vDir.z += aRingVelHistory[i].z;
    }

    velDir = vDir;

    Math::Vec3Normalize(&velDir);

    nRingIndex = (nRingIndex + 1) % BODY_RING_LENGTH;
}

// Pins the direction to the forty-five degree diagonal in x and z.

// FUNCTION: HYPERX 0x00408540
void PhysicsBody::LaunchDiagonal()
{
    ResetDynamics();

    const float flDiagonal = flSpeed * 0.707f;

    velDir.x = 0.707f;
    velDir.y = g_flZeroPhysicsBody;
    velDir.z = 0.707f;

    vel.x = flDiagonal;
    vel.y = GetSpeed() * velDir.y;
    vel.z = flDiagonal;

    smoothVel.x = g_flZeroPhysicsBody;
    smoothVel.y = g_flZeroPhysicsBody;
    smoothVel.z = g_flZeroPhysicsBody;

    axis.x   = g_flZeroPhysicsBody;
    axis.y   = g_flZeroPhysicsBody;
    axis.z   = g_flZeroPhysicsBody;
    flFacing = g_flZeroPhysicsBody;
}

// Thirty-two, stored into the body's flGravAccel and scaling the engine's gravity.
// GLOBAL: HYPERX 0x004b1604
extern const float g_flGravity;

// The whole-frame form of StepWithGravity; the gravity is re-read when it falls below 0.01.
// FUNCTION: HYPERX 0x004085d0
char PhysicsBody::IntegrateGravity()
{
    prevPos.x = pos.x;
    prevPos.y = pos.y;
    prevPos.z = pos.z;

    pos.x = vel.x + pos.x;
    pos.y = vel.y + pos.y;
    pos.z = (vel.z + pos.z) - g_flGravityHalf * flGravDeltaVel;

    if (flGravDeltaVel < 0.01)
    {
        flGravAccel    = g_flGravity;
        flGravDeltaVel = g_pGameEngine->m_clock.GetDeltaSquared() * g_flGravity;
    }

    vel.z -= flGravDeltaVel;

    const float flLength = vel.Magnitude();
    flSpeed              = flLength;

    if (flLength > g_flMinVelDirSpeed)
    {
        const float flInvSpeed = g_flOnePhysicsReset / flLength;
        velDir.x               = flInvSpeed * vel.x;
        velDir.y               = flInvSpeed * vel.y;
        velDir.z               = vel.z * flInvSpeed;
        return 1;
    }

    return 0;
}

// A body slower than a thousandth keeps its direction, and the answer says so.

// FUNCTION: HYPERX 0x004086a0
char PhysicsBody::StepWithGravity(float flFrac)
{
    prevPos.x = pos.x;
    prevPos.y = pos.y;
    prevPos.z = pos.z;

    pos.x = flFrac * vel.x + pos.x;
    pos.y = flFrac * vel.y + pos.y;
    pos.z = (flFrac * vel.z + pos.z) - flFrac * (g_flGravityHalf * flGravDeltaVel);

    vel.z -= flFrac * flGravDeltaVel;

    const float flLength = vel.Magnitude();
    flSpeed              = flLength;

    if (flLength > g_flMinVelDirSpeed)
    {
        const float flInvSpeed = g_flOnePhysicsReset / flLength;
        velDir.x               = flInvSpeed * vel.x;
        velDir.y               = flInvSpeed * vel.y;
        velDir.z               = vel.z * flInvSpeed;
        return 1;
    }

    return 0;
}

// Projects the velocity off the surface normal; below g_flMinVelDirSpeed it answers nought.
// FUNCTION: HYPERX 0x00408760
char PhysicsBody::UpdateVelocityDirection()
{
    char cResult;

    const float flDrag   = g_flOnePhysicsReset - flFriction;
    const float flBlend  = flDrag - pShell->GetFriction();
    const float flWeight = pShell->GetElasticity() * flElasticity;

    const float flDot = ((vel.z * surfNormal.z) + vel.y * surfNormal.y) + vel.GetX() * surfNormal.x;

    const float flProjY = -(flDot * surfNormal.y);
    const float flProjZ = -(flDot * surfNormal.z);
    const float flProjX = -(flDot * surfNormal.x);

    const float flNewX = (flProjX + vel.x) * flBlend + flProjX * flWeight;
    vel.x              = flNewX;
    const float flNewY = (flProjY + vel.y) * flBlend + flProjY * flWeight;
    vel.y              = flNewY;
    const float flNewZ = (flProjZ + vel.z) * flBlend + flProjZ * flWeight;
    vel.z              = flNewZ;

    const float flLength = (float)sqrt(vel.LengthSquared());
    flSpeed              = flLength;

    if (flLength > g_flMinVelDirSpeed)
    {
        const double dRecip = g_flOnePhysicsReset / flLength;
        cResult             = 1;

        velDir.x = (float)(flNewX * dRecip);
        velDir.y = (float)(flNewY * dRecip);
        velDir.z = (float)(flNewZ * dRecip);
    }
    else
    {
        cResult = 0;
    }

    return cResult;
}

// The right vector is built in a global and normalised in place.

// FUNCTION: HYPERX 0x00408880
void PhysicsBody::ApplyBankForce(unsigned char cDir, float flForce)
{
    const Vec3* pNormal = &surfNormal;
    const Vec3* pDir    = &velDir;

    g_vBankRight.x = pNormal->z * pDir->y - pNormal->y * pDir->z;
    g_vBankRight.y = pNormal->x * pDir->z - pNormal->z * pDir->x;
    g_vBankRight.z = pDir->x * pNormal->y - GetSurfNormalX() * pDir->y;

    Math::Vec3Normalize(&g_vBankRight);

    if (cDir == g_nBankDirRight)
    {
        bankForce.x = g_vBankRight.x * flForce + bankForce.x;
        bankForce.y = g_vBankRight.y * flForce + bankForce.y;
        bankForce.z = g_vBankRight.z * flForce + bankForce.z;

        return;
    }

    bankForce.x -= g_vBankRight.x * flForce;
    bankForce.y -= g_vBankRight.y * flForce;
    bankForce.z -= g_vBankRight.z * flForce;
}

// A tenth of a radian, about 5.73 degrees.
// GLOBAL: HYPERX 0x004bb3b8
extern float g_flBankingAngleRad;

// The bone whose world position the camera follows.
#define CAMERA_BONE 9

// Bits 2 and 3 enable the bank and bit 2 alone picks its sign; the angle is a tenth of a radian.
// FUNCTION: HYPERX 0x00408940
void PhysicsBody::UpdateBanking()
{
    float flAngle;
    float flCos;
    float flSin;

    const unsigned short nFlags = nHeldBindings;

    if ((nFlags & HELD_ROTATE) != 0)
    {
        if ((nFlags & HELD_ROTATE_LEFT) != 0)
        {
            flCos   = (float)cos(g_flBankingAngleRad);
            flSin   = (float)sin(g_flBankingAngleRad);
            flAngle = g_flBankingAngleRad;
        }
        else
        {
            flAngle = -g_flBankingAngleRad;
            flCos   = (float)cos(flAngle);
            flSin   = (float)sin(flAngle);
        }

        TransformObject* const pTransform = &pOwner->m_transformB;

        pTransform->m_rotZ.flCos = (float)cos(flAngle);
        pTransform->m_rotZ.flSin = (float)sin(flAngle);

        pTransform->ApplyRotationZ();

        const float flDirX = velDir.x;

        velDir.x = flDirX * flCos - flSin * velDir.y;
        velDir.y = (flDirX * flSin) + flCos * velDir.y;

        EngineObject* pCamera = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

        if (pCamera->GetPlanState()->m_pCurrent->m_wKind != BEH_TRACK_TRAJECTORY)
        {
            const float flBungee = g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT)->GetBungeeLength();

            Vec3* const pCamPos = &pCamera->GetPhysicsBody()->pos;

            EngineObject* pObject = pOwner;

            const TransformObject* pRow = &pObject->m_pBones[CAMERA_BONE].m_world;

            float flRow = pRow->m_a[0][3];

            pCamPos->x = flRow - pObject->m_transformB.GetElement(0, 1) * flBungee;

            pObject = pOwner;

            flRow = pRow->m_a[1][3];

            pCamPos->y = flRow - pObject->m_transformB.GetElement(1, 1) * flBungee;

            pObject = pOwner;

            flRow = pRow->m_a[2][3];

            pCamPos->z = flRow - pObject->m_transformB.GetElement(2, 1) * flBungee;
        }
    }
}

// The stored Z is re-read under the new position, not the one the caller passed.
// FUNCTION: HYPERX 0x00408a60
void PhysicsBody::SetFlatOnTerrain(Vec3* pTargetPos)
{
    Vec3 vBasis;
    Vec3 vCross;

    pTargetPos->z = pShell->GetZ(pTargetPos->x, pTargetPos->y);

    Vec3* pPosition = pOwner->GetPositionPtr();

    *pPosition = *pTargetPos;

    pOwner->GetLocalTransformPtr()->ResetToIdentity();

    pOwner->GetLocalTransformPtr()->SetTranslation(g_flZeroPhysicsBody, g_flZeroPhysicsBody, g_flZeroPhysicsBody);

    pOwner->ApplyRenderSlotPose();

    const Vec3* pNormal = pShell->NormalAtPoint(pTargetPos);

    if (pNormal == 0)
    {
        // STRING: HYPERX 0x004bb410
        g_gameApp.Fatal("BAD SURFACE NORMAL - SetFlat!");
    }

    float flAxis = 0.0f;

    // The client reads the normal through EDI and by field (0x00408b07, 0x00408b18).
    Vec3* const pNormalDst = &surfNormal;

    *pNormalDst = *pNormal;

    if (fabs(1.0f - pNormalDst->x) < 0.001f && fabs(-surfNormal.y) < 0.001f && fabs(-surfNormal.z) < 0.001f)
    {
        flAxis = 1.0f;
    }

    vBasis.x = flAxis * pNormalDst->z;
    vBasis.y = -pNormalDst->z;
    vBasis.z = pNormalDst->y - flAxis * pNormalDst->x;

    Math::Vec3Normalize(&vBasis);

    if (vBasis.y != g_flZeroPhysicsBody)
    {
        pOwner->GetLocalTransformPtr()->m_a[0][2] = pNormalDst->x;
        pOwner->GetLocalTransformPtr()->m_a[1][2] = surfNormal.y;
        pOwner->GetLocalTransformPtr()->m_a[2][2] = surfNormal.z;

        pOwner->GetLocalTransformPtr()->m_a[0][1] = vBasis.x;
        pOwner->GetLocalTransformPtr()->m_a[1][1] = vBasis.y;
        pOwner->GetLocalTransformPtr()->m_a[2][1] = vBasis.z;

        vCross.x = vBasis.y * pNormalDst->z - vBasis.z * pNormalDst->y;
        vCross.y = vBasis.z * pNormalDst->x - vBasis.x * pNormalDst->z;
        vCross.z = vBasis.x * pNormalDst->y - vBasis.y * pNormalDst->x;

        Math::Vec3Normalize(&vCross);

        pOwner->GetLocalTransformPtr()->m_a[0][0] = vCross.x;
        pOwner->GetLocalTransformPtr()->m_a[1][0] = vCross.y;
        pOwner->GetLocalTransformPtr()->m_a[2][0] = vCross.z;
    }

    prevPos = pos;

    pos = *pTargetPos;

    const float flGroundZ = pShell->GetZ(pTargetPos->x, pTargetPos->y);

    pos.z = flGroundZ;

    const float flAngle = flFacing;

    TransformObject* const pTransform = pOwner->GetLocalTransformPtr();

    pTransform->m_rotZ.flCos = (float)cos(flAngle);
    pTransform->m_rotZ.flSin = (float)sin(flAngle);

    pTransform->ApplyRotationZ();

    pOwner->ApplyRenderSlotPose();
}

// Adds the animation's translation, rotated through the mesh's world axes, then zeroes it.
// FUNCTION: HYPERX 0x00408cc0
void PhysicsBody::ApplyAnimDrivenForce()
{
    MeshBone*              pMesh      = pOwner->m_pBones;
    TransformObject* const pTransform = &pMesh->m_local;

    if (pMesh->IsAnimationDirty() != 0)
    {
        const MeshBoneDesc* pBone = pMesh->m_pDesc;

        pTransform->SetTranslation(
            pTransform->GetElement(0, 3) * pBone->m_vScale.x,
            pTransform->GetElement(1, 3) * pBone->m_vScale.y,
            pTransform->m_a[2][3] * pBone->m_vScale.z);

        pOwner->m_pBones->ClearFlags(MESHBONE_ANIMATION_DIRTY);
    }

    const double dTransX = pTransform->m_a[0][3];
    const double dTransZ = pTransform->m_a[2][3];

    pos.x = (float)(pMesh->m_world.m_a[0][0] * dTransX + pMesh->m_world.m_a[0][2] * dTransZ + pos.x);

    pos.y = (float)(pMesh->m_world.m_a[1][0] * dTransX + pMesh->m_world.m_a[1][2] * dTransZ + pos.y);

    pos.z = (float)(pMesh->m_world.m_a[2][0] * dTransX + pMesh->m_world.m_a[2][2] * dTransZ + pos.z);

    if (pOwner->GetSlot() < PLAYER_OBJECT_COUNT && ((PlayerObject*)pOwner)->m_wFlyingSemaphore != 0)
    {
        pOwner->GetSlot();
    }
    else
    {
        pShell->RayIntersection(&surfNormal, &pos, &pos);
    }

    pTransform->SetTranslation(0.0f, (float)pTransform->GetElement(1, 3), 0.0f);
}

// A tenth of a radian, the angle an object banks through in a turn.
float g_flBankingAngleRad = 0.1f;
