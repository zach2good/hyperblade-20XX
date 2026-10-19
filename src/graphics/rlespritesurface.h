#ifndef HYPERBLADE_RLESPRITESURFACE_H
#define HYPERBLADE_RLESPRITESURFACE_H

#include "common/record.h"
#include "graphics/cdddrawable.h"
#include "graphics/rlesprite.h"
#include <string>

// A software-rendered RLE sprite, loaded by name.
// VTABLE: HYPERX 0x004b32c4
class RLESpriteSurface : public CDDDrawable
{
public:
    RLESpriteSurface() = default;

    ~RLESpriteSurface() override;
    RLESpriteSurface(const RLESpriteSurface&)            = delete;
    RLESpriteSurface& operator=(const RLESpriteSurface&) = delete;
    RLESpriteSurface(RLESpriteSurface&&)                 = delete;
    RLESpriteSurface& operator=(RLESpriteSurface&&)      = delete;
    int               Draw() override;

    int LoadByName(const char* pszName);

    Record<RLESprite> m_pSprite; // 0x1c
    std::string       m_strName; // 0x20
};

// SYNTHETIC: HYPERX 0x0044f1e0
// RLESpriteSurface::`scalar deleting destructor'

#endif
