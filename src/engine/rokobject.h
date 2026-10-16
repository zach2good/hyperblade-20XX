#ifndef HYPERBLADE_ROKOBJECT_H
#define HYPERBLADE_ROKOBJECT_H

#include "common/compat.h"
#include "engine/devicector.h"
#include "engine/engineobject.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"

// All ones takes the Rok off its spawn slot.
#define SPAWN_SLOT_ANY 0xffffffff

// VTABLE: HYPERX 0x004b39f4
class RokObject : public EngineObject
{
public:
    RokObject(int nSlot, class ObjectDef* pDef);
    ~RokObject();

    virtual void ApplyRenderSlotPose();
    virtual void Postpass();

    void SetRenderSlotFlags();
    void ActivateAtSpawnSlot(unsigned int nSlot);

    // Three plan bodies read the carrier history below, so it is public.
public:
    // The Rok's carrier history; Set_AttachPak moves all four when the Rok changes hands.
    int m_nCarrierSlot;        // 0xd14, the slot carrying the Rok now
    int m_nPrevCarrierSlot;    // 0xd18, the slot that carried it before
    int m_nOddCarrierSlot;     // 0xd1c, the current carrier when its slot is odd
    int m_nOddPrevCarrierSlot; // 0xd20, the previous carrier when its slot was odd
    // The Rok value (id_ACF_IncRokValue): signed, team relative, stepped by two between -4 and 4.
    short m_wRokValue; // 0xd24
    // Minus one when clear. id_ACF_SetRokKillState writes it and both RokIsCharged tests read it.
    int m_nKillState; // 0xd28
    // Set while the object stands as a team catalog entry; only ActivateAtSpawnSlot sets it.
    char m_bAtSpawnSlot; // 0xd2c
};

// SYNTHETIC: HYPERX 0x00473b60
// RokObject::`scalar deleting destructor'

static_assert(sizeof(RokObject) == 0xd30);

inline RokObject* GameEngine::GetRok() const
{
    return (RokObject*)m_ppSlots[SLOT_ROK];
}

// The atexit thunk for ApplyRenderSlotPose's scratch transform.
// SYNTHETIC: HYPERX 0x00474140
// $E11

#endif
