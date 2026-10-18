// The armory's weapons, the Mine and the Swarf: each sits in the armory, is carried and thrown.

#include "common/compat.h"
#include "devicector.h"
#include "objectclasses.h"
#include "engine/weaponobject.h"
#include "engine/engineobject.h"

// FUNCTION: HYPERX 0x00473ee0
WeaponObject::WeaponObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    m_nAttachedSlot     = 0;
    m_nPrevAttachedSlot = 0;
}

// FUNCTION: HYPERX 0x00473f30
WeaponObject::~WeaponObject() = default;
