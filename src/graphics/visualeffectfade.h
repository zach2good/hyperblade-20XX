#ifndef HYPERBLADE_VISUALEFFECTFADE_H
#define HYPERBLADE_VISUALEFFECTFADE_H

#include "palettescalarfade.h"
#include "common/staticassert.h"

// The original inlines the constructor into its one caller.
// VTABLE: HYPERX 0x004b1b18
class VisualEffectFade : public PaletteFade
{
public:
    virtual int Start(const PALETTEENTRY* pStartColour);

    // Slots two and four.
    virtual int Tick();
    virtual int ApplyPhase(int bPhase);

    explicit VisualEffectFade(const PaletteRange* pRange)
    : PaletteFade(pRange)
    {
        m_nKind     = PLAIN_FADE_KIND;
        m_nDuration = PLAIN_FADE_DURATION;
    }
};

STATIC_ASSERT(sizeof(VisualEffectFade) == 0x30);

// The linker folds PaletteScalarFade's empty destructor and deleter onto these two.
// SYNTHETIC: HYPERX 0x00410cd0
// VisualEffectFade::`scalar deleting destructor'

// SYNTHETIC: HYPERX 0x0042d030
// VisualEffectFade::~VisualEffectFade

#endif
