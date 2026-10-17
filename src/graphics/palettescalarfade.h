#ifndef HYPERBLADE_PALETTESCALARFADE_H
#define HYPERBLADE_PALETTESCALARFADE_H

#include "palettefade.h"

#include "common/compat.h"
#include "graphics/bitmapformat.h"

// VTABLE: HYPERX 0x004b2b6c
class PaletteScalarFade : public PaletteFade
{
public:
    explicit PaletteScalarFade(const PaletteRange* pRange);

    // Slots one and two of 0x004b2b6c.
    virtual int Start(const PaletteEntry* pStartColour);
    virtual int Tick();

protected:
    int           m_nUnused;    // 0x030
    unsigned char m_cBrightest; // 0x034

    // Sized so the whole range is covered in the duration.
    unsigned char m_cStep; // 0x035

public:
    virtual int ApplyPhase(int bPhase);
};

#endif
