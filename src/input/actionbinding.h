// One sixteen-byte row of the action binding table.

#ifndef HYPERBLADE_ACTIONBINDING_H
#define HYPERBLADE_ACTIONBINDING_H

// The device's sixteen action slots.
constexpr auto kInputSlotCount = 0x10;

constexpr auto kActionBindingCount = 0x19;

// A binding's trigger: the key bits, each key's double-tap copy sixteen bits up, and three flags.
constexpr auto kActionBindingUp        = 0x80000000;
constexpr auto kActionBindingMask      = 0x7fff;
constexpr auto kActionBindingDoubleTap = 0x10001;
constexpr auto kActionBindingHighHalf  = 0xffff0000;

// The press and release came inside the double-tap window. A press binding with this flag fires
// only once the key is held past the window.
constexpr auto kActionBindingQuick = 0x40000000;

// The press test reads 0x20000000 only beside kActionBindingQuick, and the one row that sets
// it sets both, so it has no meaning of its own to name.
#define ACTION_BINDING_HELD (0x20000000 | kActionBindingQuick)

struct ActionBinding
{
    unsigned int m_nAction;
    unsigned int m_nCombine;
    unsigned int m_nClear;
    unsigned int m_nKey;
};

// One device's binding table, sorted into a down list and an up list per slot.
struct ActionBindingGroup
{
    int            m_nKeyCount;               // 0x000
    ActionBinding* m_apDown[kInputSlotCount]; // 0x004
    ActionBinding* m_apUp[kInputSlotCount];   // 0x044
    int            m_anUp[kInputSlotCount];   // 0x084
    int            m_anDown[kInputSlotCount]; // 0x0c4
    int            m_nCount;                  // 0x104
    unsigned short m_anKey[kInputSlotCount];  // 0x108
};

extern const ActionBinding g_aActionBindings[kActionBindingCount];

#endif
