#ifndef HYPERBLADE_CBEATTACHED_H
#define HYPERBLADE_CBEATTACHED_H

#include "planvm/kindagendaitem.h"

class EngineObject;
struct AttachPak;

// Slot 0 is the shared deleting destructor, declared on CGlideOnSurface.
// VTABLE: HYPERX 0x004b1c00
class CBeAttached : public KindAgendaItem
{
public:
    CBeAttached()
    : m_pVel(0)
    , m_pPos(0)
    , m_pPrevPos(0)
    , m_pflSpeed(0)
    , m_pAttach(0)
    {
    }

    virtual void Process(int nArg);
    virtual void Release(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    // All five bind to a named body field: vel, pos, prevPos, flSpeed and the attach pak.
    Vec3*  m_pVel;     // 0x10
    Vec3*  m_pPos;     // 0x14
    Vec3*  m_pPrevPos; // 0x18
    float* m_pflSpeed; // 0x1c
    // The owner body's attach pak, copied at Bind and refreshed from the body on every tick.
    AttachPak* m_pAttach; // 0x20
};

#endif
