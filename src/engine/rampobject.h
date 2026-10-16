// The ramp and its four collision parts, which are separate members rather than an array.

#ifndef HYPERBLADE_RAMPOBJECT_H
#define HYPERBLADE_RAMPOBJECT_H

struct Vec3;

#include "gameengine.h"
#include "globals.h"
#include "engineglobals.h"
#include "devicector.h"
#include "physics/collisionplane.h"
#include "physics/collisionobj.h"
#include "engine/engineobject.h"

// One line in the ramp's local x-z plane. Four of them describe the ramp's shape.
struct PlaneEdge
{
    float flSlope;
    float flBase;
};

// A slope of one and no base until the planes are built.
struct RampEdge : public PlaneEdge
{
    RampEdge()
    {
        flSlope = 1.0f;
        flBase  = 0.0f;
    }
};

// VTABLE: HYPERX 0x004b3e64
class RampObject : public EngineObject
{
public:
    RampObject(int nSlot, class ObjectDef* pDef);

    void InitCollisionPlanes();

    CollisionObj* GetCollisionObjForPoint(Vec3* pPoint);

private:
    RampEdge     m_edgeNearMax;  // 0xd14
    RampEdge     m_edgeFarMax;   // 0xd1c
    RampEdge     m_edgeNearMin;  // 0xd24
    RampEdge     m_edgeFarMin;   // 0xd2c
    CollisionObj m_planeMaxX;    // 0xd34
    CollisionObj m_planeMinX;    // 0xd88
    CollisionObj m_planeNear;    // 0xddc
    CollisionObj m_planeFar;     // 0xe30
    int          m_bPlanesReady; // 0xe84
};

// SYNTHETIC: HYPERX 0x0047fe40
// RampObject::`scalar deleting destructor'

// Left to the compiler: a declared destructor would store RampObject's table first.
// SYNTHETIC: HYPERX 0x0047fe60
// RampObject::~RampObject

#endif
