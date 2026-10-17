#ifndef HYPERBLADE_CSETPOSITIONRELATIVE_H
#define HYPERBLADE_CSETPOSITIONRELATIVE_H

#include "planvm/releasingagendaitem.h"
#include "math/vec3.h"

class EngineObject;
struct PlanNode;

// Slot 0 is CGlideOnSurface's shared deleting destructor; slot 2 is a shared Release.
// VTABLE: HYPERX 0x004b1c68
class CSetPositionRelative : public ReleasingAgendaItem
{
public:
    CSetPositionRelative()
    : m_pTracked(0)
    , m_pMarker(0)
    , m_flUnused(0.0f)
    {
    }

    void Process(int nArg) override;
    void BindWithNode(class EngineObject* pObject, class GameBehaviorObject* pOwner, PlanNode* pNode, int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;
    void SetPositionWithTerrainHeight(Vec3* pMarker);
    void SetSourceObjectByIndex(short nIndex);

    void SetOffset(Vec3 offset);

    // The object, not its slot.
    EngineObject* m_pTracked; // 0x10
    Vec3*         m_pMarker;  // 0x14, INTERACTOR_MARKER at seven sites
    // Defaults to 120 and takes plan argument 4; nothing reads it.
    float m_flUnused; // 0x18
    Vec3  m_vOffset;  // 0x1c
    short m_wMode;    // 0x28
};

#endif
