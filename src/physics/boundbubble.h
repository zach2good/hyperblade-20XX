// The bound bubble: a plan node with two fields of its own, and one accessor.

#ifndef HYPERBLADE_BOUNDBUBBLE_H
#define HYPERBLADE_BOUNDBUBBLE_H

struct Vec3;

#include "engine/gameengine.h"
#include "common/staticinit.h"
#include <memory>

class EngineObject;

class BoundBubble;
class BubbleCmd;

// Component -1 is the object's own transform, -2 a world-space centre, 0 and up a bone.
class BubbleShapeNode
{
public:
    short m_nComponent; // 0x00
    float m_flRadius;   // 0x04
    Vec3  m_vCentre;    // 0x08
};

#include "planvm/bubblenode.h"

// SYNTHETIC: HYPERX 0x0042a1c0
// BubbleNode::`scalar deleting destructor'

// Named CBoundBubble in the database.
// VTABLE: HYPERX 0x004b2a38
class BoundBubble : public BubbleNode
{
public:
    inline EngineObject* GetOwner() const
    {
        return m_pOwner;
    }

    BoundBubble();

    BoundBubble(EngineObject* pOwner, BubbleCmd* pShape)
    : m_pShapeNode(nullptr)
    , m_flRadius(g_flZeroEngineObject)
    {
        Init(pOwner, pShape);
    }

    void Init(EngineObject* pOwner, BubbleCmd* pShape);

    void FindAttachComponent(AttachPak* pRequest);

    void          GetWorldPosition(Vec3* pOut) override;
    unsigned char DoesIntersect(BoundBubble* pOther) override;

    unsigned char SphereSphereOverlap(BoundBubble* pOther);

    std::unique_ptr<BubbleShapeNode> m_pShapeNode; // 0x024
    float                            m_flRadius;   // 0x028, the shape node's radius, copied at init
    // What BubbleNode::m_pChildren points at.
    std::unique_ptr<BoundBubble[]> m_pChildBlock;
};

// FUNCTION: HYPERX 0x0042a8f0
// BoundBubble::~BoundBubble

// SYNTHETIC: HYPERX 0x0042ac10
// BoundBubble::`vector deleting destructor'

#include "physics/bubblecmd.h"
#include "engine/engineobject.h"
#include "graphics/meshbone.h"
#include "app/gameapp.h"

inline void GameEngine::SetColObject(short nSlot, BoundBubble* pBubble)
{
    m_ppColObjects[nSlot] = pBubble;
}

inline BoundBubble* GameEngine::GetColObjPtr(short nSlot) const
{
    BoundBubble* const pBubble = m_ppColObjects[nSlot];

    if (pBubble == nullptr)
    {
        // STRING: HYPERX 0x004bb2b0
        g_gameApp.Fatal("ERROR: GetColObjPtr: NULL Pointer\n");
    }

    return pBubble;
}

inline void EngineObject::AttachBubble(short nBone, BoundBubble* pBubble)
{
    m_pBones[nBone].m_pBubble = pBubble;
}

// The child stride: the original steps its child array by this class's size.

#endif
