#ifndef HYPERBLADE_CSLIDEONRAIL_H
#define HYPERBLADE_CSLIDEONRAIL_H

#include <memory>

#include "common/compat.h"
#include "planvm/agendaitem.h"

class EngineObject;
class Spline;

// VTABLE: HYPERX 0x004b1a84
class CSlideOnRail : public AgendaItem
{
public:
    CSlideOnRail();

    void Process(int nArg) override;
    void Release(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    ~CSlideOnRail() override;
    Vec3*                   m_pVelocity;   // 0x10, the address of the body's vel
    Vec3*                   m_pPosition;   // 0x14, the address of the body's pos
    Vec3*                   m_pPrevPos;    // 0x18, the address of the body's prevPos
    Vec3*                   m_pVelDir;     // 0x1c, the address of the body's velDir
    Vec3*                   m_pSurfNormal; // 0x20, the address of the body's surfNormal
    float*                  m_pflSpeed;    // 0x24, the address of the body's flSpeed
    float                   m_flStartY;    // 0x28, the body's pos.y when bound
    std::unique_ptr<Spline> m_pSpline;     // 0x2c
    // The body's pPlayer, copied by value; nothing on this class reads it.
    class CPlayer* m_pPlayer; // 0x30
    // The body's netTarget; the tick sums its three components.
    Vec3* m_pNetTarget; // 0x34
    // The body's bNetTargetPending, the flag raised beside netTarget off the wire.
    char* m_pNetTargetPending; // 0x38
    // The body's nPrevTick; nothing reads it back.
    unsigned int* m_pPrevTick; // 0x3c
};

// SYNTHETIC: HYPERX 0x0040cf80
// CSlideOnRail::`scalar deleting destructor'

#endif
