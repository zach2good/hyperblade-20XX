#ifndef HYPERBLADE_CSETATTITUDE_H
#define HYPERBLADE_CSETATTITUDE_H

#include "planvm/kindagendaitem.h"

class EngineObject;

// Slot 0 is CGlideOnSurface's shared deleting destructor.
// VTABLE: HYPERX 0x004b1dd4
class CSetAttitude : public KindAgendaItem
{
public:
    CSetAttitude()
    : m_bLimitPitch(0)
    {
    }

    void Process(int nArg) override;
    void Release(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;
    void OnKind(short nSlot) override;

    // Bind stores the address of a physics body member into each of these three.
    Vec3*  m_pVelDir;     // 0x10, the address of the body's velDir
    Vec3*  m_pSurfNormal; // 0x14, the address of the body's surfNormal
    float* m_pFacing;     // 0x18, the address of the body's flFacing
    // The owner's local transform; the same name at this offset on other agenda items is not this.
    class TransformObject* m_pTransform; // 0x1c
    // A byte flag the process and release arms both test.
    char m_bLimitPitch; // 0x20
};

#endif
