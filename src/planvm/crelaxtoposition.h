#ifndef HYPERBLADE_CRELAXTOPOSITION_H
#define HYPERBLADE_CRELAXTOPOSITION_H

#include "planvm/kindagendaitem.h"

class EngineObject;
struct PlanNode;

// Slot 0 is KindAgendaItem's shared deleting destructor; slot 7 shares CAvoidObjects::IsReplaceable's body.
// VTABLE: HYPERX 0x004b1ed8
class CRelaxToPosition : public KindAgendaItem
{
public:
    void Process(int nArg) override;
    void Release(int nArg) override;
    void BindWithNode(class EngineObject* pObject, class GameBehaviorObject* pOwner, PlanNode* pNode, int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    // Slot seven answers one where the base answers nought; the original folds it onto one shared body.
    unsigned char GetReadyFlag() override
    {
        return 1;
    }

    // 0x20 is read as two types in equal numbers, so it stays raw.
    Vec3          m_vDest;       // 0x10
    EngineObject* m_pTarget;     // 0x1c, INTERACTOR_TARGET at five against WATCHED at four
    char          m_bCameraBias; // 0x20
};

#endif
