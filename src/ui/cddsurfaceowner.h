#ifndef HYPERBLADE_CDDSURFACEOWNER_H
#define HYPERBLADE_CDDSURFACEOWNER_H

#include "common/compat.h"
#include "common/record.h"
#include "graphics/infobarslot.h"
#include "graphics/rlesprite.h"
#include <memory>

// InfoBarSlot::InfoBarSlot at 0x00451be0 builds 0x38 bytes and writes table 0x004b3350.
// VTABLE: HYPERX 0x004b32cc
class CDDSurfaceOwner : public InfoBarSlot
{
public:
    int Draw() override;

    // The same body RLESpriteSurface has, over this class's pair.
    int LoadByName(const char* pszName);

    // The pair is the other way round from RLESpriteSurface's.
    std::unique_ptr<char[]> m_pszName; // 0x38
    Record<RLESprite>       m_pSprite; // 0x3c
};

#endif
