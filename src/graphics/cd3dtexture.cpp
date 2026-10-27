#include "common/compat.h"
#include "graphics/cd3dtexture.h"
#include "common/pathstring.h"
#include "physics/physicsbody.h"
#include "meshbone.h"
#include "physics/boundbubble.h"
#include "globals.h"
#include "graphics/renderstate.h"
#include "graphics/gdi.h"
#include "texture.h"
#include "graphicsglobals.h"
#include <ddraw.h>
#include <stdlib.h>
#include <string.h>
#include "app/gameapp.h"
#include "graphics/ddrawapi.h"
#include "engine/object.h"

// The original built this unit without optimisation.

#pragma optimize("", off)

// FUNCTION: HYPERX 0x004339a0
CD3DTexture::CD3DTexture(IDirectDraw* pDDraw, IDirectDrawSurface* pPrimary, IDirectDrawSurface* pBack)
: Texture(pDDraw, pPrimary, pBack)
{
    ClearRecord();
}

// The one caller qualifies the call, or it becomes the inlined scalar deleting destructor.
// FUNCTION: HYPERX 0x00433a20
CD3DTexture::~CD3DTexture()
{
    if (m_pDest != 0)
    {
        m_pDest->Release();

        m_pDest = 0;
    }

    if (m_pDestSurface != 0)
    {
        m_pDestSurface->Release();

        m_pDestSurface = 0;
    }
}

#pragma optimize("", on)

#pragma optimize("", off)

// FUNCTION: HYPERX 0x00433ae0
void CD3DTexture::ClearRecord()
{
    m_strName.Empty();

    m_pBitmapRecord   = 0;
    m_pPixels         = 0;
    m_hPalette        = 0;
    m_nRowBytes       = 0;
    m_nWidthLog2      = 0;
    m_nRowMask        = 0;
    m_nColumnBytes    = 0;
    m_nHeightLog2     = 0;
    m_nColumnMask     = 0;
    m_bBottomUp       = 0;
    m_pTextureSurface = 0;
    m_dwHandle        = 0;
    m_pDest           = 0;
    m_pDestSurface    = 0;
}

#pragma optimize("", on)

#pragma optimize("", off)

// FUNCTION: HYPERX 0x00433bb0
int CD3DTexture::Create(const char* pszName, int bColourKey, int bSystemMemory)
{
    int nSurfaceResult;

    if (g_d3dState.nTextureFormat < 0)
    {
        // STRING: HYPERX 0x004bd928
        TRACE("CD3DTexture::Create - No texture formats available!\n Create Failed!\n");

        return 0;
    }

    // A second name keeps the client's schedule; without it 49.65.
    String* const pName = &m_strName;

    pName->Assign(String::SafeStrlen(pszName), pszName);

    if (LoadBitmapFile(m_strName) != 0)
    {
        m_ptSize.x = m_pBitmapRecord->biWidth;
        m_ptSize.y = m_pBitmapRecord->biHeight;
    }
    else
    {
        // STRING: HYPERX 0x004bd8f4
        g_gameApp.Error("CD3DTexture::Create FAILED! No BitMap loaded!\n%s\n", pszName);

        return 0;
    }

    nSurfaceResult = Texture::AllocSurface(m_ptSize,
                                           bColourKey,
                                           bSystemMemory);

    if (m_pSurface == 0)
    {
        return 0;
    }

    m_hr = m_pSurface->QueryInterface(g_iidTextureSurface, (void**)&m_pTextureSurface);

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bd8bc
        TRACE("CD3DTexture::Create - Could not create texture %s.\n%s\n",
              pszName,
              DDraw::ErrorToString(m_hr));

        return 0;
    }

    if (g_bEmulationOnly != 0)
    {
        m_bSystemMemory = 1;
    }
    else
    {
        m_bSystemMemory = 0;
    }

    if (CreateVidTexture(m_pSurface, &m_pDestSurface, &m_pDest) == 0)
    {
        return 0;
    }

    m_hr = m_pDest->GetHandle(g_d3dState.pDevice, &m_dwHandle);

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bd890
        TRACE("Could not get from dest texture handle.\n%s\n",
              DDraw::ErrorToString(m_hr));

        return 0;
    }

    m_hr = m_pDest->Load(m_pTextureSurface);

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bd864
        TRACE("Could not load src texture into dest.\n%s\n",
              DDraw::ErrorToString(m_hr));

        return 0;
    }

    if (m_pTextureSurface != 0)
    {
        m_pTextureSurface->Release();

        m_pTextureSurface = 0;
    }

    if (m_pSurface != 0)
    {
        m_pSurface->Release();

        m_pSurface = 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x00433ed0
int CD3DTexture::OnSurfaceLost()
{
    return RestoreContents();
}

#pragma optimize("", on)

#pragma optimize("", off)

// FUNCTION: HYPERX 0x00433ef0
int CD3DTexture::Restore(const char* pName)
{
    if (LoadBitmapFile(pName) != 0)
    {
        m_ptSize.x = m_pBitmapRecord->biWidth;
        m_ptSize.y = m_pBitmapRecord->biHeight;
    }
    else
    {
        // STRING: HYPERX 0x004bda94
        TRACE("CD3DTexture::RestoreContents FAILED! No BitMap loaded!\n");

        return 0;
    }

    Texture::AllocSurface(m_ptSize,
                          m_bColourKey,
                          1);

    if (m_pSurface == 0)
    {
        // STRING: HYPERX 0x004bda50
        TRACE("CD3DTexture::RestoreContents FAILED! No source surface created!\n");

        return 0;
    }

    m_hr = m_pSurface->QueryInterface(g_iidTextureSurface, (void**)&m_pTextureSurface);

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bda10
        TRACE("CD3DTexture::RestoreContents - QueryInterface failed!\n%s\n%s\n",
              pName,
              DDraw::ErrorToString(m_hr));

        return 0;
    }

    m_hr = m_pDestSurface->Restore();

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bd9bc
        TRACE("CD3DTexture::RestoreContents - Could not restore destination texture surface.\n%s\n",
              DDraw::ErrorToString(m_hr));

        if (m_pTextureSurface != 0)
        {
            m_pTextureSurface->Release();
            m_pTextureSurface = 0;
        }

        if (m_pSurface != 0)
        {
            m_pSurface->Release();
            m_pSurface = 0;
        }

        return 0;
    }

    m_hr = m_pDest->Load(m_pTextureSurface);

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bd970
        TRACE("CD3DTexture::RestoreContents - Could not load src texture into dest.\n%s\n",
              DDraw::ErrorToString(m_hr));

        if (m_pTextureSurface != 0)
        {
            m_pTextureSurface->Release();
            m_pTextureSurface = 0;
        }

        if (m_pSurface != 0)
        {
            m_pSurface->Release();
            m_pSurface = 0;
        }

        return 0;
    }

    if (m_pTextureSurface != 0)
    {
        m_pTextureSurface->Release();
        m_pTextureSurface = 0;
    }

    if (m_pSurface != 0)
    {
        m_pSurface->Release();
        m_pSurface = 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x00434220
int CD3DTexture::RestoreContents()
{
    if (!m_strName.IsEmpty())
    {
        return Restore(m_strName);
    }

    return 0;
}

#pragma optimize("", on)

// The original swaps the red and blue divisors.

struct IDirectDrawSurface;

#define TEXTURE_SURFACE_FLAGS (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT)
#define TEXTURE_SURFACE_CAPS  (DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY)

#pragma optimize("", off)

// FUNCTION: HYPERX 0x00434260
IDirectDrawSurface* CD3DTexture::CreateSurface(int nWidth, int nHeight, int, int)
{
    int                  nColourCount;
    unsigned int         dwPackedRGB;
    DDSURFACEDESC        lockDesc;
    LPDIRECTDRAWPALETTE  pPalette;
    PALETTEENTRY         paletteEntries[256];
    int                  nRowCount;
    int                  nI;
    DDSURFACEDESC        sd;
    LPDIRECTDRAWSURFACE  pSurfacePtr;
    int                  nMaxColours;
    int                  nPaletteCaps;
    const unsigned char* pPalSource;
    unsigned char*       pBytes;
    int                  nBitShift;
    int                  nGreenDiv;
    unsigned int         aColourArray[256];
    unsigned char*       pDest;
    int                  bPaletted;
    const unsigned char* pRead;
    unsigned short*      pRow16;
    int                  nShiftBlue;
    unsigned int*        pRowOf32;
    int                  nAcross;
    unsigned int         dwChannelMask;
    int                  nRedUp;
    int                  nBDivisor;
    int                  nShiftG;
    int                  nRowY;
    int                  nDivRed;

    pSurfacePtr = 0;
    bPaletted   = 0;

    if (m_pSurface != 0)
    {
        m_pSurface->Release();

        m_pSurface = 0;
    }

    sd = g_d3dState.aTextureFormats[g_d3dState.nTextureFormat];

    if ((sd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) != 0)
    {
        bPaletted    = 1;
        nMaxColours  = 256;
        nPaletteCaps = DDPCAPS_8BIT | DDPCAPS_ALLOW256;
    }
    else if ((sd.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) != 0)
    {
        bPaletted    = 1;
        nMaxColours  = 16;
        nPaletteCaps = DDPCAPS_4BIT;
    }

    sd.dwSize         = sizeof(sd);
    sd.dwFlags        = TEXTURE_SURFACE_FLAGS;
    sd.ddsCaps.dwCaps = TEXTURE_SURFACE_CAPS;
    sd.dwHeight       = nHeight;
    sd.dwWidth        = nWidth;

    m_hr = g_pSurfaceDDraw->CreateSurface(&sd, &pSurfacePtr, 0);

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bdbc8
        g_gameApp.Error("CD3DTexture::CreateSurface FAILED!\n%s\n",
                        DDraw::ErrorToString(m_hr));

        pSurfacePtr = 0;

        return pSurfacePtr;
    }

    memset(&lockDesc, 0, sizeof(lockDesc));

    lockDesc.dwSize = sizeof(lockDesc);

    m_hr = pSurfacePtr->Lock(0, &lockDesc, 0, 0);

    if (m_hr != 0)
    {
        pSurfacePtr->Release();

        // STRING: HYPERX 0x004bdb98
        TRACE("Lock failed while loading texture surface.\n%s\n",
              DDraw::ErrorToString(m_hr));

        return 0;
    }

    if (bPaletted == 0)
    {
        for (nBitShift = 0, dwChannelMask = sd.ddpfPixelFormat.dwRBitMask; (dwChannelMask & 1) == 0;
             nBitShift++, dwChannelMask >>= 1)
        {
        }

        nRedUp  = nBitShift;
        nDivRed = 255 / (sd.ddpfPixelFormat.dwRBitMask >> nBitShift);

        for (nBitShift = 0, dwChannelMask = sd.ddpfPixelFormat.dwGBitMask; (dwChannelMask & 1) == 0;
             nBitShift++, dwChannelMask >>= 1)
        {
        }

        nShiftG   = nBitShift;
        nGreenDiv = 255 / (sd.ddpfPixelFormat.dwGBitMask >> nBitShift);

        for (nBitShift = 0, dwChannelMask = sd.ddpfPixelFormat.dwBBitMask; (dwChannelMask & 1) == 0;
             nBitShift++, dwChannelMask >>= 1)
        {
        }

        nShiftBlue = nBitShift;
        nBDivisor  = 255 / (sd.ddpfPixelFormat.dwBBitMask >> nBitShift);

        pRead = (const unsigned char*)m_pPixels;

        switch (sd.ddpfPixelFormat.dwRGBBitCount)
        {
            case 32:
                for (nRowY = 0; nRowY < m_pBitmapRecord->biHeight; nRowY++)
                {
                    pRowOf32 = (unsigned int*)((char*)lockDesc.lpSurface + lockDesc.lPitch * nRowY);

                    for (nAcross = 0; nAcross < m_pBitmapRecord->biWidth; nAcross++)
                    {
                        int nRedDivided;
                        int nBlueDivided;
                        int nGreenDivided;

                        nBlueDivided  = *pRead++ / nDivRed;
                        nGreenDivided = *pRead++ / nGreenDiv;
                        nRedDivided   = *pRead++ / nBDivisor;

                        *pRowOf32++ = (nRedDivided << nRedUp) | (nGreenDivided << nShiftG) | (nBlueDivided << nShiftBlue);
                    }
                }
                break;

            case 16:
                for (nRowY = 0; nRowY < m_pBitmapRecord->biHeight; nRowY++)
                {
                    pRow16 = (unsigned short*)((char*)lockDesc.lpSurface + lockDesc.lPitch * nRowY);

                    for (nAcross = 0; nAcross < m_pBitmapRecord->biWidth; nAcross++)
                    {
                        int nRedDivided;
                        int nBlueDivided;
                        int nGreenDivided;

                        nBlueDivided  = *pRead++ / nDivRed;
                        nGreenDivided = *pRead++ / nGreenDiv;
                        nRedDivided   = *pRead++ / nBDivisor;

                        *pRow16++ = (unsigned short)((nRedDivided << nRedUp) | (nGreenDivided << nShiftG) | (nBlueDivided << nShiftBlue));
                    }
                }
                break;

            case 8:
                for (nRowY = 0; nRowY < m_pBitmapRecord->biHeight; nRowY++)
                {
                    pBytes = (unsigned char*)lockDesc.lpSurface + lockDesc.lPitch * nRowY;

                    for (nAcross = 0; nAcross < m_pBitmapRecord->biWidth; nAcross++)
                    {
                        int nRedDivided;
                        int nBlueDivided;
                        int nGreenDivided;

                        nBlueDivided  = *pRead++ / nDivRed;
                        nGreenDivided = *pRead++ / nGreenDiv;
                        nRedDivided   = *pRead++ / nBDivisor;

                        *pBytes++ = (unsigned char)((nRedDivided << nRedUp) | (nGreenDivided << nShiftG) | (nBlueDivided << nShiftBlue));
                    }
                }
                break;

            default:
                pSurfacePtr->Unlock(0);
                pSurfacePtr->Release();

                free(m_pBitmapRecord);

                m_pBitmapRecord = 0;

                // STRING: HYPERX 0x004bdb68
                TRACE("Unknown pixel format while loading texture. %d\n",
                      sd.ddpfPixelFormat.dwRGBBitCount);

                return 0;
        }

        pSurfacePtr->Unlock(0);

        free(m_pBitmapRecord);

        m_pBitmapRecord = 0;

        return pSurfacePtr;
    }
    else
    {
        pPalette     = 0;
        nColourCount = 0;

        pPalSource = (const unsigned char*)m_pPixels;

        for (nRowCount = 0; nRowCount < m_pBitmapRecord->biHeight; nRowCount++)
        {
            pDest = (unsigned char*)lockDesc.lpSurface + lockDesc.lPitch * nRowCount;

            for (nI = 0; nI < m_pBitmapRecord->biWidth; nI++)
            {
                int nRedByte;
                int nBlueByte;
                int nGreenByte;
                int nEntry;

                nBlueByte  = *pPalSource++;
                nGreenByte = *pPalSource++;
                nRedByte   = *pPalSource++;

                dwPackedRGB = (nRedByte << 16) | (nGreenByte << 8) | nBlueByte;

                for (nEntry = 0; nEntry < nColourCount; nEntry++)
                {
                    if (dwPackedRGB == aColourArray[nEntry])
                    {
                        break;
                    }
                }

                if (nEntry == nColourCount)
                {
                    nColourCount++;

                    if (nColourCount > nMaxColours)
                    {
                        nColourCount--;
                        nEntry = nColourCount - 1;
                    }

                    aColourArray[nEntry] = dwPackedRGB;
                }

                if (nMaxColours == 16)
                {
                    if ((nI & 1) == 0)
                    {
                        *pDest = (unsigned char)(nEntry & 0xf);
                    }
                    else
                    {
                        *pDest |= (unsigned char)((nEntry & 0xf) << 4);

                        pDest++;
                    }
                }
                else
                {
                    *pDest = (unsigned char)nEntry;

                    pDest++;
                }
            }
        }

        pSurfacePtr->Unlock(0);

        free(m_pBitmapRecord);

        m_pBitmapRecord = 0;

        if (nColourCount > nMaxColours)
        {
            pSurfacePtr->Unlock(0);
            pSurfacePtr->Release();

            // STRING: HYPERX 0x004bdb40
            TRACE("Palette burst while loading texture!\n");

            return 0;
        }

        memset(paletteEntries, 0, sizeof(paletteEntries));

        for (nI = 0; nI < nColourCount; nI++)
        {
            paletteEntries[nI].peRed   = (unsigned char)((aColourArray[nI] >> 16) & 0xff);
            paletteEntries[nI].peGreen = (unsigned char)((aColourArray[nI] >> 8) & 0xff);
            paletteEntries[nI].peBlue  = (unsigned char)(aColourArray[nI] & 0xff);
        }

        for (; nI < 256; nI++)
        {
            paletteEntries[nI].peFlags = D3DPAL_RESERVED;
        }

        m_hr = g_pSurfaceDDraw->CreatePalette(nPaletteCaps, paletteEntries, &pPalette, 0);

        if (m_hr != 0)
        {
            pSurfacePtr->Release();

            // STRING: HYPERX 0x004bdb04
            TRACE("Create palette failed while loading texture surface.\n%s\n",
                  DDraw::ErrorToString(m_hr));

            return 0;
        }

        m_hr = pSurfacePtr->SetPalette(pPalette);

        if (m_hr != 0)
        {
            pSurfacePtr->Release();
            pPalette->Release();

            // STRING: HYPERX 0x004bdacc
            TRACE("SetPalette failed while loading texture surface.\n%s\n",
                  DDraw::ErrorToString(m_hr));

            return 0;
        }

        pPalette->Release();

        return pSurfacePtr;
    }
}

#pragma optimize("", on)

// The original's palette arm is broken: m_hPalette is null and the entries are stack garbage.

struct IDirectDrawSurface;

#define VIDTEXTURE_FLAGS        (DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT)
#define VIDTEXTURE_CAPS         (DDSCAPS_ALLOCONLOAD | DDSCAPS_TEXTURE)
#define VIDTEXTURE_PALETTE_CAPS (DDPCAPS_8BIT | DDPCAPS_ALLOW256)

#pragma optimize("", off)

// FUNCTION: HYPERX 0x00434f90
int CD3DTexture::CreateVidTexture(IDirectDrawSurface* pSource, IDirectDrawSurface** ppSurface, IDirect3DTexture** ppTexture)
{
    DDSURFACEDESC desc;
    PALETTEENTRY  aEntries[256];
    int           bHasPalette;

    LPDIRECTDRAWPALETTE pPalIface = 0;

    if (DDraw::GetSurfaceDesc(&desc, pSource) == 0)
    {
        return 0;
    }

    if ((desc.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) != 0)
    {
        bHasPalette = 1;
    }
    else
    {
        bHasPalette = 0;
    }

    desc.dwFlags        = VIDTEXTURE_FLAGS;
    desc.ddsCaps.dwCaps = VIDTEXTURE_CAPS;

    if (m_bSystemMemory != 0)
    {
        desc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
    }

    m_hr = g_pSurfaceDDraw->CreateSurface(&desc, ppSurface, 0);

    if (m_hr != 0)
    {
        // STRING: HYPERX 0x004bdc7c
        TRACE("Create surface in CreateVidTexture failed.\n%s\n",
              DDraw::ErrorToString(m_hr));

        return 0;
    }

    if (DDraw::GetSurfaceDesc(&desc, *ppSurface) == 0)
    {
        return 0;
    }

    if (bHasPalette != 0)
    {
        const int nGotEntries = GetPaletteEntries(m_hPalette, 0, 256, aEntries);

        m_hr =
            g_ddrawDevice.pDDrawObject->CreatePalette(VIDTEXTURE_PALETTE_CAPS, aEntries, &pPalIface, 0);

        if (m_hr != 0)
        {
            if (*ppSurface != 0)
            {
                (*ppSurface)->Release();

                *ppSurface = 0;
            }

            // STRING: HYPERX 0x004bdc4c
            TRACE("CreatePalette in CreateVidTexture failed.\n%s\n",
                  DDraw::ErrorToString(m_hr));

            return 0;
        }

        m_hr = (*ppSurface)->SetPalette(pPalIface);

        if (m_hr != 0)
        {
            if (*ppSurface != 0)
            {
                (*ppSurface)->Release();

                *ppSurface = 0;
            }

            if (pPalIface != 0)
            {
                pPalIface->Release();

                pPalIface = 0;
            }

            // STRING: HYPERX 0x004bdc20
            TRACE("SetPalette in CreateVidTexture failed.\n%s\n",
                  DDraw::ErrorToString(m_hr));

            return 0;
        }
    }

    m_hr = (*ppSurface)->QueryInterface(g_iidTextureSurface, (void**)ppTexture);

    if (m_hr != 0)
    {
        if (*ppSurface != 0)
        {
            (*ppSurface)->Release();

            *ppSurface = 0;
        }

        if (pPalIface != 0)
        {
            pPalIface->Release();

            pPalIface = 0;
        }

        // STRING: HYPERX 0x004bdbf0
        TRACE("QueryInterface in CreateVidTexture failed.\n%s\n",
              DDraw::ErrorToString(m_hr));

        return 0;
    }

    return 1;
}

#pragma optimize("", on)

#pragma optimize("", off)

// FUNCTION: HYPERX 0x00435310
unsigned char CD3DTexture::LoadBitmapFile(const char* pszName)
{
    // Three slots the original reserves and never reads.
    int nUnusedA;
    int nUnusedB;
    int nUnusedC;

    (void)nUnusedA;
    (void)nUnusedB;
    (void)nUnusedC;

    m_pBitmapRecord = GDI::ReadBitmapFile(pszName);

    if (m_pBitmapRecord != 0)
    {
        if (m_pBitmapRecord->biBitCount == 8 || m_pBitmapRecord->biBitCount == 24)
        {
            m_nWidthLog2 = CD3DTexture::GetLog2Size(m_pBitmapRecord->biWidth);

            if (m_nWidthLog2 == 0)
            {
                // STRING: HYPERX 0x004bdd84
                g_gameApp.Fatal("Texture Map %s width is not a power of 2!", pszName);

                return 0;
            }

            m_bBottomUp = m_pBitmapRecord->biHeight > 0;

            // BI_BITFIELDS carries three DWORD masks; anything else carries biClrUsed RGBQUADs.
            if (m_pBitmapRecord->biCompression == BI_BITFIELDS)
            {
                m_pPixels = (char*)((DWORD*)GDI::ColorTable(m_pBitmapRecord) + 3);
            }
            else
            {
                m_pPixels = (char*)(GDI::ColorTable(m_pBitmapRecord) + m_pBitmapRecord->biClrUsed);
            }

            m_nRowBytes = ((m_pBitmapRecord->biWidth + 3) & ~3) * (m_pBitmapRecord->biBitCount / 8);

            m_nRowMask = m_nRowBytes - 1;

            m_nHeightLog2 = CD3DTexture::GetLog2Size(m_pBitmapRecord->biHeight);

            if (m_nHeightLog2 == 0)
            {
                // STRING: HYPERX 0x004bdd58
                g_gameApp.Fatal("Texture Map %s height is not a power of 2!", pszName);

                return 0;
            }

            m_nColumnBytes = ((m_pBitmapRecord->biHeight + 3) & ~3) * (m_pBitmapRecord->biBitCount / 8);

            m_nColumnMask = m_nColumnBytes - 1;
        }
        else
        {
            // STRING: HYPERX 0x004bdd2c
            g_gameApp.Error("Texture Map %s is a %d bit/pixel bit map!\n", pszName, m_pBitmapRecord->biBitCount);

            return 0;
        }

        if (m_pBitmapRecord->biBitCount == 8)
        {
            // STRING: HYPERX 0x004bdce8
            g_gameApp.Error("Texture Map is a %d bit/pixel bit map! Expecting 24 bit/pixel!\n%s\n",
                            m_pBitmapRecord->biBitCount,
                            pszName);

            return 0;

            // Two unknown unreachable statements the original leaves behind this return.
            TRACE("Texture Map is a 8 bit/pixel bit map!\n");

            m_pBitmapRecord = 0;
        }

        if (m_pBitmapRecord->biBitCount == 24)
        {
            // STRING: HYPERX 0x004bdcd0
            TRACE("Texture Map is 24 bit.\n");
        }
    }
    else
    {
        // STRING: HYPERX 0x004bdcac
        g_gameApp.Fatal("Texture Map %s cannot be opened!", pszName);

        return 0;
    }

    return 1;
}

#pragma optimize("", on)

// 1012 stands where 1024 belongs, so a texture 1024 across is a fatal error.
#pragma optimize("", off)

// FUNCTION: HYPERX 0x004355e0
int __cdecl CD3DTexture::GetLog2Size(unsigned int nSize)
{
    switch (nSize)
    {
        case 2:
            return 1;

        case 4:
            return 2;

        case 8:
            return 3;

        case 16:
            return 4;

        case 32:
            return 5;

        case 64:
            return 6;

        case 128:
            return 7;

        case 256:
            return 8;

        case 512:
            return 9;

        case 1012:
            return 10;

        case 2048:
            return 11;

        case 4096:
            return 12;

        case 8192:
            return 13;

        case 16384:
            return 14;

        case 32768:
            return 15;
    }

    return 0;
}

#pragma optimize("", on)
