#ifndef HYPERBLADE_CGLIDEONSURFACE_H
#define HYPERBLADE_CGLIDEONSURFACE_H

#include "planvm/kindagendaitem.h"

class EngineObject;
class CollisionPlane;

// Overrides slots 0, 1, 2, 4 and 9 of the base's thirteen.
// VTABLE: HYPERX 0x004b1b98
class CGlideOnSurface : public KindAgendaItem
{
public:
    CGlideOnSurface()
    : m_nOnSurface(0)
    {
    }

    void UpdateTerrainPhysics(int nArg);

    // Slot zero is KindAgendaItem's; only AgendaItem's own table holds 0x0040b950.
    virtual void Process(int nArg);
    virtual void Release(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    // Each field is an address inside the owner's physics body, taken in Bind.
    float* m_pflFriction; // 0x10, body + 0x124
    Vec3*  m_pVel;        // 0x14, body + 0x020
    Vec3*  m_pVelDir;     // 0x18, body + 0x02c
    Vec3*  m_pVelDirPrev; // 0x1c, body + 0x038
    // Bind stores it and OnListChanged clears it; nothing in the image reads it.
    Vec3* m_pAxis;    // 0x20, body + 0x100
    Vec3* m_pPos;     // 0x24, body + 0x044
    Vec3* m_pPosCopy; // 0x28, body + 0x050
    // A copy of the body's pShell, not its address.
    class CollisionShape* m_pSurface;   // 0x2c, body + 0x060 read
    Vec3*                 m_pNormal;    // 0x30, body + 0x064
    float*                m_pBankScale; // 0x34, body + 0x070
    float*                m_pSpeed;     // 0x38, body + 0x128
    float*                m_pBaseSpeed; // 0x3c, body + 0x00c
    float*                m_pLowAlt;    // 0x40, body + 0x118
    Vec3*                 m_pBankForce; // 0x44, body + 0x074
    Vec3*                 m_pSmoothVel; // 0x48, body + 0x014
    // The body's pPlayer, copied by value; nothing on this class reads it.
    class CPlayer*   m_pPlayer;           // 0x4c, body + 0x134
    Vec3*            m_pTargetPos;        // 0x50, body + 0x0f4
    char*            m_pNetTargetPending; // 0x54, body + 0x12c
    unsigned int*    m_pnPrevTick;        // 0x58, body + 0x130
    struct BodyMove* m_pMove;             // 0x5c, body + 0x138
    short*           m_pnHeldBindings;    // 0x60, body + 0x10c
    // A flag rather than a count: set to zero and tested `== 0`, never incremented.
    int m_nOnSurface; // 0x64
};

#endif
