// The physics body every engine object allocates, and the attach pak a bubble carries.

#ifndef HYPERBLADE_PHYSICSBODY_H
#define HYPERBLADE_PHYSICSBODY_H

#include "math/vector.h"

class CollisionShape;
class EngineObject;

// 0x1c bytes; "InitAttachPaks" is the parser's own fatal string.
struct AttachPak
{
    short                  nSlot;      // 0x00
    short                  nLinkId;    // 0x02
    Vec3                   vOffset;    // 0x04
    int                    nUnused;    // 0x10
    class TransformObject* pTransform; // 0x14
    short                  nIndex;     // 0x18
};

// The bindings held this frame, one bit per key slot; g_anKeyBindingOrder swaps Attack and SpeedBurst.
enum HeldBinding
{
    HELD_FORWARD      = 0x001,
    HELD_STOP         = 0x002,
    HELD_ROTATE_LEFT  = 0x004,
    HELD_ROTATE_RIGHT = 0x008,
    HELD_SPEED_BURST  = 0x010,
    HELD_ATTACK       = 0x020,
    HELD_SHOOT        = 0x040,
    HELD_PASS         = 0x080,
    HELD_QUICK_SHOT   = 0x100,
    HELD_JUMP         = 0x200,
    HELD_THROTTLE     = HELD_FORWARD | HELD_STOP,
    HELD_ROTATE       = HELD_ROTATE_LEFT | HELD_ROTATE_RIGHT,
    HELD_MOVEMENT     = HELD_THROTTLE | HELD_ROTATE
};

// The step PreUpdateTick takes toward a target; CGlideOnSurface keeps a pointer to the pair.
struct BodyMove
{
    Vec3 dir;    // 0x00
    Vec3 target; // 0x0c
};

struct PhysicsBody
{
    // The object constructor allocates 0x150 bytes for one of these and then calls this.
    PhysicsBody();
    // Empty; the original linker folded it onto Stub::Assert.
    ~PhysicsBody();

    float GetForwardSpeedDt() const
    {
        return flForwardSpeedDt;
    }

    float GetSpeed() const
    {
        return flSpeed;
    }

    float GetSurfNormalX() const
    {
        return surfNormal.x;
    }

    float GetVelDirX() const
    {
        return velDir.x;
    }

    Vec3 GetVel() const
    {
        return vel;
    }

    void BindToObject(class EngineObject* pOwner, class ObjectDef* pDef);

    void ResetSpeeds();
    void ResetDynamics();
    char UpdateVelocityDirection();
    void IntegrateMotion();
    void UpdateBanking();
    void LaunchDiagonal();
    char StepWithGravity(float flFrac);
    char IntegrateGravity();
    void ResetOnSurface();

    // Both callers pass GetPhysicsBody(), so the receiver is this body and never the object.
    void ApplyAnimDrivenForce();

    void SetFlatOnTerrain(Vec3* pTargetPos);
    void ApplyBankForce(unsigned char cDir, float flForce);
    // Two bodies share this name in the original: 0x00407a60 and 0x00408190.
    void TakeSurfaceNormal(const Vec3* pPoint);
    void InitOnSurface(int nUnused);

    // The movement axes the trick input reads, and the axis the player's turn is taken from.
    void  UpdateMovementAxes();
    float GetAxisX() const;

    class EngineObject* pOwner; // 0x000
    // AgendaItem slots one and two are called on it: Release(0) and Process(0).
    class AgendaItem* pTarget;            // 0x004
    float             flForwardAccel;     // 0x008
    float             flForwardSpeedDt;   // 0x00c
    float             flBaseForwardSpeed; // 0x010
    Vec3              smoothVel;          // 0x014
    Vec3              vel;                // 0x020
    Vec3              velDir;             // 0x02c
    Vec3              velDirLatched;      // 0x038
    Vec3              pos;                // 0x044
    Vec3              prevPos;            // 0x050
    // The pak of the bubble holding this body, or null.
    AttachPak*      pAttach;        // 0x05c
    CollisionShape* pShell;         // 0x060
    Vec3            surfNormal;     // 0x064
    float           flGravDeltaVel; // 0x070
    Vec3            bankForce;      // 0x074
    // Restitution, from the parameter block's `elas`.
    float flElasticity;       // 0x080
    Vec3  step;               // 0x084
    Vec3  aRingVelHistory[4]; // 0x090
    Vec3  aRingVelSmooth[4];  // 0x0c0
    int   nRingIndex;         // 0x0f0
    // What the wire sets; move.target is what the mover steers at.
    Vec3 netTarget; // 0x0f4
    Vec3 axis;      // 0x100
    // HeldBinding bits, copied from the keyboard device each frame, and the frame before's.
    short nHeldBindings;     // 0x10c
    short nPrevHeldBindings; // 0x10e
    float flGravAccel;       // 0x110
    // csteertoposition.cpp writes the arc cosine of two axes into it.
    float flFacing; // 0x114
    // Scales both steering axis gains.
    float flSteerScale; // 0x118
    float flAccelDt;    // 0x11c
    float flAccel;      // 0x120
    // Written from the parameter block's `fric`, then overwritten by plans that grip or slide.
    float flFriction; // 0x124
    float flSpeed;    // 0x128
    // Raised with netTarget by a network command; whoever consumes it snaps and clears it.
    char bNetTargetPending; // 0x12c
    // The tick the body last stepped on, which CGlideOnSurface keeps a pointer to.
    unsigned int nPrevTick; // 0x130
    // The player that has taken this body over; CPlayer::AssignObject writes it.
    class CPlayer* pPlayer; // 0x134
    BodyMove       move;    // 0x138

    void ApplySteeringForce();

    void PreUpdateTick();

    void PerFrameTick();
};

#endif
