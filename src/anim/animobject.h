// The base AnimClipSet and AnimDB share.

#ifndef HYPERBLADE_ANIMOBJECT_H
#define HYPERBLADE_ANIMOBJECT_H
#include "common/compat.h"

// VTABLE: HYPERX 0x004b22a4
class AnimObject
{
public:
    // FUNCTION: HYPERX 0x00417930
    virtual ~AnimObject() = default;

    AnimObject()
    : m_nInstallLevel(0)
    {
    }

    int m_nInstallLevel; // 0x04
};

// SYNTHETIC: HYPERX 0x00417940
// AnimObject::`scalar deleting destructor'

#endif
