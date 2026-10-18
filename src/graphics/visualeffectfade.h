#ifndef HYPERBLADE_VISUALEFFECTFADE_H
#define HYPERBLADE_VISUALEFFECTFADE_H
#include "graphics/bitmapformat.h"

#include "palettescalarfade.h"

// The original inlines the constructor into its one caller.
// VTABLE: HYPERX 0x004b1b18
class VisualEffectFade : public PaletteFade
{
public:
    int Start(const PaletteEntry* pStartColour) override;

    // Slots two and four.
    int Tick() override;
    int ApplyPhase(int bPhase) override;

    explicit VisualEffectFade(const PaletteRange* pRange)
    : PaletteFade(pRange)
    {
        m_nKind    = kPlainFadeKind;
        m_duration = kPlainFadeDuration;
    }
};

// The linker folds PaletteScalarFade's empty destructor and deleter onto these two.
// SYNTHETIC: HYPERX 0x00410cd0
// VisualEffectFade::`scalar deleting destructor'

// SYNTHETIC: HYPERX 0x0042d030
// VisualEffectFade::~VisualEffectFade

#endif
