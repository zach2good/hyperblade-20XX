#ifndef HYPERBLADE_RLESPRITESURFACE_H
#define HYPERBLADE_RLESPRITESURFACE_H

#include "graphics/cdddrawable.h"

// A software-rendered RLE sprite, loaded by name.
// VTABLE: HYPERX 0x004b32c4
class RLESpriteSurface : public CDDDrawable
{
public:
    RLESpriteSurface()
    : m_pSprite(nullptr)
    , m_pszName(nullptr)
    {
    }

    ~RLESpriteSurface() override;
    int Draw() override;

    int LoadByName(const char* pszName);

    struct RLESprite* m_pSprite; // 0x1c
    char*             m_pszName; // 0x20
};

// SYNTHETIC: HYPERX 0x0044f1e0
// RLESpriteSurface::`scalar deleting destructor'

#endif
