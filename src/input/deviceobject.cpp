#include "common/compat.h"
#include "anim/animbinding.h"
#include "engine/devicector.h"
#include "globals.h"
#include "inputglobals.h"
#include "anim/animplayer.h"
#include "engine/engineobject.h"
#include "input/keyboarddeviceobj.h"

// m_pAnimPlayer learns its owner before the six fields clear; the last two are this.

// FUNCTION: HYPERX100 0x0048fed0
// FUNCTION: HYPERX 0x0045ca90
DeviceObject::DeviceObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    m_pAnimPlayer->SetObject(this);

    m_pSlots        = 0;
    m_nSlots        = 0;
    m_wUnused       = 0;
    m_pBindingTable = 0;
    m_pOwner        = this;
    m_pEntity       = this;
}

#ifdef HYPERX_V100
#include "engine/gameengine.h"

// FUNCTION: HYPERX100 0x00490020
void DeviceObject::SetOwnerFromSlot(short nSlot)
{
    m_pOwner = g_pGameEngine->m_ppSlots[nSlot];
}
#endif
