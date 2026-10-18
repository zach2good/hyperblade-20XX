#ifndef HYPERBLADE_CDDSURFACEOWNER_H
#define HYPERBLADE_CDDSURFACEOWNER_H

#include "common/compat.h"
#include "graphics/infobarslot.h"
#include <memory>

struct RLESprite;

// InfoBarSlot::InfoBarSlot at 0x00451be0 builds 0x38 bytes and writes table 0x004b3350.
// VTABLE: HYPERX 0x004b32cc
class CDDSurfaceOwner : public InfoBarSlot
{
public:
    CDDSurfaceOwner()
    : m_pSprite(nullptr)
    {
    }

    ~CDDSurfaceOwner() override;
    int Draw() override;

    // The same body RLESpriteSurface has, over this class's pair.
    int LoadByName(const char* pszName);

    // The pair is the other way round from RLESpriteSurface's.
    std::unique_ptr<char[]> m_pszName; // 0x38
    struct RLESprite*       m_pSprite; // 0x3c
};

// SYNTHETIC: HYPERX 0x0044f8d0
// CDDSurfaceOwner::`scalar deleting destructor'

#endif
