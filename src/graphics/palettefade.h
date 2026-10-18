// The fade is 0x30 bytes and the scalar fade 0x38, so the byte at 0x34 is the derived class's.

#ifndef HYPERBLADE_PALETTEFADE_H
#define HYPERBLADE_PALETTEFADE_H

#include "common/compat.h"
#include "graphics/bitmapformat.h"
#include "graphics/paletterange.h"

// GLOBAL: HYPERX 0x004cb780
extern int g_nPaletteAnimated;

// VTABLE: HYPERX 0x004b2b58
class PaletteFade
{
public:
    explicit PaletteFade(const PaletteRange* pRange);

    // Slots one, two and four hold _purecall in the original's table.
    virtual ~PaletteFade(); // slot 0
    // cpaletteeffect.cpp starts the fade through slot one and steps it through slot two.
    virtual int  Start(const PaletteEntry* pStartColour) = 0;
    virtual int  Tick()                                  = 0; // slot 2, pure
    virtual void RestoreEntries();                            // slot 3, 0x0042ce90
    virtual int  ApplyPhase(int bPhase) = 0;                  // slot 4, pure

protected:
    int           m_nKind;        // 0x004
    unsigned int  m_iStart;       // 0x008
    unsigned int  m_cEntries;     // 0x00c
    PaletteEntry* m_pEntries;     // 0x010
    unsigned int  m_iWorkStart;   // 0x014
    unsigned int  m_cWorkEntries; // 0x018
    PaletteEntry* m_pWorkEntries; // 0x01c
    unsigned int  m_nDuration;    // 0x020
    unsigned int  m_dwStepped;    // 0x024
    int           m_bPhase;       // 0x028
    int           m_bActive;      // 0x02c
};

constexpr auto PLAIN_FADE_KIND     = 2;
constexpr auto PLAIN_FADE_DURATION = 500;

// SYNTHETIC: HYPERX 0x0042ce40
// PaletteFade::`scalar deleting destructor'

#endif
