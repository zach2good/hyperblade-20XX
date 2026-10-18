#ifndef HYPERBLADE_AGENDAITEM_H
#define HYPERBLADE_AGENDAITEM_H

#include "math/vector.h"

class EngineObject;
struct PlanNode;
class AgendaItem;

// src/globals.h declares it too, but reaches Windows types this header does not want.
extern const float g_flZeroAgendaItem;

// VTABLE: HYPERX 0x004b1744
class AgendaItem
{
public:
    // The whole body is the vptr store; the deleting destructors call it out of line.
    // FUNCTION: HYPERX 0x00412f90
    virtual ~AgendaItem()
    {
    }

    // Slot one, folded onto slot ten's body; nothing in the image reads the argument.
    virtual void Process(int)
    {
    }

    // Slot two, folded onto slot ten's body.
    virtual void Release(int)
    {
    }

    virtual void BindWithNode(class EngineObject* pObject, class GameBehaviorObject* pOwner, PlanNode* pNode, int nArg);

    // Slot four, folded onto 0x00480fb0, which the engine calls by name.
    virtual void Bind(class EngineObject*, class GameBehaviorObject*)
    {
    }

    virtual char Activate();
    virtual void Detach();

    // FUNCTION: HYPERX 0x0040b920
    virtual unsigned char GetReadyFlag()
    {
        return 0;
    }

    // Slot eight, folded onto slot seven's body.
    virtual char IsReplaceable()
    {
        return 0;
    }

    virtual void OnListChanged();

    // FUNCTION: HYPERX 0x0040b910
    virtual void Tick(const struct Vec3*)
    {
    }

    // Slots eleven and twelve, folded no-ops in the base.
    virtual struct Vec3* GetVector()
    {
        return nullptr;
    }

    virtual void OnKind(short)
    {
    }

    AgendaItem()
    : m_wKind(-1)
    , m_bReady(1)
    , m_bPersistent(0)
    , m_pOwner(nullptr)
    , m_pNext(nullptr)
    {
    }

    // Three kinds read m_bReady as something of their own.
    short m_wKind;  // 0x04
    char  m_bReady; // 0x06
    // Set on a behaviour's slot items and the palette effect; a removal unlinks it, never frees it.
    char          m_bPersistent; // 0x07
    EngineObject* m_pOwner;      // 0x08
    AgendaItem*   m_pNext;       // 0x0c

    // The camera's agenda items hang off a CameraObject.
    class CameraObject* GetCamera() const
    {
        return (class CameraObject*)m_pOwner;
    }

    void SetNext(AgendaItem* pNext)
    {
        m_pNext = pNext;
    }

    char FindByRef(class EngineObject* pObject);
};

// SYNTHETIC: HYPERX 0x0040b950
// AgendaItem::`scalar deleting destructor'

#endif
