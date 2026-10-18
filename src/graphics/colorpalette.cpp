#include "common/compat.h"
#include "common/path.h"
#include "graphics/bitmapformat.h"
#include "globals.h"
#include "colorpalette.h"
#include "graphicsglobals.h"
#include "common/pathstring.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include "app/gameapp.h"
#include "graphics/cgpalette.h"
#include "graphics/meshmaterial.h"

// Set while the renderer draws the drome.
// GLOBAL: HYPERX 0x004ce984
extern int g_bDromePass;

// GLOBAL: HYPERX 0x004b40fc
extern const char* const g_pszPaletteExtension;

// The version a definition file may always claim, whatever the palette's own is.
#define PALETTE_VERSION_ANY 0x302

// The name off the first line has no length check of its own.
#define CUSTOM_NAME_MAX 80

// FUNCTION: HYPERX 0x004255e0
ColorPalette::ColorPalette()
: m_current()
, m_rampBounds(0x20, 0xef)
, m_smallRampBounds(0x80, 0xef)
, m_largeRampBounds(0x20, 0x7f)
, m_rampLength(7, 0xf)
, m_arenaRange(0x20, 0x40)
, m_objectRange(0x60, 0x20)
, m_teamARange(0x80, 0x20)
, m_teamBRange(0xa0, 0x20)
, m_rokRange(0xc0, 8)
, m_openRange(0xc8, 8)
, m_generalRange(0xd0, 0x10)
, m_remapRange(m_teamARange.nFirst, m_teamARange.nCount + m_teamARange.nFirst - 1)
{
    m_wVersion = 0x303;

    Load();
}

// FUNCTION: HYPERX 0x00425750
PaletteRange ColorPalette::GetFrameRange(int nKind)
{
    PaletteRange r;
    int          nFirst;
    int          nCount;

    switch (nKind)
    {
        case 1:
            nFirst = m_arenaRange.nFirst;
            nCount = m_arenaRange.nCount;
            break;

        case 2:
            nFirst = m_arenaRange.nFirst;
            nCount = m_rampLength.nCount + 1;
            break;

        case 3:
            nFirst = m_arenaRange.nFirst + m_rampLength.nCount + 1;
            nCount = m_rampLength.nCount + 1;
            break;

        case 4:
            nFirst = m_arenaRange.nFirst + (m_rampLength.nCount + 1) * 2;
            nCount = m_rampLength.nCount + 1;
            break;

        case 5:
            nFirst = (m_rampLength.nCount + 1) * 3 + m_arenaRange.nFirst;
            nCount = m_rampLength.nCount + 1;
            break;

        case 6:
            nFirst = m_objectRange.nFirst;
            nCount = m_objectRange.nCount;
            break;

        case 7:
            nFirst = m_objectRange.nFirst;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 8:
            nFirst = m_objectRange.nFirst + m_rampLength.nFirst + 1;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 9:
            nFirst = m_objectRange.nFirst + (m_rampLength.nFirst + 1) * 2;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 10:
            nFirst = (m_rampLength.nFirst + 1) * 3 + m_objectRange.nFirst;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 11:
            nFirst = m_teamARange.nFirst;
            nCount = m_teamARange.nCount;
            break;

        case 12:
            nFirst = m_teamARange.nFirst;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 13:
            nFirst = m_teamARange.nFirst + m_rampLength.nFirst + 1;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 14:
            nFirst = m_teamARange.nFirst + (m_rampLength.nFirst + 1) * 2;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 15:
            nFirst = (m_rampLength.nFirst + 1) * 3 + m_teamARange.nFirst;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 16:
            nFirst = m_teamBRange.nFirst;
            nCount = m_teamBRange.nCount;
            break;

        case 17:
            nFirst = m_teamBRange.nFirst;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 18:
            nFirst = m_teamBRange.nFirst + m_rampLength.nFirst + 1;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 19:
            nFirst = m_teamBRange.nFirst + (m_rampLength.nFirst + 1) * 2;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 20:
            nFirst = (m_rampLength.nFirst + 1) * 3 + m_teamBRange.nFirst;
            nCount = m_rampLength.nFirst + 1;
            break;

        case 21:
            nFirst = m_rokRange.nFirst;
            nCount = m_rokRange.nCount;
            break;

        case 22:
            nFirst = m_openRange.nFirst;
            nCount = m_openRange.nCount;
            break;

        case 23:
            nFirst = m_generalRange.nFirst;
            nCount = m_generalRange.nCount;
            break;
        default:
            nFirst = 0;
            nCount = 0;
            break;
    }

    r.nFirst = nFirst;
    r.nCount = nCount;

    return r;
}

// The original matched the colours against a palettised desktop's static ones; on a true-colour
// desktop that left them as they were, which is all this keeps.
// FUNCTION: HYPERX 0x00425a90
void ColorPalette::Build()
{
    for (int i = 0; i < 256; i++)
    {
        if (m_entries[i].peFlags != PALETTE_RESERVED)
        {
            m_entries[i].peFlags = PALETTE_NOCOLLAPSE;
        }

        m_current[i] = m_entries[i];
    }
}

// FUNCTION: HYPERX 0x00425d50
void ColorPalette::ReloadPalette(short nMode)
{
    if (nMode == 3)
    {
        Build();
    }

    SyncDIBColorTable(0, 0x100);
}

int ColorPalette::NearestIndex(std::uint32_t colour, int nSkip) const
{
    auto nBest     = 0;
    auto nBestDist = 0x7fffffff;

    for (int i = 0; i < 256; i++)
    {
        if (i == nSkip)
        {
            continue;
        }

        const int dr    = (int)m_current[i].peRed - (int)(colour & 0xff);
        const int dg    = (int)m_current[i].peGreen - (int)(colour >> 8 & 0xff);
        const int db    = (int)m_current[i].peBlue - (int)(colour >> 16 & 0xff);
        const int nDist = dr * dr + dg * dg + db * db;

        if (nDist < nBestDist)
        {
            nBestDist = nDist;
            nBest     = i;
        }
    }

    return nBest;
}

// A version mismatch is fatal and returns 1; any other failure returns nought.
// FUNCTION: HYPERX 0x00425ec0
unsigned int ColorPalette::LoadCustomPalette()
{
    char      szName[CUSTOM_NAME_MAX];
    GifScreen screen;
    int       nCount;
    char      c;

    // STRING: HYPERX 0x004bc92c
    FILE* pFile = fopen(Path::Native(String::Concat(g_gameApp.m_strExeDir.m_str, "Custom.pal")).c_str(), "r");

    if (pFile == nullptr)
    {
        return 0;
    }

    // STRING: HYPERX 0x004bb810
    if (fscanf(pFile, "%s", szName) == -1)
    {
        return 0;
    }

    // The client holds the name in EBP (0x00425f22).
    String* const pName = &m_name;

    pName->Assign(String::SafeStrlen(szName), szName);

    {
        unsigned short wVersion = 0;

        c = (char)getc(pFile);

        while (c != '\n')
        {
            c = (char)getc(pFile);
        }

        ungetc(c, pFile);

        // STRING: HYPERX 0x004bc988
        if (fscanf(pFile, "%hu %hu", &wVersion, &m_wNumEntries) == -1)
        {
            return 0;
        }

        if (m_wVersion != wVersion)
        {
            // STRING: HYPERX 0x004bc944
            g_gameApp.Fatal("Palette definition file contains wrong version %3x.\nExpecting %3x.", wVersion, m_wVersion & 0xffff);

            return 1;
        }
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    // STRING: HYPERX 0x004bc93c
    if (fscanf(pFile, "%d %d", &m_rampBounds.nFirst, &m_rampBounds.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_smallRampBounds.nFirst, &m_smallRampBounds.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_largeRampBounds.nFirst, &m_largeRampBounds.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_rampLength.nFirst, &m_rampLength.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_arenaRange.nFirst, &m_arenaRange.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_objectRange.nFirst, &m_objectRange.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_teamARange.nFirst, &m_teamARange.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_teamBRange.nFirst, &m_teamBRange.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_rokRange.nFirst, &m_rokRange.nCount) == -1)
    {
        return 0;
    }

    c = (char)getc(pFile);

    while (c != '\n')
    {
        c = (char)getc(pFile);
    }

    ungetc(c, pFile);

    if (fscanf(pFile, "%d %d", &m_openRange.nFirst, &m_openRange.nCount) == -1)
    {
        return 0;
    }

    fclose(pFile);

    // STRING: HYPERX 0x004bc938
    pFile = fopen(Path::Native(String::Concat(g_gameApp.m_strExeDir.m_str, pName->m_pchData)).c_str(), "rb");

    if (pFile != nullptr)
    {
        nCount = CGPalette::ReadGifScreen(&screen, pFile);

        fclose(pFile);

        if (nCount > 0)
        {
            PaletteEntry* pEntry = m_entries;

            for (short i = 0; i < nCount; i++)
            {
                pEntry->peFlags = 0;
                pEntry->peBlue  = screen.aColours[i][2];
                pEntry->peGreen = screen.aColours[i][1];
                pEntry->peRed   = screen.aColours[i][0];
                pEntry++;
            }
        }

        return nCount;
    }

    return 0;
}

// Only a count of 256 or one is quiet; the count is returned either way.
// FUNCTION: HYPERX 0x00426350
unsigned int ColorPalette::Load()
{
    const unsigned int nResult = LoadCustomPalette();

    if (nResult != 0x100)
    {
        switch (nResult)
        {
            case 0:
                // STRING: HYPERX 0x004bc994
                g_gameApp.Fatal("Palette definition file not found or corrupt.");
                break;

            case 1:
                break;

            default:
                // STRING: HYPERX 0x004bc9c4
                g_gameApp.Fatal("Palette definition file corrupt.");
                break;
        }
    }

    return nResult;
}

// The drome pass raises the ramp base to the light level and takes one off.
// FUNCTION: HYPERX 0x004263a0
unsigned char ColorPalette::LookupColorIntensity(ColorPalette* pPalette, unsigned char cShade, float flIntensity, unsigned char cColour)
{
    switch (cShade)
    {
        case SHADE_BLACK:
            return cColour;

        case SHADE_WHITE:
            if (cColour >= pPalette->m_largeRampBounds.nFirst && cColour <= pPalette->m_largeRampBounds.nCount)
            {
                return (unsigned char)(pPalette->m_rampLength.nCount + cColour);
            }

            return (unsigned char)(pPalette->m_rampLength.nFirst + cColour);

        case SHADE_SCALED:
            if (g_bDromePass != 0)
            {
                if (cColour < pPalette->m_largeRampBounds.nFirst || cColour > pPalette->m_largeRampBounds.nCount)
                {
                    const int nRampOut = pPalette->m_rampLength.nFirst + 1;

                    return (unsigned char)((int)pow((double)nRampOut, static_cast<double>(flIntensity)) + cColour - 1);
                }
                else
                {
                    const int nRampIn = pPalette->m_rampLength.nCount + 1;

                    return (unsigned char)((int)pow((double)nRampIn, static_cast<double>(flIntensity)) + cColour - 1);
                }
            }
            if (cColour >= pPalette->m_largeRampBounds.nFirst && cColour <= pPalette->m_largeRampBounds.nCount)
            {
                return (unsigned char)((int)(pPalette->m_rampLength.nCount * flIntensity) + cColour);
            }

            return (unsigned char)((int)(pPalette->m_rampLength.nFirst * flIntensity) + cColour);
    }

    return 0;
}

// FUNCTION: HYPERX 0x004264a0
int ColorPalette::LoadPaletteEntries(const char* pszName, char bRemap)
{
    int          nFileVersion = 0;
    int          nCount       = 0;
    PathString   strPath(MAX_PATH_LENGTH, g_gameApp.m_strModelPath.m_str.m_pchData, pszName, g_pszPaletteExtension);
    unsigned int nRed;
    unsigned int nGreen;
    unsigned int nBlue;

    // STRING: HYPERX 0x004bc990
    FILE* const pFile = fopen(Path::Native(strPath.m_str.m_pchData).c_str(), "r");

    if (pFile != nullptr)
    {
        // STRING: HYPERX 0x004bc93c
        if (fscanf(pFile, "%d %d", &nFileVersion, &nCount) == -1)
        {
            fclose(pFile);

            return 0;
        }

        if (m_wVersion != nFileVersion && nFileVersion != PALETTE_VERSION_ANY)
        {
            // STRING: HYPERX 0x004bca28
            g_gameApp.Error("Palette definition file %s contains wrong version %3x.\nExpecting %3x. Changes will be ignored.", strPath.m_str.m_pchData, nFileVersion, m_wVersion);

            fclose(pFile);

            return 1;
        }

        char c = (char)getc(pFile);

        while (c != '\n')
        {
            c = (char)getc(pFile);
        }

        ungetc(c, pFile);

        if (nCount > 0)
        {
            for (int i = 0; i < nCount; i++)
            {
                unsigned int nIndex;
                PaletteEntry entry;

                // STRING: HYPERX 0x004bca1c
                if (fscanf(pFile, "%u %u %u %u", &nIndex, &nRed, &nGreen, &nBlue) == -1)
                {
                    fclose(pFile);

                    return 0;
                }

                entry.peRed   = (unsigned char)nRed;
                entry.peGreen = (unsigned char)nGreen;
                entry.peBlue  = (unsigned char)nBlue;
                entry.peFlags = PALETTE_NOCOLLAPSE;

                c = (char)getc(pFile);

                while (c != '\n')
                {
                    c = (char)getc(pFile);
                }

                ungetc(c, pFile);

                if (bRemap != 0)
                {
                    if ((int)nIndex < m_remapRange.nFirst || (int)nIndex > m_remapRange.nCount)
                    {
                        nIndex &= 0xff;
                    }
                    else
                    {
                        nIndex = (m_teamARange.nCount + nIndex) & 0xff;
                    }
                }

                nIndex &= 0xff;

                SetEntries(nIndex, 1, &entry);
            }

            fclose(pFile);
        }
        else
        {
            fclose(pFile);

            return nCount;
        }
    }
    else
    {
        // STRING: HYPERX 0x004bc9e8
        g_gameApp.Error("CGPalette::LoadPaletteEntries could not open %s!", strPath.m_str.m_pchData);
    }

    return nCount;
}

// FUNCTION: HYPERX 0x00426740
int ColorPalette::SetEntries(unsigned int iStart, unsigned int cEntries, PaletteEntry* pEntries)
{
    memcpy(&m_current[iStart], pEntries, cEntries * sizeof(PaletteEntry));
    memcpy(&m_entries[iStart], pEntries, cEntries * sizeof(PaletteEntry));

    SyncDIBColorTable(iStart, cEntries);

    return (int)cEntries;
}

// FUNCTION: HYPERX 0x004267a0
void ColorPalette::SyncDIBColorTable(unsigned int iStart, unsigned int cEntries)
{
    for (unsigned int i = iStart; i < iStart + cEntries; i++)
    {
        g_bmiSoftBackBuffer.bmiColors[i] = RgbQuad{ m_current[i].peBlue, m_current[i].peGreen, m_current[i].peRed, 0 };
    }
}

// FUNCTION: HYPERX 0x00426810
char ColorPalette::AnimateEntries(unsigned int iStart, unsigned int cEntries, PaletteEntry* pEntries)
{
    // As AnimatePalette, only the reserved entries take the new colours.
    for (unsigned int i = 0; i < cEntries; i++)
    {
        auto& current = m_current[iStart + i];

        if (current.peFlags == PALETTE_RESERVED)
        {
            current.peRed   = pEntries[i].peRed;
            current.peGreen = pEntries[i].peGreen;
            current.peBlue  = pEntries[i].peBlue;
        }
    }

    SyncDIBColorTable(iStart, cEntries);

    return 1;
}

// Rises geometrically from the floor to the full colour. The object is never touched.
// FUNCTION: HYPERX 0x00426860
void ColorPalette::BuildFadeTable(PaletteEntry* pEntries, short nCount, PaletteEntry peColour, float flFloor)
{
    auto* pflScale  = new float[nCount];
    *pflScale       = flFloor;
    const int nLast = nCount - 1;

    const auto flRatio = (float)pow(static_cast<double>(1.0f / flFloor), static_cast<double>(1.0f / nLast));

    PaletteEntry* pOut  = pEntries;
    const float   flRed = peColour.peRed;
    pOut->peRed         = (unsigned char)(int)(flRed * flFloor);
    const float flGreen = peColour.peGreen;
    pOut->peGreen       = (unsigned char)(int)(flGreen * flFloor);
    const float flBlue  = peColour.peBlue;
    pOut->peBlue        = (unsigned char)(int)(flBlue * flFloor);
    pOut->peFlags       = peColour.peFlags;
    pOut++;

    for (short i = 1; i < nCount; i++)
    {
        const double dScale = pflScale[i - 1] * flRatio;

        pflScale[i]   = (float)dScale;
        pOut->peRed   = (unsigned char)(int)(flRed * dScale);
        pOut->peGreen = (unsigned char)(int)(flGreen * dScale);
        pOut->peBlue  = (unsigned char)(int)(flBlue * dScale);
        pOut->peFlags = peColour.peFlags;
        pOut++;
    }

    delete[] pflScale;
}

// FUNCTION: HYPERX 0x00426980
PaletteEntry* ColorPalette::GetRemappedEntry(PaletteEntry* pOut, int nIndex)
{
    // The same palette at every call site, but read separately.
    if (nIndex >= g_pColorPalette->m_rampBounds.nFirst && nIndex <= g_pColorPalette->m_rampBounds.nCount)
    {
        if (nIndex >= g_pColorPalette->m_largeRampBounds.nFirst && nIndex <= g_pColorPalette->m_largeRampBounds.nCount)
        {
            *pOut = m_entries[m_rampLength.nCount + nIndex];
            return pOut;
        }

        *pOut = m_entries[m_rampLength.nFirst + nIndex];
        return pOut;
    }

    *pOut = m_entries[nIndex];
    return pOut;
}

// The tail repeats a range test already failed, so it always returns 1.
// FUNCTION: HYPERX 0x004269f0
char ColorPalette::IsEntryUsable(int nIndex)
{
    char bUsable;

    if (m_rampBounds.Holds(nIndex))
    {
        int nStep = 1;

        if (m_largeRampBounds.Holds(nIndex))
        {
            nStep = 0x10;
        }

        if (m_smallRampBounds.Holds(nIndex))
        {
            nStep = 8;
        }

        bUsable = nIndex % nStep == 0;

        return bUsable;
    }

    if (!m_rampBounds.Holds(nIndex))
    {
        bUsable = 1;
    }
    else
    {
        bUsable = 0;
    }

    return bUsable;
}

// FUNCTION: HYPERX 0x004197c0
void ColorPalette::Release()
{
    m_name.FreeBuffer();
}
