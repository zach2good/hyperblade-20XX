#include "common/compat.h"
#include "graphics/ddrawcaps.h"
#include "globals.h"
#include "graphics/renderstate.h"
#ifdef HYPERX_BRINGUP
#include "app/bringup.h"
#define DISPLAY_MODE_MAX 50
#endif
#include "graphicsglobals.h"
#include "d3ddeviceinit.h"
#include "math/vector.h"
#include "app/window.h"
#include "ddrawflipdetect.h"
#include "cddbitmap.h"
#include "ui/radar.h"
#include "damagebar.h"
#include "texture.h"
#include "scrollbitmap.h"
#include "common/pathstring.h"
#include "renderer.h"
#include "engine/cameraobject.h"
#include <ddraw.h>
#include <stdlib.h>
#include <string.h>
#include "app/gameapp.h"
#include "math/point.h"
#include "cddtextsurface.h"
#include "framedbarbitmap.h"
#include "d3ddevice.h"
#include "cd3dtexturemanager.h"
#include "graphics/d3dapi.h"
#include "graphics/d3drend.h"
#include "graphics/d3dviewport.h"
#include "graphics/ddrawapi.h"
#include "common/mem.h"
#include "app/stringids.h"
#include "engine/objectclasses.h"
#include "ui/hud.h"
#include "ui/hudsurfaces.h"
#include "ui/hudglobals.h"
#include "ui/uiglobals.h"
#include "physics/collisionellipsoid.h"
#include "common/string.h"
#include <stdio.h>
#include "common/staticinit.h"

float g_flDotPitchInchProjection      = g_flDotsPerInchProjection * g_flDotPitchProjection;
float g_flScreenDotsPerInchProjection = g_flDotsPerInchProjection;
float g_flDotPitchCmProjection        = g_flDotPitchInchProjection / g_flCmPerInchProjection;
float g_flScreenDotsPerCmProjection   = g_flScreenDotsPerInchProjection / g_flCmPerInchProjection;
float g_flUnusedLightLevelA           = g_flZeroLightLevel;
float g_flUnusedLightLevelB           = g_flZeroLightLevel;
float g_flUnusedLightLevelC           = g_flZeroLightLevel;

extern int          g_bBltColourFill;
extern unsigned int g_nVidMemTotal;
extern int          g_bDoubleResMode;
#define DDSCL_GAME (DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX)

#define DDRAW_EXCLUSIVE_FULLSCREEN (DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWMODEX)

// Written here and read nowhere else.
// GLOBAL: HYPERX 0x004cbb84
extern unsigned int g_nVideoMemoryTotal;

struct IDirectDraw;
struct IDirectDrawSurface;

// FUNCTION: HYPERX 0x00430e40
HRESULT WINAPI DDraw::EnumDisplayModesCallback(DDSURFACEDESC* pDesc, void*)
{
#ifdef HYPERX_BRINGUP
    // The original does not check, and modern DirectDraw lists more than fifty modes.
    if (g_displayState.nDisplayModeCount >= DISPLAY_MODE_MAX)
    {
        return DDENUMRET_CANCEL;
    }
#endif

    if (pDesc->ddpfPixelFormat.dwRGBBitCount == 8 || pDesc->ddpfPixelFormat.dwRGBBitCount == 16)
    {
        g_displayState.aDisplayModes[g_displayState.nDisplayModeCount].nWidth  = pDesc->dwWidth;
        g_displayState.aDisplayModes[g_displayState.nDisplayModeCount].nHeight = pDesc->dwHeight;
        g_displayState.aDisplayModes[g_displayState.nDisplayModeCount].nBpp    = pDesc->ddpfPixelFormat.dwRGBBitCount;
        g_displayState.nDisplayModeCount++;
    }

    return DDENUMRET_OK;
}

// Deepest first, then widest, then tallest.
// FUNCTION: HYPERX 0x00430ea0
int __cdecl DDraw::CompareDisplayModes(const void* pLeft, const void* pRight)
{
    const DisplayModeEntry* pA = (const DisplayModeEntry*)pLeft;
    const DisplayModeEntry* pB = (const DisplayModeEntry*)pRight;

    int nA = pA->nBpp;
    int nB = pB->nBpp;

    if (nB < nA)
    {
        return -1;
    }

    if (nB > nA)
    {
        return 1;
    }

    nA = pA->nWidth;
    nB = pB->nWidth;

    if (nB < nA)
    {
        return -1;
    }

    if (nB > nA)
    {
        return 1;
    }

    nA = pA->nHeight;
    nB = pB->nHeight;

    if (nA > nB)
    {
        return -1;
    }

    return nB > nA;
}

// The scan keeps looking after a hit, so the last matching entry wins.
// FUNCTION: HYPERX 0x00430f00
int __cdecl DDraw::EnumDisplayModes(struct IDirectDraw* pDDraw)
{
    DDrawCaps caps;

    memset(&caps, 0, sizeof(caps));

    caps.dwSize = sizeof(caps);

    g_hrD3DResult = pDDraw->GetCaps((LPDDCAPS)&caps, 0);

    if (g_hrD3DResult != 0)
    {
        g_bFullScreen = 0;

        // STRING: HYPERX 0x004bd510
        g_gameApp.Error("GetCaps failed while enumerating display modes.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    g_nVideoMemoryTotal              = caps.dwVidMemTotal;
    g_displayState.nDisplayModeCount = 0;

    g_hrD3DResult = pDDraw->EnumDisplayModes(0, 0, 0, DDraw::EnumDisplayModesCallback);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004bd4f4
        g_gameApp.Error("EnumDisplayModes failed.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    qsort(g_displayState.aDisplayModes, g_displayState.nDisplayModeCount, sizeof(DisplayModeEntry), DDraw::CompareDisplayModes);

    g_displayState.nDefaultDisplayModeIndex = 0;

    for (int i = 0; i < g_displayState.nDisplayModeCount; i++)
    {
        if (g_displayState.aDisplayModes[i].nWidth == 640 && g_displayState.aDisplayModes[i].nHeight == 480 && g_displayState.aDisplayModes[i].nBpp == 16)
        {
            g_displayState.nDefaultDisplayModeIndex = i;
        }
    }

    g_displayState.nDisplayModeIndex = g_displayState.nDefaultDisplayModeIndex;

    g_displayState.displayMode = g_displayState.aDisplayModes[g_displayState.nDefaultDisplayModeIndex];

    return 1;
}

// Times a flip chain against a blit. Every failure answers nought, "cannot flip".

#define FLIP_PRIMARY_CAPS (DDSCAPS_3DDEVICE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX)
#define BLIT_PRIMARY_CAPS (DDSCAPS_3DDEVICE | DDSCAPS_PRIMARYSURFACE)
#define BLIT_BACK_CAPS    (DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY)

#define TIMED_FRAMES 100

// FUNCTION: HYPERX 0x00439620
unsigned int __cdecl DDraw::DetectFlipCapability(void)
{
    DDSCAPS       caps;
    DDSURFACEDESC desc;
    DDBLTFX       bltfx;

    DWORD dwStart;
    DWORD dwFlipTime;
    DWORD dwBlitTime;
    int   i;

    memset(&desc, 0, sizeof(desc));

    bltfx.dwSize           = sizeof(bltfx);
    desc.dwBackBufferCount = g_nFlipChainCount - 1;
    bltfx.dwFillColor      = 0;
    desc.dwSize            = sizeof(desc);
    desc.dwFlags           = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    desc.ddsCaps.dwCaps    = FLIP_PRIMARY_CAPS;

    g_hrLastResult = g_pDDrawDevice->CreateSurface(&desc, &g_ddrawDevice.pDDrawPrimary, 0);

    if (g_hrLastResult != DD_OK)
    {
        return 0;
    }

    caps.dwCaps = DDSCAPS_BACKBUFFER;

    g_hrLastResult = g_ddrawDevice.pDDrawPrimary->GetAttachedSurface(&caps, &g_ddrawDevice.pBackSurface);

    if (g_hrLastResult != DD_OK)
    {
        g_ddrawDevice.pDDrawPrimary->Release();

        return 0;
    }

    for (i = 0; i < g_nFlipChainCount; i++)
    {
        g_ddrawDevice.pBackSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
        memset(desc.lpSurface, 0, desc.lPitch * desc.dwHeight);
        g_ddrawDevice.pBackSurface->Unlock(0);

        g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Flip(0, DDFLIP_WAIT);

        if (g_hrLastResult != DD_OK)
        {
            goto releasePrimary;
        }
    }

    dwStart = timeGetTime();

    for (i = 0; i < TIMED_FRAMES; i++)
    {
        g_ddrawDevice.pBackSurface->Lock(0, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
        Mem::ZeroBlock192((char*)desc.lpSurface + i);
        g_ddrawDevice.pBackSurface->Unlock(0);

        g_hrLastResult = g_ddrawDevice.pBackSurface->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &bltfx);
        g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Flip(0, DDFLIP_WAIT);

        if (g_hrLastResult != DD_OK)
        {
            goto releasePrimary;
        }
    }

    dwFlipTime = timeGetTime() - dwStart;

    dwStart = timeGetTime();

    for (i = 0; i < TIMED_FRAMES; i++)
    {
        g_ddrawDevice.pBackSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
        Mem::ZeroBlockUnrolled128((char*)desc.lpSurface + i);
        g_ddrawDevice.pBackSurface->Unlock(0);

        g_hrLastResult = g_ddrawDevice.pBackSurface->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &bltfx);
        g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Flip(0, DDFLIP_WAIT);

        if (g_hrLastResult != DD_OK)
        {
            goto releasePrimary;
        }
    }

    dwBlitTime = timeGetTime() - dwStart;

    if (dwBlitTime < dwFlipTime)
    {
        dwFlipTime = dwBlitTime;
    }

    g_ddrawDevice.pDDrawPrimary->Release();

    desc.dwFlags        = DDSD_CAPS;
    desc.ddsCaps.dwCaps = BLIT_PRIMARY_CAPS;

    g_hrLastResult = g_pDDrawDevice->CreateSurface(&desc, &g_ddrawDevice.pDDrawPrimary, 0);

    if (g_hrLastResult != DD_OK)
    {
        return 0;
    }

    desc.dwHeight       = g_dwDisplayHeight;
    desc.dwWidth        = g_dwDisplayWidth;
    desc.dwSize         = sizeof(desc);
    desc.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps = BLIT_BACK_CAPS;

    g_hrLastResult = g_pDDrawDevice->CreateSurface(&desc, &g_ddrawDevice.pBackSurface, 0);

    if (g_hrLastResult != DD_OK)
    {
        g_ddrawDevice.pDDrawPrimary->Release();

        return 0;
    }

    for (i = 0; i < g_nFlipChainCount; i++)
    {
        g_ddrawDevice.pBackSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
        memset(desc.lpSurface, 0, desc.lPitch * desc.dwHeight);
        g_ddrawDevice.pBackSurface->Unlock(0);

        g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Blt(&g_rcBltDest, g_ddrawDevice.pBackSurface, 0, DDBLT_WAIT, 0);

        if (g_hrLastResult != DD_OK)
        {
            goto releaseBack;
        }
    }

    dwStart = timeGetTime();

    for (i = 0; i < TIMED_FRAMES; i++)
    {
        g_ddrawDevice.pBackSurface->Lock(0, &desc, DDLOCK_WAIT, 0);
        Mem::ZeroBlockUnrolled128((char*)desc.lpSurface + i);
        g_ddrawDevice.pBackSurface->Unlock(0);

        g_hrLastResult = g_ddrawDevice.pBackSurface->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &bltfx);
        g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Blt(&g_rcBltDest, g_ddrawDevice.pBackSurface, 0, DDBLT_WAIT, 0);

        if (g_hrLastResult != DD_OK)
        {
            goto releaseBack;
        }
    }

    dwBlitTime = timeGetTime() - dwStart;

    g_ddrawDevice.pBackSurface->Release();
    g_ddrawDevice.pDDrawPrimary->Release();

    g_bHWFlipAvailable = dwBlitTime >= dwFlipTime;

    return g_bHWFlipAvailable;

releaseBack:
    g_ddrawDevice.pBackSurface->Release();

releasePrimary:
    g_ddrawDevice.pDDrawPrimary->Release();

    return 0;
}

// rcWindow.top gets a dead store from the client area, as in the original.
// FUNCTION: HYPERX 0x00439a90
unsigned int DDraw::BlitBackToWindow(HWND hWnd, int bMenu)
{
    RECT rcClient;
    RECT rcDest;
    RECT rcWindow;

    if (bMenu != 0 && g_ddrawDevice.pDDrawPrimary != 0 && g_ddrawDevice.pBackSurface != 0)
    {
        GetClientRect(hWnd, &rcClient);

        int nLeft      = rcClient.left;
        rcDest.left    = nLeft;
        const int nTop = rcClient.top;
        rcDest.top     = nTop;
        int nRight     = rcClient.right;
        rcDest.right   = nRight;
        int nBottom    = rcClient.bottom;
        rcDest.bottom  = nBottom;

        rcWindow.top = nTop;

        AdjustWindowRectEx(&rcClient,
                           GetWindowLongA(hWnd, GWL_STYLE),
                           bMenu,
                           GetWindowLongA(hWnd, GWL_EXSTYLE));

        if (g_gameApp.m_bIsActive != 0 && DDraw::CheckDisplayMode() != 0)
        {
            nBottom -= rcClient.bottom;
            nLeft -= rcClient.left;
            nRight -= rcClient.right;

            GetWindowRect(hWnd, &rcWindow);

            rcDest.left   = rcWindow.left - nLeft;
            rcDest.top    = rcWindow.top;
            rcDest.right  = rcWindow.right + nRight;
            rcDest.bottom = rcWindow.bottom + nBottom;

            g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Blt(&rcDest, g_ddrawDevice.pBackSurface, 0, DDBLT_WAIT, 0);

            if (g_hrLastResult == DDERR_SURFACELOST)
            {
                g_hrLastResult = DDraw::RestoreAllSurfaces();
            }
        }
    }

    return 0;
}

// The body never reads pDDraw, but its one caller pushes it, so the parameter stays declared.
// FUNCTION: HYPERX 0x00439bb0
int DDraw::GetCurrentDisplayMode(struct IDirectDraw*)
{
    DDSURFACEDESC ddsd;

    memset(&ddsd, 0, sizeof(ddsd));

    ddsd.dwSize = sizeof(ddsd);

    g_hrD3DResult = g_ddrawDevice.pDDrawObject->GetDisplayMode(&ddsd);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004be6c0
        g_gameApp.Error("Getting the current display mode failed.\n%s\n",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    g_ddrawDevice.nDisplayWidth  = ddsd.dwWidth;
    g_ddrawDevice.nDisplayHeight = ddsd.dwHeight;
    g_ddrawDevice.nDisplayBpp    = ddsd.ddpfPixelFormat.dwRGBBitCount;

    return 1;
}

// FUNCTION: HYPERX 0x00439c30
int __stdcall DDraw::CheckDisplayMode(void)
{
    DDSURFACEDESC desc;

    memset(&desc, 0, sizeof(desc));

    desc.dwSize = sizeof(desc);

    g_hrD3DResult = g_ddrawDevice.pDDrawObject->GetDisplayMode(&desc);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004be6c0
        g_gameApp.Error("Getting the current display mode failed.\n%s\n", DDraw::ErrorToString(g_hrD3DResult));
    }
    else if (g_ddrawDevice.nDisplayWidth == (int)desc.dwWidth && g_ddrawDevice.nDisplayHeight == (int)desc.dwHeight && g_ddrawDevice.nDisplayBpp == (int)desc.ddpfPixelFormat.dwRGBBitCount)
    {
        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00439cc0
int __cdecl DDraw::GetCaps(struct DDrawCaps* pDriver, struct DDrawCaps* pHel)
{
    memset(pDriver, 0, sizeof(int));
    pDriver->dwSize = sizeof(DDrawCaps);
    memset(pHel, 0, sizeof(int));
    pHel->dwSize = sizeof(DDrawCaps);

    g_hrD3DResult = g_ddrawDevice.pDDrawObject->GetCaps((LPDDCAPS)pDriver, (LPDDCAPS)pHel);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004be6f0
        g_gameApp.Error("GetCaps failed in while checking driver capabilities.\n%s\n",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    g_ddrawDevice.nDDrawDriverSurfaceCaps = pDriver->dwDX6SurfaceCaps;

    g_ddrawDevice.ddrawDriverCaps = *pDriver;
    g_ddrawDevice.ddrawHelCaps    = *pHel;

    return 1;
}

// The capability blocks are 380 bytes, the DirectX 6 DDCAPS; this tree's ddraw.h declares 364.

// FUNCTION: HYPERX 0x00439d50
BOOL CALLBACK DDraw::EnumDriversCallback(GUID* pGuid, char*, char*, void* pCtx)
{
    IDirectDraw* pDDraw;
    DDrawCaps    driverCaps;
    DDrawCaps    helCaps;

    if (pGuid == 0)
    {
        return DDENUMRET_OK;
    }

    g_hrD3DResult = DirectDrawCreate(pGuid, &pDDraw, 0);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004be77c
        g_gameApp.Error("Failing while creating a DD device for testing.  Continuing with execution.\n%s\n",
                        DDraw::ErrorToString(g_hrD3DResult));

        return DDENUMRET_OK;
    }

    memset(&driverCaps, 0, sizeof(driverCaps));
    driverCaps.dwSize = sizeof(driverCaps);

    memset(&helCaps, 0, sizeof(helCaps));
    helCaps.dwSize = sizeof(helCaps);

    g_hrD3DResult = pDDraw->GetCaps((LPDDCAPS)&driverCaps, (LPDDCAPS)&helCaps);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004be72c
        g_gameApp.Error("GetCaps failed in while testing a DD device.  Continuing with execution.\n%s\n",
                        DDraw::ErrorToString(g_hrD3DResult));
    }
    else if (g_bForceSoftwareRender == 0 && (driverCaps.dwCaps & DDCAPS_3D) != 0)
    {
        *(IDirectDraw**)pCtx = pDDraw;

        g_ddrawDevice.ddrawDriverGuid = *pGuid;

        g_ddrawDevice.ddrawDriverCaps = driverCaps;

        g_ddrawDevice.bIsPrimary = 0;
        g_d3dState.bIsPrimary    = 0;

        return DDENUMRET_CANCEL;
    }
    else
    {
        g_ddrawDevice.bIsPrimary = 1;
        g_d3dState.bIsPrimary    = 1;

        *(IDirectDraw**)pCtx = 0;
    }

    pDDraw->Release();

    return DDENUMRET_OK;
}

// FUNCTION: HYPERX 0x00439ec0
char* DDraw::ErrorToString(int hr)
{
    switch (hr)
    {
        case DDERR_OUTOFMEMORY:
            // STRING: HYPERX 0x004c0510
            return "DirectDraw does not have enough memory to perform the operation.";

        case DDERR_GENERIC:
            // STRING: HYPERX 0x004c04fc
            return "Generic failure.";

        case DDERR_UNSUPPORTED:
            // STRING: HYPERX 0x004c04e4
            return "Action not supported.";

        case DDERR_INVALIDPARAMS:
            // STRING: HYPERX 0x004c049c
            return "One or more of the parameters passed to the function are incorrect.";

        case DDERR_CANNOTDETACHSURFACE:
            // STRING: HYPERX 0x004c045c
            return "This surface can not be detached from the requested surface.";

        case DDERR_CANNOTATTACHSURFACE:
            // STRING: HYPERX 0x004c0420
            return "This surface can not be attached to the requested surface.";

        case DDERR_ALREADYINITIALIZED:
            // STRING: HYPERX 0x004c03f8
            return "This object is already initialized.";

        case DDERR_CURRENTLYNOTAVAIL:
            // STRING: HYPERX 0x004c03d0
            return "Support is currently not available.";

        case DDERR_EXCEPTION:
            // STRING: HYPERX 0x004c0388
            return "An exception was encountered while performing the requested operation.";

        case DDERR_HEIGHTALIGN:
            // STRING: HYPERX 0x004c0344
            return "Height of rectangle provided is not a multiple of reqd alignment.";

        case DDERR_INCOMPATIBLEPRIMARY:
            // STRING: HYPERX 0x004c02f0
            return "Unable to match primary surface creation request with existing primary surface.";

        case DDERR_INVALIDCAPS:
            // STRING: HYPERX 0x004c02ac
            return "One or more of the caps bits passed to the callback are incorrect.";

        case DDERR_INVALIDCLIPLIST:
            // STRING: HYPERX 0x004c0278
            return "DirectDraw does not support the provided cliplist.";

        case DDERR_INVALIDMODE:
            // STRING: HYPERX 0x004c0244
            return "DirectDraw does not support the requested mode.";

        case DDERR_INVALIDOBJECT:
            // STRING: HYPERX 0x004c01fc
            return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.";

        case DDERR_INVALIDPIXELFORMAT:
            // STRING: HYPERX 0x004c01d0
            return "The pixel format was invalid as specified.";

        case DDERR_INVALIDRECT:
            // STRING: HYPERX 0x004c01ac
            return "Rectangle provided was invalid.";

        case DDERR_LOCKEDSURFACES:
            // STRING: HYPERX 0x004c015c
            return "Operation could not be carried out because one or more surfaces are locked.";

        case DDERR_NO3D:
            // STRING: HYPERX 0x004c0140
            return "There is no 3D present.";

        case DDERR_NOALPHAHW:
            // STRING: HYPERX 0x004c00d4
            return "Operation could not be carried out because there is no alpha accleration hardware present or available.";

        case DDERR_NOCLIPLIST:
            // STRING: HYPERX 0x004c00bc
            return "No cliplist available.";

        case DDERR_NOCOLORCONVHW:
            // STRING: HYPERX 0x004c0054
            return "Operation could not be carried out because there is no color conversion hardware present or available.";

        case DDERR_NOCOLORKEY:
            // STRING: HYPERX 0x004c0028
            return "Surface doesn't currently have a color key";

        case DDERR_NOCOLORKEYHW:
            // STRING: HYPERX 0x004bffc0
            return "Operation could not be carried out because there is no hardware support of the destination color key.";

        case DDERR_NOCOOPERATIVELEVELSET:
            // STRING: HYPERX 0x004bff64
            return "Create function called without DirectDraw object method SetCooperativeLevel being called.";

        case DDERR_NOEXCLUSIVEMODE:
            // STRING: HYPERX 0x004bfef4
            return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.";

        case DDERR_NOFLIPHW:
            // STRING: HYPERX 0x004bfec4
            return "Flipping visible surfaces is not supported.";

        case DDERR_NOGDI:
            // STRING: HYPERX 0x004bfea8
            return "There is no GDI present.";

        case DDERR_NOMIRRORHW:
            // STRING: HYPERX 0x004bfe50
            return "Operation could not be carried out because there is no hardware present or available.";

        case DDERR_NOTFOUND:
            // STRING: HYPERX 0x004bfe30
            return "Requested item was not found.";

        case DDERR_NOOVERLAYHW:
            // STRING: HYPERX 0x004bfdd0
            return "Operation could not be carried out because there is no overlay hardware present or available.";

        case DDERR_NORASTEROPHW:
            // STRING: HYPERX 0x004bfd60
            return "Operation could not be carried out because there is no appropriate raster op hardware present or available.";

        case DDERR_NOROTATIONHW:
            // STRING: HYPERX 0x004bfd00
            return "Operation could not be carried out because there is no rotation hardware present or available.";

        case DDERR_NOSTRETCHHW:
            // STRING: HYPERX 0x004bfca4
            return "Operation could not be carried out because there is no hardware support for stretching.";

        case DDERR_NOT4BITCOLOR:
            // STRING: HYPERX 0x004bfc38
            return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.";

        case DDERR_NOT4BITCOLORINDEX:
            // STRING: HYPERX 0x004bfbc0
            return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.";

        case DDERR_NOT8BITCOLOR:
            // STRING: HYPERX 0x004bfb60
            return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.";

        case DDERR_NOTEXTUREHW:
            // STRING: HYPERX 0x004bfaf8
            return "Operation could not be carried out because there is no texture mapping hardware present or available.";

        case DDERR_NOVSYNCHW:
            // STRING: HYPERX 0x004bfa80
            return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.";

        case DDERR_NOZBUFFERHW:
            // STRING: HYPERX 0x004bfa20
            return "Operation could not be carried out because there is no hardware support for zbuffer blitting.";

        case DDERR_NOZOVERLAYHW:
            // STRING: HYPERX 0x004bf9a0
            return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.";

        case DDERR_OUTOFCAPS:
            // STRING: HYPERX 0x004bf950
            return "The hardware needed for the requested operation has already been allocated.";

        case DDERR_OUTOFVIDEOMEMORY:
            // STRING: HYPERX 0x004c0510
            return "DirectDraw does not have enough memory to perform the operation.";

        case DDERR_OVERLAYCANTCLIP:
            // STRING: HYPERX 0x004bf91c
            return "The hardware does not support clipped overlays.";

        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
            // STRING: HYPERX 0x004bf8dc
            return "Can only have ony color key active at one time for overlays.";

        case DDERR_PALETTEBUSY:
            // STRING: HYPERX 0x004bf878
            return "Access to this palette is being refused because the palette is already locked by another thread.";

        case DDERR_COLORKEYNOTSET:
            // STRING: HYPERX 0x004bf848
            return "No src color key specified for this operation.";

        case DDERR_SURFACEALREADYATTACHED:
            // STRING: HYPERX 0x004bf7fc
            return "This surface is already attached to the surface it is being attached to.";

        case DDERR_SURFACEALREADYDEPENDENT:
            // STRING: HYPERX 0x004bf7a4
            return "This surface is already a dependency of the surface it is being made a dependency of.";

        case DDERR_SURFACEBUSY:
            // STRING: HYPERX 0x004bf740
            return "Access to this surface is being refused because the surface is already locked by another thread.";

        case DDERR_SURFACEISOBSCURED:
            // STRING: HYPERX 0x004bf704
            return "Access to surface refused because the surface is obscured.";

        case DDERR_SURFACELOST:
            // STRING: HYPERX 0x004bf65c
            return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.";

        case DDERR_SURFACENOTATTACHED:
            // STRING: HYPERX 0x004bf634
            return "The requested surface is not attached.";

        case DDERR_TOOBIGHEIGHT:
            // STRING: HYPERX 0x004bf604
            return "Height requested by DirectDraw is too large.";

        case DDERR_TOOBIGSIZE:
            // STRING: HYPERX 0x004bf5ac
            return "Size requested by DirectDraw is too large, but the individual height and width are OK.";

        case DDERR_TOOBIGWIDTH:
            // STRING: HYPERX 0x004bf57c
            return "Width requested by DirectDraw is too large.";

        case DDERR_UNSUPPORTEDFORMAT:
            // STRING: HYPERX 0x004bf544
            return "FOURCC format requested is unsupported by DirectDraw.";

        case DDERR_UNSUPPORTEDMASK:
            // STRING: HYPERX 0x004bf4fc
            return "Bitmask in the pixel format requested is unsupported by DirectDraw.";

        case DDERR_VERTICALBLANKINPROGRESS:
            // STRING: HYPERX 0x004bf4dc
            return "Vertical blank is in progress.";

        case DDERR_WASSTILLDRAWING:
            // STRING: HYPERX 0x004bf468
            return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.";

        case DDERR_XALIGN:
            // STRING: HYPERX 0x004bf420
            return "Rectangle provided was not horizontally aligned on required boundary.";

        case DDERR_INVALIDDIRECTDRAWGUID:
            // STRING: HYPERX 0x004bf3cc
            return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.";

        case DDERR_BLTFASTCANTCLIP:
            // STRING: HYPERX 0x004bf370
            return "Return if a clipper object is attached to the source surface passed into a BltFast call.";

        case DDERR_CANTCREATEDC:
            // STRING: HYPERX 0x004bf348
            return "Windows can not create any more DCs.";

        case DDERR_CANTDUPLICATE:
            // STRING: HYPERX 0x004bf2f4
            return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.";

        case DDERR_CLIPPERISUSINGHWND:
            // STRING: HYPERX 0x004bf294
            return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.";

        case DDERR_DIRECTDRAWALREADYCREATED:
            // STRING: HYPERX 0x004bf238
            return "A DirectDraw object representing this driver has already been created for this process.";

        case DDERR_EXCLUSIVEMODEALREADYSET:
            // STRING: HYPERX 0x004bf1e0
            return "An attempt was made to set the cooperative level when it was already set to exclusive.";

        case DDERR_HWNDALREADYSET:
            // STRING: HYPERX 0x004bf164
            return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.";

        case DDERR_HWNDSUBCLASSED:
            // STRING: HYPERX 0x004bf0f4
            return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.";

        case DDERR_IMPLICITLYCREATED:
            // STRING: HYPERX 0x004bf0a4
            return "This surface can not be restored because it is an implicitly created surface.";

        case DDERR_INVALIDPOSITION:
            // STRING: HYPERX 0x004bf03c
            return "Returned when the position of the overlay on the destination is no longer legal for that destination.";

        case DDERR_NOBLTHW:
            // STRING: HYPERX 0x004bf01c
            return "No blitter hardware present.";

        case DDERR_NOCLIPPERATTACHED:
            // STRING: HYPERX 0x004befec
            return "No clipper object attached to surface object.";

        case DDERR_NODC:
            // STRING: HYPERX 0x004befc0
            return "No DC was ever created for this surface.";

        case DDERR_NODDROPSHW:
            // STRING: HYPERX 0x004befa0
            return "No DirectDraw ROP hardware.";

        case DDERR_NODIRECTDRAWHW:
            // STRING: HYPERX 0x004bef38
            return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.";

        case DDERR_NOEMULATION:
            // STRING: HYPERX 0x004bef14
            return "Software emulation not available.";

        case DDERR_NOHWND:
            // STRING: HYPERX 0x004beeac
            return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.";

        case DDERR_NOOVERLAYDEST:
            // STRING: HYPERX 0x004bee28
            return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.";

        case DDERR_NOPALETTEATTACHED:
            // STRING: HYPERX 0x004bedf8
            return "No palette object attached to this surface.";

        case DDERR_NOPALETTEHW:
            // STRING: HYPERX 0x004bedc4
            return "No hardware support for 16 or 256 color palettes.";

        case DDERR_NOTAOVERLAYSURFACE:
            // STRING: HYPERX 0x004bed7c
            return "Returned when an overlay member is called for a non-overlay surface.";

        case DDERR_NOTFLIPPABLE:
            // STRING: HYPERX 0x004bed38
            return "An attempt has been made to flip a surface that is not flippable.";

        case DDERR_NOTLOCKED:
            // STRING: HYPERX 0x004becb8
            return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.";

        case DDERR_NOTPALETTIZED:
            // STRING: HYPERX 0x004bec80
            return "The surface being used is not a palette-based surface.";

        case DDERR_OVERLAYNOTVISIBLE:
            // STRING: HYPERX 0x004bec3c
            return "Returned when GetOverlayPosition is called on a hidden overlay.";

        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            // STRING: HYPERX 0x004bec04
            return "This process already has created a primary surface.";

        case DDERR_REGIONTOOSMALL:
            // STRING: HYPERX 0x004bebcc
            return "Region passed to Clipper::GetClipList is too small.";

        case DDERR_WRONGMODE:
            // STRING: HYPERX 0x004beb7c
            return "This surface can not be restored because it was created in a different mode.";

        case D3DERR_BADMAJORVERSION:
            // STRING: HYPERX 0x004beb64
            return "D3DERR_BADMAJORVERSION";

        case D3DERR_BADMINORVERSION:
            // STRING: HYPERX 0x004beb4c
            return "D3DERR_BADMINORVERSION";

        case D3DERR_EXECUTE_LOCKED:
            // STRING: HYPERX 0x004beb34
            return "D3DERR_EXECUTE_LOCKED";

        case D3DERR_EXECUTE_NOT_LOCKED:
            // STRING: HYPERX 0x004beb18
            return "D3DERR_EXECUTE_NOT_LOCKED";

        case D3DERR_EXECUTE_CREATE_FAILED:
            // STRING: HYPERX 0x004beaf8
            return "D3DERR_EXECUTE_CREATE_FAILED";

        case D3DERR_EXECUTE_DESTROY_FAILED:
            // STRING: HYPERX 0x004bead8
            return "D3DERR_EXECUTE_DESTROY_FAILED";

        case D3DERR_EXECUTE_LOCK_FAILED:
            // STRING: HYPERX 0x004beabc
            return "D3DERR_EXECUTE_LOCK_FAILED";

        case D3DERR_EXECUTE_UNLOCK_FAILED:
            // STRING: HYPERX 0x004bea9c
            return "D3DERR_EXECUTE_UNLOCK_FAILED";

        case D3DERR_EXECUTE_FAILED:
            // STRING: HYPERX 0x004bea84
            return "D3DERR_EXECUTE_FAILED";

        case D3DERR_EXECUTE_CLIPPED_FAILED:
            // STRING: HYPERX 0x004bea64
            return "D3DERR_EXECUTE_CLIPPED_FAILED";

        case D3DERR_TEXTURE_NO_SUPPORT:
            // STRING: HYPERX 0x004bea48
            return "D3DERR_TEXTURE_NO_SUPPORT";

        case D3DERR_TEXTURE_CREATE_FAILED:
            // STRING: HYPERX 0x004bea28
            return "D3DERR_TEXTURE_CREATE_FAILED";

        case D3DERR_TEXTURE_DESTROY_FAILED:
            // STRING: HYPERX 0x004bea08
            return "D3DERR_TEXTURE_DESTROY_FAILED";

        case D3DERR_TEXTURE_LOCK_FAILED:
            // STRING: HYPERX 0x004be9ec
            return "D3DERR_TEXTURE_LOCK_FAILED";

        case D3DERR_TEXTURE_UNLOCK_FAILED:
            // STRING: HYPERX 0x004be9cc
            return "D3DERR_TEXTURE_UNLOCK_FAILED";

        case D3DERR_TEXTURE_LOAD_FAILED:
            // STRING: HYPERX 0x004be9b0
            return "D3DERR_TEXTURE_LOAD_FAILED";

        case D3DERR_TEXTURE_LOCKED:
            // STRING: HYPERX 0x004be998
            return "D3DERR_TEXTURELOCKED";

        case D3DERR_TEXTURE_NOT_LOCKED:
            // STRING: HYPERX 0x004be97c
            return "D3DERR_TEXTURE_NOT_LOCKED";

        case D3DERR_MATRIX_CREATE_FAILED:
            // STRING: HYPERX 0x004be95c
            return "D3DERR_MATRIX_CREATE_FAILED";

        case D3DERR_MATRIX_DESTROY_FAILED:
            // STRING: HYPERX 0x004be93c
            return "D3DERR_MATRIX_DESTROY_FAILED";

        case D3DERR_MATRIX_SETDATA_FAILED:
            // STRING: HYPERX 0x004be91c
            return "D3DERR_MATRIX_SETDATA_FAILED";

        case D3DERR_SETVIEWPORTDATA_FAILED:
            // STRING: HYPERX 0x004be8fc
            return "D3DERR_SETVIEWPORTDATA_FAILED";

        case D3DERR_MATERIAL_CREATE_FAILED:
            // STRING: HYPERX 0x004be8dc
            return "D3DERR_MATERIAL_CREATE_FAILED";

        case D3DERR_MATERIAL_DESTROY_FAILED:
            // STRING: HYPERX 0x004be8bc
            return "D3DERR_MATERIAL_DESTROY_FAILED";

        case D3DERR_MATERIAL_SETDATA_FAILED:
            // STRING: HYPERX 0x004be89c
            return "D3DERR_MATERIAL_SETDATA_FAILED";

        case D3DERR_MATERIAL_GETDATA_FAILED:
            // STRING: HYPERX 0x004be87c
            return "D3DERR_MATERIAL_GETDATA_FAILED";

        case D3DERR_LIGHT_SET_FAILED:
            // STRING: HYPERX 0x004be860
            return "D3DERR_LIGHT_SET_FAILED";

        case D3DERR_SCENE_NOT_IN_SCENE:
            // STRING: HYPERX 0x004be844
            return "D3DERR_SCENE_NOT_IN_SCENE";

        case D3DERR_SCENE_IN_SCENE:
            // STRING: HYPERX 0x004be82c
            return "D3DERR_SCENE_IN_SCENE";

        case D3DERR_SCENE_BEGIN_FAILED:
            // STRING: HYPERX 0x004be810
            return "D3DERR_SCENE_BEGIN_FAILED";

        case 0x00000000:
            // STRING: HYPERX 0x004be7e8
            return "No error.";

        case D3DERR_SCENE_END_FAILED:
            // STRING: HYPERX 0x004be7cc
            return "D3DERR_SCENE_END_FAILED";
    }

    // STRING: HYPERX 0x004be7f4
    return "Unrecognized error value.";
}

// FUNCTION: HYPERX 0x0043a6c0
unsigned int DDraw::InitDeviceAndSurfaces(HPALETTE hPalette)
{
    DDrawCaps    capsDriver;
    DDrawCaps    capsHel;
    PALETTEENTRY aEntries[256];

    DDraw::ClearDeviceState();

    g_hrD3DResult = DirectDrawEnumerateA(DDraw::EnumDriversCallback, &g_ddrawDevice.pDDrawObject);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004c073c
        g_gameApp.Error("DirectDrawEnumerate failed.\n%s\n",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    if (g_ddrawDevice.pDDrawObject == 0)
    {
        g_hrLastResult = DirectDrawCreate(0, &g_ddrawDevice.pDDrawObject, 0);

        if (g_hrLastResult != 0)
        {
            return DDraw::FailInitialization(g_hrLastResult & DDRAW_ERROR_CODE_MASK);
        }
    }
    else
    {
        g_hrLastResult = 0;
    }

    if (g_pDDrawDevice == 0)
    {
        if (g_bFullScreen != 0)
        {
            g_hrLastResult =
                g_ddrawDevice.pDDrawObject->SetCooperativeLevel(g_hWndGame, DDSCL_GAME);
        }
        else
        {
            g_hrLastResult =
                g_ddrawDevice.pDDrawObject->SetCooperativeLevel(g_hWndGame, DDSCL_NORMAL);
        }

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c071c
            g_gameApp.Error("SetCooperativeLevel failed!\n%s\n",
                            DDraw::ErrorToString(g_hrLastResult));

            return DDraw::FailInitialization(g_hrLastResult & DDRAW_ERROR_CODE_MASK);
        }
    }

    DDraw::GetCurrentDisplayMode(g_ddrawDevice.pDDrawObject);

    if (DDraw::GetCaps(&capsDriver, &capsHel) == 0)
    {
        return 0;
    }

    if ((capsDriver.dwCaps & DDCAPS_NOHARDWARE) != 0)
    {
        g_bForceSoftwareRender = 1;
        g_bEmulationOnly       = 0;
    }
    else
    {
        if (g_bEmulationOnly != 0)
        {
            capsDriver.dwCaps = capsHel.dwCaps;
        }

        if (g_bForceSoftwareRender == 0 && ((capsDriver.dwCaps & DDCAPS_3D) != 0 || g_bEmulationOnly != 0))
        {
            int nDriver;
            int nMode;

            D3D::GetCurrentDisplayMode(g_ddrawDevice.pDDrawObject);
            DDraw::EnumDisplayModes(g_ddrawDevice.pDDrawObject);
            D3D::Initialize(g_ddrawDevice.pDDrawObject);
            D3D::EnumDrivers();

            if (D3D::SelectDisplayMode(&nDriver, &nMode) == 0)
            {
                return 0;
            }

            g_bUseOffscreenSurface = 0;
            g_displayMode.nWidth   = g_displayState.aDisplayModes[nMode].nWidth;
            g_displayMode.nHeight  = g_displayState.aDisplayModes[nMode].nHeight;
            g_dwDisplayBpp         = g_displayState.aDisplayModes[nMode].nBpp;

#ifdef HYPERX_BRINGUP
            // The enumeration overwrites g_dwDisplayWidth on this path, so HB_RES is applied again.
            BringUp::OverrideDisplayMode(&g_displayMode.nWidth, &g_displayMode.nHeight);
#endif
        }
    }

    if (g_bFullScreen != 0)
    {
        if (g_ddrawDevice.bIsPrimary == 0)
        {
            g_pSavedDesktopMode = new SavedDisplayMetrics;

            g_pSavedDesktopMode->nBpp    = ::GetDeviceCaps(g_gameApp.GetScreenDC(), BITSPIXEL);
            g_pSavedDesktopMode->nWidth  = ::GetDeviceCaps(g_gameApp.GetScreenDC(), HORZRES);
            g_pSavedDesktopMode->nHeight = ::GetDeviceCaps(g_gameApp.GetScreenDC(), VERTRES);

            DDraw::ChangeDisplayMode(0);
        }

        g_hrLastResult = g_ddrawDevice.pDDrawObject
                             ->SetDisplayMode(g_displayMode.nWidth, g_displayMode.nHeight, g_dwDisplayBpp);

        if (g_hrLastResult != 0)
        {
            if (g_displayMode.nWidth != 640 || g_displayMode.nHeight != 480)
            {
                // STRING: HYPERX 0x004c06fc
                g_gameApp.Error("Can't set mode trying 640x480!\n");

                g_displayMode.nWidth  = 640;
                g_displayMode.nHeight = 480;

                g_hrLastResult = g_ddrawDevice.pDDrawObject
                                     ->SetDisplayMode(g_displayMode.nWidth, g_displayMode.nHeight, g_dwDisplayBpp);
            }

            if (g_hrLastResult != 0)
            {
                if (g_dwDisplayBpp != 8)
                {
                    // STRING: HYPERX 0x004c06d8
                    g_gameApp.Error("Can't set mode trying 640x480x8!\n");

                    g_dwDisplayBpp = 8;

                    g_hrLastResult = g_ddrawDevice.pDDrawObject
                                         ->SetDisplayMode(g_displayMode.nWidth,
                                                          g_displayMode.nHeight,
                                                          g_dwDisplayBpp);
                }

                if (g_hrLastResult != 0)
                {
                    // STRING: HYPERX 0x004c06c4
                    g_gameApp.Error("SetMode failed!\n%s\n",
                                    DDraw::ErrorToString(g_hrLastResult));

                    return DDraw::FailInitialization(g_hrLastResult & DDRAW_ERROR_CODE_MASK);
                }
            }
        }
    }
    else
    {
        RECT rc;
        RECT rcWork;

        if (g_ddrawDevice.bIsPrimary == 0)
        {
            // STRING: HYPERX 0x004c0660
            g_gameApp.Fatal("This graphics accelerator card does not support drawing in a window!\nChange to fullscreen mode.\n");
        }

        if ((g_bEmulationOnly == 0 && (capsDriver.dwCaps & DDCAPS_3D) == 0) || g_bForceSoftwareRender != 0)
        {
            g_hrLastResult = DDraw::SetFullscreenMode(g_hWndGame, 8);
        }
        else
        {
            g_hrLastResult = DDraw::SetFullscreenMode(g_hWndGame, 16);
        }

        if (g_hrLastResult == 0)
        {
            if (DDraw::GetCaps(&capsDriver, &capsHel) == 0)
            {
                return 0;
            }

            if (g_bEmulationOnly != 0)
            {
                capsDriver.dwCaps = capsHel.dwCaps;
            }
        }

        if (g_bDoubleResMode != 0)
        {
            SetRect(&rc, 0, 0, g_displayMode.nWidth * 2, g_displayMode.nHeight * 2);
        }
        else
        {
            SetRect(&rc, 0, 0, g_displayMode.nWidth, g_displayMode.nHeight);
        }

        AdjustWindowRectEx(&rc, GetWindowLongA(g_hWndGame, GWL_STYLE), GetMenu(g_hWndGame) != 0, GetWindowLongA(g_hWndGame, GWL_EXSTYLE));

        SetWindowPos(g_hWndGame, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(g_hWndGame, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        SystemParametersInfoA(SPI_GETWORKAREA, 0, &rcWork, 0);
        GetWindowRect(g_hWndGame, &rc);

        if (rc.left < rcWork.left)
        {
            rc.left = rcWork.left;
        }

        if (rc.top < rcWork.top)
        {
            rc.top = rcWork.top;
        }

        SetWindowPos(g_hWndGame, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    g_bBltColourFill = (capsDriver.dwCaps & DDCAPS_BLTCOLORFILL) != 0;

    if ((capsDriver.dwCaps & DDCAPS_BANKSWITCHED) != 0 && (g_bForceSoftwareRender != 0 || (capsDriver.dwCaps & DDCAPS_3D) == 0))
    {
        g_bUseOffscreenSurface = 1;
    }

    g_nVidMemTotal = capsDriver.dwVidMemTotal;

    if (g_nFlipChainCount == 0)
    {
        unsigned int nNeeded;

        nNeeded = (g_dwDisplayBpp >> 3) * g_displayMode.nHeight * g_displayMode.nWidth * 2;

#ifdef HYPERX_BRINGUP
        // Modern DirectDraw reports no video memory, so the back buffer goes to system memory.
        if (capsDriver.dwVidMemTotal == 0)
        {
            g_bUseOffscreenSurface = 1;
        }
        else
#endif
            if (capsDriver.dwVidMemTotal <= nNeeded && g_bFullScreen == 0)
        {
            if (capsDriver.dwVidMemTotal > nNeeded / 2)
            {
                if ((capsDriver.dwCaps & DDCAPS_CANBLTSYSMEM) != 0)
                {
                    // STRING: HYPERX 0x004c05d4
                    g_gameApp.Error("Insufficient free video memory for double buffered graphics.\nPlacing back buffer in system memory.\n\n%0.2f KB Available.\n%0.2f KB Required.\n", capsDriver.dwVidMemTotal / 1024.0f, nNeeded / 1024.0f);

                    g_bUseOffscreenSurface = 1;
                }
                else
                {
                    // STRING: HYPERX 0x004c0554
                    g_gameApp.Fatal("Insufficient free video memory for graphics buffers.\nTry reducing desktop resolution.\n\n%0.2f KB Available.\n%0.2f KB Required.\n", capsDriver.dwVidMemTotal / 1024.0f, nNeeded / 1024.0f);
                }
            }
            else
            {
                // STRING: HYPERX 0x004c0554
                g_gameApp.Fatal("Insufficient free video memory for graphics buffers.\nTry reducing desktop resolution.\n\n%0.2f KB Available.\n%0.2f KB Required.\n", capsDriver.dwVidMemTotal / 1024.0f, nNeeded / 1024.0f);
            }
        }

        if (capsDriver.dwVidMemTotal > ((g_dwDisplayBpp >> 3) * 1048576) && g_displayMode.nWidth <= 640)
        {
            if (g_bTripleBufferEnabled != 0)
            {
                g_nFlipChainCount = 3;
            }
            else
            {
                g_nFlipChainCount = 2;
            }
        }
        else
        {
            g_nFlipChainCount = 2;
        }
    }

    if ((capsDriver.dwCaps & DDCAPS_COLORKEY) != 0 && (capsDriver.dwCKeyCaps & DDCKEYCAPS_DESTBLT) != 0 && (capsDriver.dwCKeyCaps & DDCKEYCAPS_SRCBLT) == 0 && capsDriver.dwVidMemTotal >= ((g_dwDisplayBpp >> 3) * 2097152))
    {
        g_bBackBufferColourKey = 1;
    }

    g_pDDrawDevice = g_ddrawDevice.pDDrawObject;

    g_nPrevWidth            = g_dwDisplayWidth;
    g_nBackBufferWidth      = g_dwDisplayWidth;
    g_nPrevHeight           = g_dwDisplayHeight;
    g_nBackBufferHeight     = g_dwDisplayHeight;
    g_nBackBufferPixelCount = (g_dwDisplayBpp >> 3) * g_dwDisplayHeight * g_dwDisplayWidth;
    g_bBottomUpDIB          = 0;

    if (g_ddrawDevice.bD3DActive != 0)
    {
        GetPaletteEntries(hPalette, 0, 256, aEntries);

        g_hrLastResult = g_pDDrawDevice->CreatePalette(
            DDPCAPS_8BIT, aEntries, &g_pDevicePalette, 0);
    }

    return 1;
}

// The 2904 byte device block, the 868 byte display state and the 448 byte HUD surfaces.
// FUNCTION: HYPERX 0x0043ade0
void DDraw::ClearDeviceState(void)
{
    memset(&g_ddrawDevice, 0, sizeof(g_ddrawDevice));
    memset(&g_displayState, 0, sizeof(g_displayState));
    memset(&g_hudSurfaces, 0, sizeof(g_hudSurfaces));

    g_ddrawDevice.bIsPrimary = 1;

    D3DRend::ResetState();
}

// GLOBAL: HYPERX 0x004cee68
extern int g_bDoubleResMode;

// FUNCTION: HYPERX 0x0043ae20
int __cdecl DDraw::SetFullscreenMode(HWND hWnd, int nBpp)
{
    const HDC hdc = GetDC(0);

    const int nBits = ::GetDeviceCaps(hdc, PLANES) * ::GetDeviceCaps(hdc, BITSPIXEL);

    ReleaseDC(0, hdc);

#ifdef HYPERX_BRINGUP
    // Reached on depth alone, and modern Windows offers no 8-bit mode; E_FAIL is already handled.
    return E_FAIL;
#endif

    if (nBits != nBpp)
    {
        g_hrLastResult = g_ddrawDevice.pDDrawObject->SetCooperativeLevel(hWnd, DDRAW_EXCLUSIVE_FULLSCREEN);

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c071c
            g_gameApp.Error("SetCooperativeLevel failed!\n%s\n",
                            DDraw::ErrorToString(g_hrLastResult));

            return g_hrLastResult;
        }

        g_hrLastResult = g_ddrawDevice.pDDrawObject->SetDisplayMode(
            g_ddrawDevice.nDisplayWidth, g_ddrawDevice.nDisplayHeight, nBpp);

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c06c4
            g_gameApp.Fatal("SetMode failed!\n%s\n", DDraw::ErrorToString(g_hrLastResult));

            return DDraw::FailInitialization(g_hrLastResult & DDRAW_ERROR_CODE_MASK);
        }

        g_hrLastResult = g_ddrawDevice.pDDrawObject->SetCooperativeLevel(g_hWndGame, DDSCL_NORMAL);

        if (g_hrLastResult == 0)
        {
            g_dwDisplayBpp = nBpp;

            return g_hrLastResult;
        }

        g_gameApp.Error("SetCooperativeLevel failed!\n%s\n",
                        DDraw::ErrorToString(g_hrLastResult));

        return g_hrLastResult;
    }

    return E_FAIL;
}

// Nothing returns from the fatal box, so the zero below is unreachable.
// FUNCTION: HYPERX 0x0043af20
int __cdecl DDraw::FailInitialization(int nError)
{
    DDraw::DestroyAllSurfaces();

    // STRING: HYPERX 0x004c075c
    g_gameApp.Fatal("DirectDraw initialization failed err=%d!", nError);

    return 0;
}

// Eight bits per pixel has no masks, so the key is 0xff.
// FUNCTION: HYPERX 0x0043af40
unsigned int __stdcall DDraw::CreatePrimaryAndBackBuffer(void)
{
    DDrawCaps     caps;
    DDSURFACEDESC desc;
    DDPIXELFORMAT format;
    HRESULT       hr;

    caps.dwSize = sizeof(caps);

    if (g_pDDrawDevice->GetCaps((LPDDCAPS)&caps, 0) != DD_OK)
    {
        return 0;
    }

    if (g_bUseOffscreenSurface == 0)
    {
        DDraw::DetectFlipCapability();
    }
    else
    {
        g_bHWFlipAvailable = 1;
    }

    if (g_bUseOffscreenSurface != 0 && g_nFlipChainCount > 2)
    {
        g_nFlipChainCount--;
    }

    memset(&desc, 0, sizeof(desc));

    desc.dwSize = sizeof(desc);

    if (g_bFullScreen != 0)
    {
        if (g_bForceSoftwareRender != 0 && g_bHWFlipAvailable == 0)
        {
            desc.dwFlags        = DDSD_CAPS;
            desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;
        }
        else
        {
            desc.dwFlags           = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
            desc.dwBackBufferCount = g_nFlipChainCount - 1;
            desc.ddsCaps.dwCaps =
                DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        }

        hr = g_pDDrawDevice->CreateSurface(&desc, &g_ddrawDevice.pDDrawPrimary, 0);

        if (hr != DD_OK)
        {
            // STRING: HYPERX 0x004c0788
            g_gameApp.Error("CreateSurface FAILED!\n%s\n", DDraw::ErrorToString(hr));

            return DDraw::FailInitialization(hr & DDRAW_ERROR_CODE_MASK);
        }

        g_nSurfaceMemoryUsed = g_nSurfaceMemoryUsed + (g_dwDisplayBpp >> 3) * g_dwDisplayWidth * g_dwDisplayHeight * g_nFlipChainCount;

        if (g_bForceSoftwareRender != 0 && g_bHWFlipAvailable == 0)
        {
            desc.dwHeight       = g_dwDisplayHeight;
            desc.dwSize         = sizeof(desc);
            desc.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
            desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
            desc.dwWidth        = g_dwDisplayWidth;

            hr = g_pDDrawDevice->CreateSurface(&desc, &g_ddrawDevice.pBackSurface, 0);

            if (hr != DD_OK)
            {
                // STRING: HYPERX 0x004c07c4
                g_gameApp.Error("CreateSurface on back buffer FAILED!\n%s\n",
                                DDraw::ErrorToString(hr));

                return DDraw::FailInitialization(hr & DDRAW_ERROR_CODE_MASK);
            }
        }
        else
        {
            DDSCAPS scaps;

            scaps.dwCaps = DDSCAPS_BACKBUFFER;

            hr = g_ddrawDevice.pDDrawPrimary
                     ->GetAttachedSurface(&scaps, &g_ddrawDevice.pBackSurface);

            if (hr != DD_OK)
            {
                // STRING: HYPERX 0x004c07a4
                g_gameApp.Error("GetAttachedSurface failed!\n%s\n",
                                DDraw::ErrorToString(hr));

                return DDraw::FailInitialization(hr & DDRAW_ERROR_CODE_MASK);
            }
        }

        if (g_bDoubleResMode != 0 && (caps.dwCaps & DDCAPS_BANKSWITCHED) != 0)
        {
            g_ddrawDevice.pZoomSurface = DDraw::CreateOffscreenSurface(g_dwDisplayWidth, g_dwDisplayHeight, 1, 0);
        }
    }
    else
    {
        if (g_ddrawDevice.bIsPrimary == 0)
        {
            // STRING: HYPERX 0x004c0660
            g_gameApp.Fatal("This graphics accelerator card does not support drawing in a window!\nChange to fullscreen mode.\n");
        }

        desc.dwFlags        = DDSD_CAPS;
        desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE;

        hr = g_pDDrawDevice->CreateSurface(&desc, &g_ddrawDevice.pDDrawPrimary, 0);

        if (hr != DD_OK)
        {
            // STRING: HYPERX 0x004c0788
            g_gameApp.Error("CreateSurface FAILED!\n%s\n", DDraw::ErrorToString(hr));

            return DDraw::FailInitialization(hr & DDRAW_ERROR_CODE_MASK);
        }

        g_nSurfaceMemoryUsed = g_nSurfaceMemoryUsed + (g_dwDisplayBpp >> 3) * g_dwDisplayWidth * g_dwDisplayHeight;

        if (g_bUseOffscreenSurface != 0)
        {
            g_ddrawDevice.pBackSurface = DDraw::CreateOffscreenSurface(g_dwDisplayWidth, g_dwDisplayHeight, 1, 0);
        }
        else
        {
            g_ddrawDevice.pBackSurface = DDraw::CreateOffscreenSurface(g_dwDisplayWidth, g_dwDisplayHeight, 0, 0);
        }

        if (g_ddrawDevice.pBackSurface == 0)
        {
            return DDraw::FailInitialization(0);
        }

        hr = g_pDDrawDevice->CreateClipper(0, &g_ddrawDevice.pDDrawClipper, 0);

        if (hr != DD_OK)
        {
            return DDraw::FailInitialization(hr & DDRAW_ERROR_CODE_MASK);
        }

        hr = g_ddrawDevice.pDDrawClipper->SetHWnd(0, g_hWndGame);

        if (hr != DD_OK)
        {
            return DDraw::FailInitialization(hr & DDRAW_ERROR_CODE_MASK);
        }

        hr = g_ddrawDevice.pDDrawPrimary->SetClipper(g_ddrawDevice.pDDrawClipper);

        if (hr != DD_OK)
        {
            return DDraw::FailInitialization(hr & DDRAW_ERROR_CODE_MASK);
        }
    }

    if (g_bUseOffscreenSurface != 0)
    {
        g_ddrawDevice.pOffscreenSurface = g_ddrawDevice.pBackSurface;
    }

    g_ddrawDevice.bD3DActive = D3DDevice::Init(g_ddrawDevice.pBackSurface);
    g_cD3DActive             = g_ddrawDevice.bD3DActive;

    format.dwSize = sizeof(format);

    g_ddrawDevice.pDDrawPrimary->GetPixelFormat(&format);

    if (format.dwRGBBitCount == 8)
    {
        g_nKeyColour = 0xff;
    }
    else
    {
        g_nKeyColour = format.dwRBitMask | format.dwGBitMask | format.dwBBitMask;
    }

    if (g_bBackBufferColourKey != 0)
    {
        DDCOLORKEY key;

        key.dwColorSpaceLowValue  = g_nKeyColour;
        key.dwColorSpaceHighValue = g_nKeyColour;

        g_ddrawDevice.pBackSurface->SetColorKey(DDCKEY_DESTBLT, &key);
    }

    DDraw::ClearAllBuffers();

    return 1;
}

#define CLOCK_POINTS 0x14

#define METER_UNUSED 10

#define METER_COLOUR_MODE 1

#define DAMAGE_BAR_STEPS 10

#define DAMAGE_COLOUR_MODE 1

#define DAMAGE_ORIGIN 2

#define DAMAGE_SCROLL_TARGET 0xff

#define RADAR_ORIGIN 100

// GLOBAL: HYPERX 0x004be6bc
extern int g_nHudColourMode;

#define BORDER_ORIGIN 2

// GLOBAL: HYPERX 0x004cda18
extern char g_szHudSampleText[128];

#define SCORE_POINTS 0xf

#define SCORE_SAMPLE 99

#define SMALL_POINTS 8

#define COLOUR_KEYED 1

#define IN_SYSTEM_MEMORY 1

#define SHOT_CAM_X 24

#define SHOT_CAM_Y 28

#define SHOT_CAM_COLOUR 0x505050

#define SHOT_CAM_POINTS 10

#define SHOT_POWER_ORIGIN 1

#define SHOT_POWER_SCROLL 0x1abf9c

#define TIMER_POINTS 0xf

inline void HUD::AssignName(String* pName, const char* psz)
{
    pName->Assign(String::SafeStrlen(psz), psz);
}

#define TICKER_POINTS 0xb

#define WIDGET_POINTS 9

// FUNCTION: HYPERX 0x0043b330
int __cdecl HUD::CreateSurfaces(void)
{
    Point           pt;
    char            szExitPrompt[128];
    char            szContinuePrompt[128];
    String*         pName;
    CDDTextSurface* pText;
    char*           pszSrc;

    if (g_ddrawDevice.bD3DActive == 0)
    {
        return 0;
    }

    int nResult = HUD::CreateFPSText();
    nResult += HUD::CreateSpeedMeter();
    nResult += HUD::CreateDamageAndRadar();

    g_hudSurfaces.pTeamAFrame = new CDDBitMap(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    if (g_hudSurfaces.pTeamAFrame == 0)
    {
        // STRING: HYPERX 0x004c0adc
        g_gameApp.Error("Cannot create surface for team A logo borders!");
    }
    else
    {
        nResult += g_hudSurfaces.pTeamAFrame
                       // STRING: HYPERX 0x004c0ad4
                       ->LoadFromFile("frdamgA", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    pt.Set(BORDER_ORIGIN, BORDER_ORIGIN);

    g_hudSurfaces.pTeamAFrame->SetClipRect(1, pt);

    g_hudSurfaces.pTeamABitmap = new CDDBitMap(g_hudSurfaces.pTeamAFrame);

    if (g_hudSurfaces.pTeamABitmap == 0)
    {
        // STRING: HYPERX 0x004c0aa8
        g_gameApp.Error("Cannot create surface for team A logos!\n");
    }
    else
    {
        nResult += g_hudSurfaces.pTeamABitmap
                       // STRING: HYPERX 0x004c0aa0
                       ->LoadFromFile("hudlos", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pMessageFrame = new CDDBitMap(g_hudSurfaces.pTeamAFrame);

    g_hudSurfaces.nUnusedB = 0;

    if (g_hudSurfaces.pMessageFrame == 0)
    {
        // STRING: HYPERX 0x004c0a74
        g_gameApp.Error("Cannot create surface for Message border!\n");
    }
    else
    {
        nResult += g_hudSurfaces.pMessageFrame
                       // STRING: HYPERX 0x004c0a6c
                       ->LoadFromFile("frmsg", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pMessageText = new CDDTextSurface(g_hudSurfaces.pMessageFrame);

    // STRING: HYPERX 0x004c0a44
    sprintf(g_szHudSampleText, "This is the Length of Longest String.");

    if (g_hudSurfaces.pMessageText == 0)
    {
        // STRING: HYPERX 0x004c0a20
        g_gameApp.Error("Cannot create surface for Messages!");
    }
    else
    {
        nResult += g_hudSurfaces.pMessageText
                       ->SetFont(g_strHudDefaultName.m_pchData, SCORE_POINTS, g_szHudSampleText, g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pPeriodFrame = new CDDBitMap;

    if (g_hudSurfaces.pPeriodFrame == 0)
    {
        // STRING: HYPERX 0x004c09f4
        g_gameApp.Error("Cannot create surface for Period border!\n");
    }
    else
    {
        nResult += g_hudSurfaces.pPeriodFrame
                       // STRING: HYPERX 0x004c09ec
                       ->LoadFromFile("frper", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pPeriodOverlay = new CDDBitMap(g_hudSurfaces.pPeriodFrame);

    if (g_hudSurfaces.pPeriodOverlay == 0)
    {
        // STRING: HYPERX 0x004c09c0
        g_gameApp.Error("Cannot create surface for Period light!\n");
    }
    else
    {
        nResult += g_hudSurfaces.pPeriodOverlay
                       // STRING: HYPERX 0x004c09b8
                       ->LoadFromFile("period", 0, g_bUseOffscreenSurface);
    }

    sprintf(g_szHomeScore, "%d", SCORE_SAMPLE);

    g_hudSurfaces.pTeamAScoreText = new CDDTextSurface(g_hudSurfaces.pTeamAFrame);

    if (g_hudSurfaces.pTeamAScoreText == 0)
    {
        // STRING: HYPERX 0x004c0988
        g_gameApp.Error("Cannot create surface for team A Score Text!");
    }
    else
    {
        nResult += g_hudSurfaces.pTeamAScoreText
                       ->SetFont(g_strHudDefaultName.m_pchData, SCORE_POINTS, g_szHomeScore, g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pSmallText = new CDDTextSurface(g_hudSurfaces.pTeamAFrame);

    if (g_hudSurfaces.pSmallText != 0)
    {
        nResult += g_hudSurfaces.pSmallText
                       // STRING: HYPERX 0x004c0984
                       ->SetFont(g_strHudDefaultName.m_pchData, SMALL_POINTS, "x4", COLOUR_KEYED, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pTeamBFrame = new CDDBitMap;

    if (g_hudSurfaces.pTeamBFrame == 0)
    {
        // STRING: HYPERX 0x004c0954
        g_gameApp.Error("Cannot create surface for team B logo borders!");
    }
    else
    {
        nResult += g_hudSurfaces.pTeamBFrame
                       // STRING: HYPERX 0x004c094c
                       ->LoadFromFile("frdamgB", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pTeamBFrame->SetClipRect(1, pt);

    g_hudSurfaces.pTeamBBitmap = new CDDBitMap(g_hudSurfaces.pTeamBFrame);

    if (g_hudSurfaces.pTeamBBitmap == 0)
    {
        // STRING: HYPERX 0x004c0924
        g_gameApp.Error("Cannot create surface for team B logos!");
    }
    else
    {
        nResult += g_hudSurfaces.pTeamBBitmap
                       // STRING: HYPERX 0x004c091c
                       ->LoadFromFile("hudsea", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    sprintf(g_szAwayScore, "%d", SCORE_SAMPLE);

    g_hudSurfaces.pTeamBScoreText = new CDDTextSurface(g_hudSurfaces.pTeamBFrame);

    if (g_hudSurfaces.pTeamBScoreText == 0)
    {
        // STRING: HYPERX 0x004c08ec
        g_gameApp.Error("Cannot create surface for team B Score Text!");
    }
    else
    {
        nResult += g_hudSurfaces.pTeamBScoreText
                       ->SetFont(g_strHudDefaultName.m_pchData, SCORE_POINTS, g_szAwayScore, g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pNetworkIcon = new CDDBitMap;

    if (g_hudSurfaces.pNetworkIcon == 0)
    {
        // STRING: HYPERX 0x004c08c4
        g_gameApp.Error("Cannot create surface for network icon!");
    }
    else
    {
        nResult += g_hudSurfaces.pNetworkIcon
                       // STRING: HYPERX 0x004c08bc
                       ->LoadFromFile("network", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pHeadCamFrame = new CDDBitMap;

    if (g_hudSurfaces.pHeadCamFrame == 0)
    {
        // STRING: HYPERX 0x004c0890
        g_gameApp.Error("Cannot create surface for second camera!");
    }
    else
    {
        if (g_ddrawDevice.bD3DActive != 0)
        {
            nResult += g_hudSurfaces.pHeadCamFrame
                           // STRING: HYPERX 0x004c0888
                           ->LoadFromFile("frshot", g_nHudColourMode, g_bUseOffscreenSurface);
        }
        else
        {
            nResult += g_hudSurfaces.pHeadCamFrame
                           ->LoadFromFile("frshot", g_nHudColourMode, IN_SYSTEM_MEMORY);
        }
    }

    pt.x = SHOT_CAM_X;
    pt.y = SHOT_CAM_Y;

    g_hudSurfaces.pHeadCamFrame->SetClipRect(1, pt);

    g_hudSurfaces.pHeadCamFrame->SetFillColor(DDraw::ColourToPixel(
        g_hudSurfaces.pHeadCamFrame->GetSurface(),
        SHOT_CAM_COLOUR));

    nResult += g_hudSurfaces.pHeadCamFrame
                   // STRING: HYPERX 0x004c087c
                   ->SetTextOverlay(g_strHudDefaultName.m_pchData, SHOT_CAM_POINTS, "Shot Camera", COLOUR_KEYED);

    g_hudSurfaces.pReticle = new CDDBitMap(g_hudSurfaces.pHeadCamFrame);

    if (g_hudSurfaces.pReticle == 0)
    {
        // STRING: HYPERX 0x004c084c
        g_gameApp.Error("Cannot create surface for camera reticle icon!");
    }
    else
    {
        nResult += g_hudSurfaces.pReticle
                       ->LoadFromFile("Reticl2", g_nHudColourMode, g_bUseOffscreenSurface);
    }

    g_hudSurfaces.pShotPowerMeter = new ScrollBitmap;

    if (g_hudSurfaces.pShotPowerMeter == 0)
    {
        // STRING: HYPERX 0x004c0818
        g_gameApp.Error("Cannot create surface for shot power meter!");
    }
    else
    {
        nResult += g_hudSurfaces.pShotPowerMeter
                       // STRING: HYPERX 0x004c0810
                       ->LoadFromFile("frshotp", COLOUR_KEYED, g_bUseOffscreenSurface);
    }

    pt.x = SHOT_POWER_ORIGIN;
    pt.y = SHOT_POWER_ORIGIN;

    g_hudSurfaces.pShotPowerMeter->SetClipRect(1, pt);
    g_hudSurfaces.pShotPowerMeter->SetColorMode(SHOT_POWER_SCROLL);
    g_hudSurfaces.pShotPowerMeter->SetVisible(0);

    g_hudSurfaces.pExitMatchText = new CDDTextSurface(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    if (g_hudSurfaces.pExitMatchText != 0)
    {
        g_hudSurfaces.pExitMatchString = new String(
            g_gameApp.m_stringPool.LoadText(g_gameApp.GetInstance(), STRING_EXIT_MATCH));

        pszSrc = g_hudSurfaces.pExitMatchString->m_pchData;

        strcpy(szExitPrompt, pszSrc);

        // STRING: HYPERX 0x004c080c
        strcat(szExitPrompt, "X");

        nResult += g_hudSurfaces.pExitMatchText
                       ->SetFont(g_strHudDefaultName.m_pchData, TIMER_POINTS, szExitPrompt, COLOUR_KEYED, g_bUseOffscreenSurface);

        pText = g_hudSurfaces.pExitMatchText;
        pName = &pText->m_strText;

        AssignName(pName, g_hudSurfaces.pExitMatchString->m_pchData);

        pText->RestoreContents();
        g_hudSurfaces.pExitMatchText->SetVisible(0);
    }

    g_hudSurfaces.pContinueText = new CDDTextSurface(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    if (g_hudSurfaces.pContinueText != 0)
    {
        g_hudSurfaces.pContinueString = new String(
            g_gameApp.m_stringPool.LoadText(g_gameApp.GetInstance(), STRING_CONTINUE));

        pszSrc = g_hudSurfaces.pContinueString->m_pchData;

        strcpy(szContinuePrompt, pszSrc);

        strcat(szContinuePrompt, "X");

        nResult += g_hudSurfaces.pContinueText
                       ->SetFont(g_strHudDefaultName.m_pchData, TIMER_POINTS, szContinuePrompt, COLOUR_KEYED, g_bUseOffscreenSurface);

        pText = g_hudSurfaces.pContinueText;
        pName = &pText->m_strText;

        AssignName(pName, g_hudSurfaces.pContinueString->m_pchData);

        pText->RestoreContents();
        g_hudSurfaces.pContinueText->SetVisible(0);
    }

    pText = new CDDTextSurface(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    g_hudSurfaces.apTeamNames[0] = pText;

    nResult += pText->SetFont(g_strHudDefaultName.m_pchData, TICKER_POINTS,
                              // STRING: HYPERX 0x004c07f0
                              "XXXXXXXXXXXXXXXXXXXXXXXXXXX",
                              COLOUR_KEYED,
                              g_bUseOffscreenSurface);

    pText = new CDDTextSurface(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    g_hudSurfaces.apTeamNames[1] = pText;

    nResult += pText->SetFont(g_strHudDefaultName.m_pchData, TICKER_POINTS, "XXXXXXXXXXXXXXXXXXXXXXXXXXX", COLOUR_KEYED, g_bUseOffscreenSurface);

    for (int i = 0; i < 8; i++)
    {
        pText = new CDDTextSurface(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

        g_hudSurfaces.apPlayerWidgets[i] = pText;

        nResult += pText->SetFont(g_strHudDefaultName.m_pchData, WIDGET_POINTS, "XXXXXXXXXXXXXXXXXXXXXXXXXXX", COLOUR_KEYED, g_bUseOffscreenSurface);
    }

    return nResult;
}

// The clipper is released until its count reaches nought.
// FUNCTION: HYPERX 0x0043bf80
void __stdcall DDraw::DestroyAllSurfaces(void)
{
    int i;

    if (g_cUseSoftRenderer != 0)
    {
        return;
    }

    if (g_ddrawDevice.pDDrawObject != 0)
    {
        D3DRend::ReleaseAllBuffers();

        if (g_ddrawDevice.pZoomSurface != 0)
        {
            g_ddrawDevice.pZoomSurface->Release();
            g_ddrawDevice.pZoomSurface = 0;
        }

        if (g_ddrawDevice.pDDrawClipper != 0)
        {
            while (g_ddrawDevice.pDDrawClipper->Release() != 0)
            {
            }

            g_ddrawDevice.pDDrawClipper = 0;
        }

        if (g_pDevicePalette != 0)
        {
            g_pDevicePalette->Release();
            g_pDevicePalette = 0;
        }

        if (g_ddrawDevice.pFpsText != 0)
        {
            delete g_ddrawDevice.pFpsText;
            g_ddrawDevice.pFpsText = 0;
        }

        if (g_hudSurfaces.pClockText != 0)
        {
            delete g_hudSurfaces.pClockText;
            g_hudSurfaces.pClockText = 0;
        }

        if (g_hudSurfaces.pTeamAScoreText != 0)
        {
            delete g_hudSurfaces.pTeamAScoreText;
            g_hudSurfaces.pTeamAScoreText = 0;
        }

        if (g_hudSurfaces.pTeamBScoreText != 0)
        {
            delete g_hudSurfaces.pTeamBScoreText;
            g_hudSurfaces.pTeamBScoreText = 0;
        }

        if (g_hudSurfaces.pSpeedMeter != 0)
        {
            delete g_hudSurfaces.pSpeedMeter;
            g_hudSurfaces.pSpeedMeter = 0;
        }

        if (g_hudSurfaces.pShotPowerMeter != 0)
        {
            delete g_hudSurfaces.pShotPowerMeter;
            g_hudSurfaces.pShotPowerMeter = 0;
        }

        if (g_hudSurfaces.pExitMatchText != 0)
        {
            String* pString;

            delete g_hudSurfaces.pExitMatchText;
            pString                      = g_hudSurfaces.pExitMatchString;
            g_hudSurfaces.pExitMatchText = 0;

            delete pString;

            g_hudSurfaces.pExitMatchString = 0;
        }

        if (g_hudSurfaces.pContinueText != 0)
        {
            String* pString;

            delete g_hudSurfaces.pContinueText;
            pString                     = g_hudSurfaces.pContinueString;
            g_hudSurfaces.pContinueText = 0;

            delete pString;

            g_hudSurfaces.pContinueString = 0;
        }

        for (i = 0; i < 2; i++)
        {
            if (g_hudSurfaces.apTeamNames[i] != 0)
            {
                delete g_hudSurfaces.apTeamNames[i];
                g_hudSurfaces.apTeamNames[i] = 0;
            }
        }

        for (i = 0; i < 8; i++)
        {
            if (g_hudSurfaces.apPlayerWidgets[i] != 0)
            {
                delete g_hudSurfaces.apPlayerWidgets[i];
                g_hudSurfaces.apPlayerWidgets[i] = 0;
            }
        }

        if (g_hudSurfaces.pTeamADamageBar1 != 0)
        {
            delete g_hudSurfaces.pTeamADamageBar1;
            g_hudSurfaces.pTeamADamageBar1 = 0;
        }

        if (g_hudSurfaces.pTeamADamageBar2 != 0)
        {
            delete g_hudSurfaces.pTeamADamageBar2;
            g_hudSurfaces.pTeamADamageBar2 = 0;
        }

        if (g_hudSurfaces.pTeamBDamageBar1 != 0)
        {
            delete g_hudSurfaces.pTeamBDamageBar1;
            g_hudSurfaces.pTeamBDamageBar1 = 0;
        }

        if (g_hudSurfaces.pTeamBDamageBar2 != 0)
        {
            delete g_hudSurfaces.pTeamBDamageBar2;
            g_hudSurfaces.pTeamBDamageBar2 = 0;
        }

        if (g_hudSurfaces.pRadar != 0)
        {
            delete g_hudSurfaces.pRadar;
            g_hudSurfaces.pRadar = 0;
        }

        if (g_hudSurfaces.pTeamAFrame != 0)
        {
            delete g_hudSurfaces.pTeamAFrame;
            g_hudSurfaces.pTeamAFrame = 0;
        }

        if (g_hudSurfaces.pTeamABitmap != 0)
        {
            delete g_hudSurfaces.pTeamABitmap;
            g_hudSurfaces.pTeamABitmap = 0;
        }

        if (g_hudSurfaces.pTeamBFrame != 0)
        {
            delete g_hudSurfaces.pTeamBFrame;
            g_hudSurfaces.pTeamBFrame = 0;
        }

        if (g_hudSurfaces.pTeamBBitmap != 0)
        {
            delete g_hudSurfaces.pTeamBBitmap;
            g_hudSurfaces.pTeamBBitmap = 0;
        }

        if (g_hudSurfaces.pNetworkIcon != 0)
        {
            delete g_hudSurfaces.pNetworkIcon;
            g_hudSurfaces.pNetworkIcon = 0;
        }

        if (g_hudSurfaces.pHeadCamFrame != 0)
        {
            delete g_hudSurfaces.pHeadCamFrame;
            g_hudSurfaces.pHeadCamFrame = 0;
        }

        if (g_hudSurfaces.pReticle != 0)
        {
            delete g_hudSurfaces.pReticle;
            g_hudSurfaces.pReticle = 0;
        }

        if (g_hudSurfaces.pPeriodFrame != 0)
        {
            delete g_hudSurfaces.pPeriodFrame;
            g_hudSurfaces.pPeriodFrame = 0;
        }

        if (g_hudSurfaces.pMessageFrame != 0)
        {
            delete g_hudSurfaces.pMessageFrame;
            g_hudSurfaces.pMessageFrame = 0;
        }

        if (g_hudSurfaces.pMessageText != 0)
        {
            delete g_hudSurfaces.pMessageText;
            g_hudSurfaces.pMessageText = 0;
        }

        if (g_hudSurfaces.pPeriodOverlay != 0)
        {
            delete g_hudSurfaces.pPeriodOverlay;
            g_hudSurfaces.pPeriodOverlay = 0;
        }

        if (g_hudSurfaces.pSmallText != 0)
        {
            delete g_hudSurfaces.pSmallText;
            g_hudSurfaces.pSmallText = 0;
        }

        if (g_ddrawDevice.pDDrawPrimary != 0)
        {
            g_ddrawDevice.pDDrawPrimary->Release();
            g_ddrawDevice.pDDrawPrimary = 0;
        }

        if (g_ddrawDevice.pBackSurface != 0 && g_bForceSoftwareRender != 0 && g_bHWFlipAvailable == 0)
        {
            g_ddrawDevice.pBackSurface->Release();
            g_ddrawDevice.pBackSurface = 0;
        }

        g_ddrawDevice.pDDrawObject->Release();
        g_ddrawDevice.pDDrawObject = 0;
        g_pDDrawDevice             = 0;
    }

    DDraw::ChangeDisplayMode(1);

    delete g_pSavedDesktopMode;
    g_pSavedDesktopMode = 0;
}

// A windowed run never changes the mode and answers success.
// FUNCTION: HYPERX 0x0043c310
int __cdecl DDraw::ChangeDisplayMode(int nMode)
{
    DEVMODEA dm;

#ifdef HYPERX_BRINGUP
    return 0;
#endif

    if (g_ddrawDevice.bIsPrimary == 0)
    {
        LONG lResult = 0;

        dm.dmSize = sizeof(dm);

        switch (nMode)
        {
            case 0:
                dm.dmPelsWidth  = g_dwDisplayWidth;
                dm.dmPelsHeight = g_dwDisplayHeight;
                dm.dmBitsPerPel = g_dwDisplayBpp;
                dm.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
                lResult         = ChangeDisplaySettingsA(&dm, 0);
                break;

            case 1:
                if (g_ddrawDevice.bD3DActive != 0 && g_pSavedDesktopMode != 0)
                {
                    dm.dmPelsWidth  = g_pSavedDesktopMode->nWidth;
                    dm.dmPelsHeight = g_pSavedDesktopMode->nHeight;
                    dm.dmBitsPerPel = g_pSavedDesktopMode->nBpp;
                    dm.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
                    lResult         = ChangeDisplaySettingsA(&dm, 0);
                }
                break;
        }

        switch (lResult)
        {
            case 0:
                goto done;

            case 1:
                g_hrLastResult = g_ddrawDevice.pDDrawObject->SetCooperativeLevel(g_hWndGame, DDSCL_NORMAL);
                DDraw::ChangeDisplayMode(1);
                g_gameApp.Fatal(g_gameApp.m_stringPool.LoadText(g_gameApp.m_hInstance, STRING_HIGH_COLOR));
                break;
        }

    done:
        return lResult;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0043c410
int DDraw::GetLastError(void)
{
    return g_hrLastResult;
}

// FUNCTION: HYPERX 0x0043c420
int __stdcall DDraw::RestoreAllSurfaces(void)
{
    int hr = g_ddrawDevice.pDDrawPrimary->Restore();

    if (hr != 0)
    {
        return hr;
    }

    if (g_ddrawDevice.pBackSurface != 0 && g_bForceSoftwareRender != 0)
    {
        const int hrBackBuffer = g_ddrawDevice.pBackSurface->Restore();

        if (hrBackBuffer != 0)
        {
            return hrBackBuffer;
        }
    }

    if (g_ddrawDevice.pOffscreenSurface != 0)
    {
        hr = g_ddrawDevice.pOffscreenSurface->Restore();
    }

    if (g_ddrawDevice.bD3DActive != 0)
    {
        g_hrLastResult = CD3DTextureManager::RestoreAllTextures();
    }

    if (g_ddrawDevice.pFpsText != 0)
    {
        g_hrLastResult = g_ddrawDevice.pFpsText->OnSurfaceLost();
        g_ddrawDevice.pFpsText->DrawText(g_szFpsText);
    }

    if (g_hudSurfaces.pClockText != 0)
    {
        g_hrLastResult = g_hudSurfaces.pClockText->OnSurfaceLost();
        g_hudSurfaces.pClockText->DrawText(g_szClockText);
    }

    if (g_hudSurfaces.pSpeedMeter != 0)
    {
        g_hrLastResult = g_hudSurfaces.pSpeedMeter->OnSurfaceLost();
    }

    if (g_hudSurfaces.pRadar != 0)
    {
        g_hrLastResult = g_hudSurfaces.pRadar->OnSurfaceLost();
    }

    if (g_hudSurfaces.pTeamAFrame != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamAFrame->OnSurfaceLost();
        g_hudSurfaces.pTeamAFrame->Draw();
    }

    if (g_hudSurfaces.pTeamABitmap != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamABitmap->OnSurfaceLost();
        g_hudSurfaces.pTeamABitmap->Draw();
    }

    if (g_hudSurfaces.pTeamBFrame != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamBFrame->OnSurfaceLost();
        g_hudSurfaces.pTeamBFrame->Draw();
    }

    if (g_hudSurfaces.pTeamBBitmap != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamBBitmap->OnSurfaceLost();
        g_hudSurfaces.pTeamBBitmap->Draw();
    }

    if (g_hudSurfaces.pTeamADamageBar1 != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamADamageBar1->OnSurfaceLost();
    }

    if (g_hudSurfaces.pTeamADamageBar2 != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamADamageBar2->OnSurfaceLost();
    }

    if (g_hudSurfaces.pTeamBDamageBar1 != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamBDamageBar1->OnSurfaceLost();
    }

    if (g_hudSurfaces.pTeamBDamageBar2 != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamBDamageBar2->OnSurfaceLost();
    }

    if (g_hudSurfaces.pNetworkIcon != 0)
    {
        g_hrLastResult = g_hudSurfaces.pNetworkIcon->OnSurfaceLost();
        g_hudSurfaces.pNetworkIcon->Draw();
    }

    if (g_hudSurfaces.pReticle != 0)
    {
        g_hrLastResult = g_hudSurfaces.pReticle->OnSurfaceLost();
        g_hudSurfaces.pReticle->Draw();
    }

    if (g_hudSurfaces.pShotPowerMeter != 0)
    {
        g_hrLastResult = g_hudSurfaces.pShotPowerMeter->OnSurfaceLost();
    }

    if (g_hudSurfaces.pExitMatchText != 0)
    {
        g_hrLastResult = g_hudSurfaces.pExitMatchText->OnSurfaceLost();
    }

    if (g_hudSurfaces.pContinueText != 0)
    {
        g_hrLastResult = g_hudSurfaces.pContinueText->OnSurfaceLost();
    }

    if (g_hudSurfaces.pTeamAScoreText != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamAScoreText->OnSurfaceLost();
        g_hudSurfaces.pTeamAScoreText->DrawText(0);
    }

    if (g_hudSurfaces.pTeamBScoreText != 0)
    {
        g_hrLastResult = g_hudSurfaces.pTeamBScoreText->OnSurfaceLost();
        g_hudSurfaces.pTeamBScoreText->DrawText(0);
    }

    if (g_hudSurfaces.pPeriodFrame != 0)
    {
        g_hrLastResult = g_hudSurfaces.pPeriodFrame->OnSurfaceLost();
        g_hudSurfaces.pPeriodFrame->Draw();
    }

    if (g_hudSurfaces.pMessageFrame != 0)
    {
        g_hrLastResult = g_hudSurfaces.pMessageFrame->OnSurfaceLost();
        g_hudSurfaces.pMessageFrame->Draw();
    }

    if (g_hudSurfaces.pPeriodOverlay != 0)
    {
        g_hrLastResult = g_hudSurfaces.pPeriodOverlay->OnSurfaceLost();
        g_hudSurfaces.pPeriodOverlay->Draw();
    }

    if (g_hudSurfaces.pSmallText != 0)
    {
        g_hrLastResult = g_hudSurfaces.pSmallText->OnSurfaceLost();
    }

    g_hrLastResult = DDraw::GetLastError();

    return hr;
}

// The millisecond line is formatted before its surface exists, so it measures "0:00".
// FUNCTION: HYPERX 0x0043c700
int __stdcall HUD::CreateFPSText(void)
{
    int nResult = 0;

    // STRING: HYPERX 0x004c0b1c
    AssignName(&g_strHudDefaultName, "MS Sans Serif");

    g_ddrawDevice.pFpsText = new CDDTextSurface(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    if (g_ddrawDevice.pFpsText != 0)
    {
        nResult = g_ddrawDevice.pFpsText
                      // STRING: HYPERX 0x004c0b0c
                      ->SetFontScaled(g_strHudDefaultName.m_pchData, "FPS 00000.00", COLOUR_KEYED, g_bUseOffscreenSurface);
    }

    sprintf(g_szClockText, g_pszClockFormat, 0, 0);

    g_hudSurfaces.pClockText = new CDDTextSurface;

    if (g_hudSurfaces.pClockText != 0)
    {
        nResult += g_hudSurfaces.pClockText
                       ->SetFont(g_strHudDefaultName.m_pchData, CLOCK_POINTS, g_szClockText, COLOUR_KEYED, g_bUseOffscreenSurface);
    }

    return nResult;
}

// A meter that exists but did not load still reaches the caller as a failure.
// FUNCTION: HYPERX 0x0043c830
int __cdecl HUD::CreateSpeedMeter(void)
{
    Point pt;
    int   nResult;

    g_hudSurfaces.pSpeedMeter = new FramedBarBitmap(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    if (g_hudSurfaces.pSpeedMeter == 0)
    {
        return 0;
    }

    nResult = g_hudSurfaces.pSpeedMeter
                  // STRING: HYPERX 0x004c0b2c
                  ->LoadFromFile("speed", METER_UNUSED, METER_COLOUR_MODE, g_bUseOffscreenSurface);

    pt.Set(1, 1);

    g_hudSurfaces.pSpeedMeter->SetClipRect(1, pt);

    return nResult;
}

// The answer is the five loads summed. The Point set to a hundred is never read.
// FUNCTION: HYPERX 0x0043c900
int __cdecl HUD::CreateDamageAndRadar(void)
{
    Point pt;
    Point ptRadar;
    Vec3  vScale;
    int   nResult;

    g_hudSurfaces.pTeamADamageBar1 = new DamageBar(g_pDDrawDevice, g_ddrawDevice.pDDrawPrimary, g_ddrawDevice.pBackSurface);

    if (g_hudSurfaces.pTeamADamageBar1 == 0)
    {
        return 0;
    }

    nResult = g_hudSurfaces.pTeamADamageBar1
                  // STRING: HYPERX 0x004c0b50
                  ->InitAndLoad("BDamgBar", DAMAGE_BAR_STEPS, DAMAGE_COLOUR_MODE, g_bUseOffscreenSurface);

    pt.Set(DAMAGE_ORIGIN, DAMAGE_ORIGIN);

    g_hudSurfaces.pTeamADamageBar1->SetClipRect(1, pt);
    g_hudSurfaces.pTeamADamageBar1->SetColorMode(DAMAGE_SCROLL_TARGET);

    g_hudSurfaces.pTeamADamageBar2 = new DamageBar;

    if (g_hudSurfaces.pTeamADamageBar2 == 0)
    {
        return 0;
    }

    nResult += g_hudSurfaces.pTeamADamageBar2
                   // STRING: HYPERX 0x004c0b50
                   ->InitAndLoad("BDamgBar", DAMAGE_BAR_STEPS, DAMAGE_COLOUR_MODE, g_bUseOffscreenSurface);

    g_hudSurfaces.pTeamADamageBar2->SetClipRect(1, pt);
    g_hudSurfaces.pTeamADamageBar2->SetColorMode(DAMAGE_SCROLL_TARGET);

    g_hudSurfaces.pTeamBDamageBar1 = new DamageBar;

    if (g_hudSurfaces.pTeamBDamageBar1 == 0)
    {
        return 0;
    }

    nResult += g_hudSurfaces.pTeamBDamageBar1
                   // STRING: HYPERX 0x004c0b44
                   ->InitAndLoad("RDamgBar", DAMAGE_BAR_STEPS, DAMAGE_COLOUR_MODE, g_bUseOffscreenSurface);

    g_hudSurfaces.pTeamBDamageBar1->SetClipRect(1, pt);
    g_hudSurfaces.pTeamBDamageBar1->SetColorMode(DAMAGE_SCROLL_TARGET);

    g_hudSurfaces.pTeamBDamageBar2 = new DamageBar;

    if (g_hudSurfaces.pTeamBDamageBar2 == 0)
    {
        return 0;
    }

    nResult += g_hudSurfaces.pTeamBDamageBar2
                   // STRING: HYPERX 0x004c0b44
                   ->InitAndLoad("RDamgBar", DAMAGE_BAR_STEPS, DAMAGE_COLOUR_MODE, g_bUseOffscreenSurface);

    g_hudSurfaces.pTeamBDamageBar2->SetClipRect(1, pt);
    g_hudSurfaces.pTeamBDamageBar2->SetColorMode(DAMAGE_SCROLL_TARGET);

    g_hudSurfaces.pRadar = new Radar;

    if (g_hudSurfaces.pRadar == 0)
    {
        return 0;
    }

    ptRadar.Set(RADAR_ORIGIN, RADAR_ORIGIN);

    vScale.x = g_arenaEllipsoid.GetRadiusX();
    vScale.y = g_arenaEllipsoid.GetRadiusY();
    vScale.z = g_arenaEllipsoid.GetRadiusZ();

    if (g_hudSurfaces.bHud320 != 0)
    {
        // STRING: HYPERX 0x004c0b3c
        nResult += g_hudSurfaces.pRadar->Init("320radr", &vScale, g_bUseOffscreenSurface);
    }
    else
    {
        // STRING: HYPERX 0x004c0b34
        nResult += g_hudSurfaces.pRadar->Init("frradar", &vScale, g_bUseOffscreenSurface);
    }

    return nResult;
}

// A lost surface is restored and a failed restore is fatal; any other failure is an error.
// FUNCTION: HYPERX 0x0043cc80
int __cdecl DDraw::ClearBackBuffer(unsigned int nColour)
{
    DDBLTFX fx;

    fx.dwSize = sizeof(DDBLTFX);

    if (g_ddrawDevice.bD3DActive == 0)
    {
        if (g_bUseOffscreenSurface != 0)
        {
            fx.dwFillColor = DDraw::ColourToPixel(g_ddrawDevice.pOffscreenSurface, nColour);

            g_hrLastResult = g_ddrawDevice.pOffscreenSurface->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
        }
        else
        {
            fx.dwFillColor = DDraw::ColourToPixel(g_ddrawDevice.pBackSurface, nColour);

            g_hrLastResult = g_ddrawDevice.pBackSurface->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
        }

        if (g_hrLastResult == DDERR_SURFACELOST)
        {
            if (g_bUseOffscreenSurface != 0)
            {
                g_hrLastResult = g_ddrawDevice.pOffscreenSurface->Restore();

                if (g_hrLastResult != 0)
                {
                    // STRING: HYPERX 0x004c0bb8
                    g_gameApp.Fatal("DDClearBackBuffer: Paint Surface Restore failed!\n%s\n",
                                    DDraw::ErrorToString(g_hrLastResult));
                }
            }
            else
            {
                g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Restore();

                if (g_hrLastResult != 0)
                {
                    // STRING: HYPERX 0x004c0b80
                    g_gameApp.Fatal("DDClearBackBuffer: Primary Surface Restore failed!\n%s\n",
                                    DDraw::ErrorToString(g_hrLastResult));
                }
            }
        }

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c0b5c
            g_gameApp.Error("DDClearBackBuffer: Blt failed!\n%s\n",
                            DDraw::ErrorToString(g_hrLastResult));
        }

        return 1;
    }

    return D3DViewport::Clear();
}

// Ghidra calls this DDraw::InitClipper; it touches no clipper.

#define CLEAR_PASSES 4

// FUNCTION: HYPERX 0x0043cda0
int __stdcall DDraw::ClearAllBuffers(void)
{
    DDBLTFX bltfx;

    UpdateWindow(g_hWndGame);

    bltfx.dwSize      = sizeof(bltfx);
    bltfx.dwFillColor = DDraw::ColourToPixel(g_ddrawDevice.pBackSurface, 0);

    if (g_bFullScreen != 0)
    {
        for (int nPass = 0; nPass < CLEAR_PASSES; nPass++)
        {
            g_hrLastResult =
                g_ddrawDevice.pBackSurface->Blt(0, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &bltfx);

            if (g_hrLastResult != 0)
            {
                return 0;
            }

            g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Flip(0, DDFLIP_WAIT);

            if (g_hrLastResult != 0)
            {
                return 0;
            }
        }
    }
    else
    {
        g_hrLastResult =
            g_ddrawDevice.pDDrawPrimary->Blt(&g_rcBltDest, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &bltfx);

        if (g_hrLastResult != 0)
        {
            return 0;
        }
    }

    return 1;
}

// The page lock needs IDirectDrawSurface2, but a surface without it is still usable.

// Declared here so the marker has a name; dxguid.lib supplies it.
// GLOBAL: HYPERX 0x004b4f20
EXTERN_C const GUID IID_IDirectDrawSurface2;

// FUNCTION: HYPERX 0x0043ce60
LPDIRECTDRAWSURFACE DDraw::CreateOffscreenSurface(int nWidth, int nHeight, int bSystemMemory, int bColourKey)
{
    DDCOLORKEY    key;
    DDSURFACEDESC desc;

    LPDIRECTDRAWSURFACE  pSurface  = 0;
    LPDIRECTDRAWSURFACE2 pSurface2 = 0;

    memset(&desc, 0, sizeof(desc));

    desc.dwSize         = sizeof(desc);
    desc.dwFlags        = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN;

    if (bSystemMemory != 0)
    {
        desc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    }

    desc.dwHeight = nHeight;
    desc.dwWidth  = nWidth;

    g_hrLastResult = g_pDDrawDevice->CreateSurface(&desc, &pSurface, 0);

    if (g_hrLastResult == DD_OK)
    {
        g_hrLastResult = pSurface->QueryInterface(IID_IDirectDrawSurface2, (void**)&pSurface2);

        if (g_hrLastResult == DD_OK)
        {
            g_hrLastResult = pSurface2->PageLock(0);
        }

        if (bSystemMemory == 0)
        {
            g_nSurfaceMemoryUsed += (g_dwDisplayBpp >> 3) * nWidth * nHeight;
        }

        if (bColourKey != 0 && g_bBackBufferColourKey == 0)
        {
            key.dwColorSpaceLowValue  = g_nKeyColour;
            key.dwColorSpaceHighValue = g_nKeyColour;

            g_hrLastResult = pSurface->SetColorKey(DDCKEY_SRCBLT, &key);
        }
    }
    else
    {
        // STRING: HYPERX 0x004c0bf0
        g_gameApp.Error("CreateSurface FAILED, rc = %ld! %s\n", HRESULT_CODE(g_hrLastResult), DDraw::ErrorToString(g_hrLastResult));

        pSurface = 0;
    }

    if (pSurface2 != 0)
    {
        return (LPDIRECTDRAWSURFACE)pSurface2;
    }

    return pSurface;
}

// The surface's pixel for a colour, in through GDI and back through Lock; no device context answers nought.

// FUNCTION: HYPERX 0x0043cfc0
unsigned int __cdecl DDraw::ColourToPixel(IDirectDrawSurface* pSurface,
                                          unsigned int        nColour)
{
    DDSURFACEDESC ddsd;
    HDC           hdc;

    unsigned int nPixel = CLR_INVALID;

    g_hrD3DResult = pSurface->GetDC(&hdc);

    if (g_hrD3DResult == 0)
    {
        const COLORREF nSaved = GetPixel(hdc, 0, 0);

        SetPixel(hdc, 0, 0, nColour);

        g_hrD3DResult = pSurface->ReleaseDC(hdc);

        ddsd.dwSize = sizeof(ddsd);

        if (pSurface->Lock(0, &ddsd, DDLOCK_WAIT, 0) == 0)
        {
            nPixel = ((1 << ddsd.ddpfPixelFormat.dwRGBBitCount) - 1) & *(unsigned int*)ddsd.lpSurface;

#ifdef HYPERX_BRINGUP
            // GDI and Lock no longer see the same memory, so the value is computed from the masks.
            if (nPixel == 0)
            {
                nPixel = BringUp::ColourFromMasks(nColour, ddsd.ddpfPixelFormat.dwRBitMask, ddsd.ddpfPixelFormat.dwGBitMask, ddsd.ddpfPixelFormat.dwBBitMask);
            }
#endif

            pSurface->Unlock(0);
        }

        g_hrD3DResult = pSurface->GetDC(&hdc);

        if (g_hrD3DResult == 0)
        {
            SetPixel(hdc, 0, 0, nSaved);

            g_hrD3DResult = pSurface->ReleaseDC(hdc);

            return nPixel;
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x0043d0b0
int __stdcall DDraw::Flip(void)
{
#ifdef HYPERX_BRINGUP
    // The Direct3D path's frame is snapshotted as it reaches the screen.
    BringUp::SaveD3DFrame("frame");
#endif

    return g_ddrawDevice.pDDrawPrimary->Flip(0, DDFLIP_WAIT) == 0;
}

// FUNCTION: HYPERX 0x0043d0d0
int __stdcall DDraw::BltFrontFromBack(void)
{
#ifdef HYPERX_BRINGUP
    BringUp::SaveD3DFrame("frame");
#endif

    const HRESULT hr = g_ddrawDevice.pDDrawPrimary->Blt(&g_rcBltDest, g_ddrawDevice.pBackSurface, 0, DDBLT_WAIT, 0);

    if (hr == DDERR_SURFACELOST)
    {
        return DDraw::RestoreAllSurfaces();
    }

    return hr;
}

// FUNCTION: HYPERX 0x0043d100
int __stdcall DDraw::FlipOrBlt(void)
{
#ifdef HYPERX_BRINGUP
    BringUp::SaveD3DFrame("frame");
#endif

    if (g_bFullScreen != 0)
    {
        if (g_bForceSoftwareRender != 0 && g_bHWFlipAvailable == 0)
        {
            return DDraw::BltFrontFromBack();
        }

        return DDraw::Flip();
    }

    return DDraw::BltFrontFromBack();
}

// A failure returns zeroes, which read as a card with no memory.
// FUNCTION: HYPERX 0x0043d130
VideoMemory* DDraw::GetVideoMemory(VideoMemory* pMemory, IDirectDraw* pDevice)
{
    DDrawCaps caps;
    Point     vidMem;

    vidMem.Set(0, 0);

    caps.dwSize = sizeof(caps);

    g_hrLastResult = pDevice->GetCaps((LPDDCAPS)&caps, 0);

    if (g_hrLastResult != 0)
    {
        // STRING: HYPERX 0x004c0c14
        g_gameApp.Error("GetCaps failed!\n%s\n", DDraw::ErrorToString(g_hrLastResult));

        pMemory->nTotal = vidMem.x;
        pMemory->nFree  = vidMem.y;

        return pMemory;
    }

    pMemory->nTotal = caps.dwVidMemTotal;
    pMemory->nFree  = caps.dwVidMemFree;

    return pMemory;
}

// FUNCTION: HYPERX 0x0043d1c0
int __cdecl DDraw::GetSurfaceDesc(DDSURFACEDESC* pDesc, struct IDirectDrawSurface* pSurface)
{
    memset(pDesc, 0, sizeof(*pDesc));

    pDesc->dwSize = sizeof(*pDesc);

    g_hrD3DResult = pSurface->GetSurfaceDesc(pDesc);

    return g_hrD3DResult == 0;
}

// FUNCTION: HYPERX 0x0043d200
void __stdcall DDraw::LockBackBuffer(void)
{
    DDSURFACEDESC ddsd;

    ddsd.dwSize = sizeof(ddsd);

    if (g_bUseOffscreenSurface != 0)
    {
        g_hrLastResult = g_ddrawDevice.pOffscreenSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);

        while (g_hrLastResult == DDERR_SURFACELOST)
        {
            g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Restore();

            if (g_hrLastResult != 0)
            {
                // STRING: HYPERX 0x004c0cbc
                g_gameApp.Fatal("PaintSurface: Primary Surface Restore failed!\n%s\n", DDraw::ErrorToString(g_hrLastResult));
            }

            g_hrLastResult = g_ddrawDevice.pOffscreenSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
        }

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c0c8c
            g_gameApp.Fatal("PaintSurface: lpPaintSurface lock failed!\n%s\n", DDraw::ErrorToString(g_hrLastResult));
        }
    }
    else
    {
        g_hrLastResult = g_ddrawDevice.pBackSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);

        while (g_hrLastResult == DDERR_SURFACELOST)
        {
            g_hrLastResult = DDraw::RestoreAllSurfaces();

            if (g_hrLastResult != 0)
            {
                // STRING: HYPERX 0x004c0c54
                g_gameApp.Fatal("PaintSurface: Primary Surface Restore failed!\n%s %d\n", DDraw::ErrorToString(g_hrLastResult), g_hrLastResult & DDRAW_ERROR_CODE_MASK);
            }

            g_hrLastResult = g_ddrawDevice.pBackSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
        }

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c0c28
            g_gameApp.Fatal("PaintSurface: BackBuffer lock failed!\n%s\n", DDraw::ErrorToString(g_hrLastResult));
        }
    }

    g_cameraViewportBlock.nPitch = ddsd.lPitch;
    g_nSurfacePitchCopy          = ddsd.lPitch;
    g_cameraViewportBlock.pBits  = ddsd.lpSurface;
    g_pBackBufferBitsCopy        = (unsigned char*)ddsd.lpSurface;
    g_nBackBufferWidth           = g_dwDisplayWidth;
    g_nBackBufferHeight          = g_dwDisplayHeight;
}

// FUNCTION: HYPERX 0x0043d370
void __stdcall DDraw::UnlockBackBuffer(void)
{
    if (g_bUseOffscreenSurface)
    {
        g_ddrawDevice.pOffscreenSurface->Unlock(0);
    }
    else
    {
        g_ddrawDevice.pBackSurface->Unlock(0);
    }
}

// Returns an uninitialised value when there is no surface, as the original does.
// FUNCTION: HYPERX 0x0043d3a0
HDC __stdcall DDraw::GetBackBufferDC(void)
{
    HDC                 hdc;
    LPDIRECTDRAWSURFACE pSurface;

    if (g_bUseOffscreenSurface)
    {
        pSurface = g_ddrawDevice.pOffscreenSurface;
    }
    else
    {
        pSurface = g_ddrawDevice.pBackSurface;
    }

    if (pSurface != 0)
    {
        if (pSurface->GetDC(&hdc) != 0)
        {
            return 0;
        }
    }

    return hdc;
}

// FUNCTION: HYPERX 0x0043d3e0
void DDraw::ReleaseBackBufferDC(HDC hdc)
{
    if (hdc != 0)
    {
        if (g_bUseOffscreenSurface)
        {
            g_ddrawDevice.pOffscreenSurface->ReleaseDC(hdc);
        }
        else
        {
            g_ddrawDevice.pBackSurface->ReleaseDC(hdc);
        }
    }
}

// A failed lock on the back buffer is fatal.
// FUNCTION: HYPERX 0x0043d410
void __stdcall DDraw::PaintSurface(void)
{
    DDSURFACEDESC ddsd;

    ddsd.dwSize                  = sizeof(ddsd);
    const Texture* const pTarget = g_pScene->m_pTarget;

    if (g_ddrawDevice.bD3DActive != 0)
    {
        g_hrLastResult = Renderer::RenderFrame(pTarget);

        return;
    }

    if (g_pScene->GetSlot() == CAMERA_MAIN_SLOT)
    {
        DDraw::ClearBackBuffer(CLEAR_COLOUR);
    }

    if (g_bUseOffscreenSurface != 0)
    {
        g_hrLastResult = g_ddrawDevice.pOffscreenSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);

        while (g_hrLastResult == DDERR_SURFACELOST)
        {
            g_hrLastResult = g_ddrawDevice.pDDrawPrimary->Restore();

            if (g_hrLastResult != 0)
            {
                // STRING: HYPERX 0x004c0cbc
                g_gameApp.Fatal("PaintSurface: Primary Surface Restore failed!\n%s\n",
                                DDraw::ErrorToString(g_hrLastResult));
            }

            g_hrLastResult = g_ddrawDevice.pOffscreenSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
        }

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c0c8c
            g_gameApp.Error("PaintSurface: lpPaintSurface lock failed!\n%s\n",
                            DDraw::ErrorToString(g_hrLastResult));

            return;
        }

        g_cameraViewportBlock.nPitch = ddsd.lPitch;
        g_nSurfacePitchCopy          = ddsd.lPitch;
        g_cameraViewportBlock.pBits  = ddsd.lpSurface;
        g_pBackBufferBitsCopy        = (unsigned char*)ddsd.lpSurface;

        Renderer::DrawWholeScene();

        g_ddrawDevice.pOffscreenSurface->Unlock(0);

        return;
    }

    g_hrLastResult = g_ddrawDevice.pBackSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);

    while (g_hrLastResult == DDERR_SURFACELOST)
    {
        g_hrLastResult = DDraw::RestoreAllSurfaces();

        if (g_hrLastResult != 0)
        {
            // STRING: HYPERX 0x004c0c54
            g_gameApp.Fatal("PaintSurface: Primary Surface Restore failed!\n%s %d\n",
                            DDraw::ErrorToString(g_hrLastResult),
                            g_hrLastResult & DDRAW_ERROR_CODE_MASK);
        }

        g_hrLastResult = g_ddrawDevice.pBackSurface->Lock(0, &ddsd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0);
    }

    if (g_hrLastResult != 0)
    {
        // STRING: HYPERX 0x004c0c28
        g_gameApp.Fatal("PaintSurface: BackBuffer lock failed!\n%s\n",
                        DDraw::ErrorToString(g_hrLastResult));
    }

    g_cameraViewportBlock.nPitch = ddsd.lPitch;
    g_nSurfacePitchCopy          = ddsd.lPitch;
    g_cameraViewportBlock.pBits  = ddsd.lpSurface;
    g_pBackBufferBitsCopy        = (unsigned char*)ddsd.lpSurface;

    Renderer::DrawWholeScene();

    g_ddrawDevice.pBackSurface->Unlock(0);
}

// FUNCTION: HYPERX 0x0043d600
int DDraw::FlipToGDISurface(int bShowGDI)
{
    int hr;

    if (bShowGDI != 0)
    {
        hr             = 0;
        g_hrLastResult = hr;

        if (g_pDDrawDevice != 0 && g_bFullScreen != 0)
        {
            if (g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE)
            {
                DDraw::BlitPrimaryToScreen();
            }

            hr             = g_pDDrawDevice->FlipToGDISurface();
            g_hrLastResult = hr;
        }
    }
    else
    {
        hr             = 0;
        g_hrLastResult = hr;
    }

    return hr == 0;
}

// FUNCTION: HYPERX 0x0043d660
int __stdcall DDraw::BlitPrimaryToScreen(void)
{
    if (g_ddrawDevice.bIsPrimary == 0)
    {
        InvalidateRect(g_gameApp.m_hWnd, 0, 0);

        const HDC hdcMem = CreateCompatibleDC(g_gameApp.GetScreenDC());

        const HBITMAP hbm = DDraw::SurfaceToGDIBitmap(g_ddrawDevice.pDDrawPrimary);

        if (hbm != 0)
        {
            SelectObject(hdcMem, hbm);

            BitBlt(g_gameApp.m_hScreenDC, 0, 0, g_displayState.displayMode.nWidth, g_displayState.displayMode.nHeight, hdcMem, 0, 0, SRCCOPY);

            DeleteObject(hbm);
            DeleteDC(hdcMem);

            return 1;
        }
    }

    return 0;
}

// Room for the three BI_BITFIELDS masks, of which BITMAPINFO declares one.
struct DibInfo565
{
    BITMAPINFOHEADER bmiHeader;
    DWORD            adwMasks[3];
};

// The size is the screen globals', so a larger surface is read short.
// FUNCTION: HYPERX 0x0043d6f0
HBITMAP DDraw::SurfaceToGDIBitmap(struct IDirectDrawSurface* pSurface)
{
    DibInfo565    bmi;
    BITMAP        bitmap;
    DDSURFACEDESC desc;
    char*         pBits;

    if (pSurface != 0)
    {
        memset(&bmi, 0, sizeof(BITMAPINFO));

        bmi.bmiHeader.biWidth       = g_displayState.displayMode.nWidth;
        bmi.bmiHeader.biHeight      = -g_displayState.displayMode.nHeight;
        bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 16;
        bmi.bmiHeader.biCompression = BI_BITFIELDS;

        bmi.adwMasks[0] = 0xf800;
        bmi.adwMasks[1] = 0x7e0;
        bmi.adwMasks[2] = 0x1f;

        HDC const hdc = g_gameApp.m_hScreenDC;

        const HBITMAP hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&pBits, 0, 0);

        if (hBitmap != 0)
        {
            desc.dwSize = sizeof(desc);

            g_hrLastResult = pSurface->Lock(0, &desc, DDLOCK_WAIT | DDLOCK_READONLY, 0);

            if (g_hrLastResult == 0)
            {
                GetObjectA(hBitmap, sizeof(bitmap), &bitmap);

                const unsigned int nRowBytes = (bitmap.bmBitsPixel >> 3) * bitmap.bmWidth;
                char*              pSource   = (char*)desc.lpSurface;

                for (short nRow = 0; nRow < g_displayState.displayMode.nHeight; nRow++)
                {
                    memcpy(pBits, pSource, nRowBytes);

                    pBits += bitmap.bmWidthBytes;
                    pSource += desc.lPitch;
                }

                pSurface->Unlock(0);

                return hBitmap;
            }
        }
    }

    return 0;
}

unsigned int g_dwDisplayWidth  = 640;
unsigned int g_dwDisplayHeight = 480;
unsigned int g_dwDisplayBpp    = 8;

// The colour mode every HUD picture load is handed.
int g_nHudColourMode = 1;
