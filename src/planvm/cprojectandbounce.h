#ifndef HYPERBLADE_CPROJECTANDBOUNCE_H
#define HYPERBLADE_CPROJECTANDBOUNCE_H

#include "planvm/kindagendaitem.h"

class CollisionShape;
class EngineObject;

// Slot 0 is KindAgendaItem's deleting destructor, which thirty three tables share.
// VTABLE: HYPERX 0x004b1bcc
class CProjectAndBounce : public KindAgendaItem
{
public:
    CProjectAndBounce()
    : m_pVelocity(0)
    , m_pVelDir(0)
    , m_pSource(0)
    , m_pDest(0)
    , m_pSpeed(0)
    , m_pShape(0)
    , m_pNormal(0)
    , m_pPlayer(0)
    , m_pTarget(0)
    , m_pNetTargetPending(0)
    , m_pnPrevTick(0)
    , m_nMode(0)
    {
    }

    void ProjectAndBounce(int nArg);

    void IntegrateSpringAndGravity();
    void AddStep(const Vec3* pStep);

    float GetSpeed() const
    {
        return *m_pSpeed;
    }

    virtual void Process(int nArg);
    virtual void Release(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    Vec3*           m_pVelocity; // 0x10
    Vec3*           m_pVelDir;   // 0x14, the address of the body's velDir
    Vec3*           m_pSource;   // 0x18
    Vec3*           m_pDest;     // 0x1c
    float*          m_pSpeed;    // 0x20, INTERACTOR_SPEED at six sites
    CollisionShape* m_pShape;    // 0x24, INTERACTOR_SHAPE at six
    Vec3*           m_pNormal;   // 0x28, INTERACTOR_NORMAL at two
    // The body's pPlayer, copied by value; nothing on this class reads it.
    class CPlayer* m_pPlayer; // 0x2c
    // The address of the body's netTarget; the reset arm reads the steps below out of it.
    Vec3* m_pTarget;           // 0x30
    char* m_pNetTargetPending; // 0x34
    // Stored and never read.
    unsigned int* m_pnPrevTick; // 0x38
    Vec3          m_vStep;      // 0x3c
    int           m_nMode;      // 0x48
};

#endif
