// The keyboard as an engine object, the device the match reads keys through g_pInputDevice.

#ifndef HYPERBLADE_KEYBOARDDEVICEOBJ_H
#define HYPERBLADE_KEYBOARDDEVICEOBJ_H

#include "engine/devicector.h"
#include "actionbinding.h"
#include "engine/gameengine.h"

// VTABLE: HYPERX 0x004b34e4
class KeyboardDeviceObj : public DeviceObject
{
public:
    KeyboardDeviceObj(int nSlot, class ObjectDef* pDef);

    ~KeyboardDeviceObj() override;

    static int  GetActionBitIndex(unsigned int nMask);
    static char BuildActionBindingTable(struct ActionBindingGroup** ppTableOut, short* pnGroupsOut, KeyboardDeviceObj* pDevice);

    void CommitKeyBindings(const unsigned int* pnKeys, int nCount);

    unsigned int* GetDefaultKeyBindings() const;
    // The down handler returns -1 for an unknown key, and the caller then tries the global keys.
    int  OnButtonDown(unsigned int nButton);
    int  HandleKeyDown(unsigned int nKey);
    int  HandleKeyUp(unsigned int nKey);
    void CheckButtonTimeouts();

    unsigned int m_nHeldMask;                          // 0xd28, the current frame's held-action bitmask
    unsigned int m_nPrevHeldMask;                      // 0xd2c, the previous frame's snapshot of it
    unsigned int m_nStickyMask;                        // 0xd30, actions latched by a sticky binding
    int          m_aPrevPressTime[INPUT_SLOT_COUNT];   // 0xd34, press time before the one in m_aPressTime
    int          m_aPressTime[INPUT_SLOT_COUNT];       // 0xd74, this key's last press timestamp
    int          m_aPrevReleaseTime[INPUT_SLOT_COUNT]; // 0xdb4, release time before the one in m_aReleaseTime
    int          m_aReleaseTime[INPUT_SLOT_COUNT];     // 0xdf4, this key's last release timestamp
    unsigned int m_nDoubleTapMs;                       // 0xe34, the double-tap window both handlers test against
    // The sticky binding recorded per key, or null.
    struct ActionBinding* m_apStickyBinding[INPUT_SLOT_COUNT]; // 0xe38

    static unsigned int* LoadBindingsFromIni();
};

// SYNTHETIC: HYPERX 0x0045d090
// KeyboardDeviceObj::`scalar deleting destructor'

inline KeyboardDeviceObj* GameEngine::GetInputDevice(int nSlot) const
{
    return (KeyboardDeviceObj*)m_ppSlots[nSlot];
}

#endif
