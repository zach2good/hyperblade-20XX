#include "common/compat.h"
#include "d3ddevice.h"
#include "globals.h"
#ifdef HYPERX_BRINGUP
#define TEXTURE_FORMAT_MAX 10
#endif
#include "graphicsglobals.h"
#include "d3ddeviceinit.h"
#include "d3dshapes.h"
#include "app/gameapp.h"
#include "math/vector.h"
#include "renderstate.h"
#include <string.h>
#include <ddraw.h>
#include "graphics/d3dapi.h"
#include "graphics/ddrawapi.h"

// GLOBAL: HYPERX 0x004b4f10
extern const GUID g_iidDirect3D;

#define TEXFORMAT_NONE (-100)

struct IDirectDraw;

#define D3D_DRIVER_LIMIT 5

#define PREFERRED_WIDTH 640

#define PREFERRED_HEIGHT 480

#define PREFERRED_BPP 16

// FUNCTION: HYPERX 0x00431050
int __cdecl D3D::Initialize(IDirectDraw* pDDraw)
{
    if (g_bForceSoftwareRender != 0)
    {
        return 0;
    }

    g_hrD3DResult = pDDraw->QueryInterface(g_iidDirect3D, (void**)&g_d3dState.pDirect3D);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004bd544
        g_gameApp.Error("Creation of Direct3D interface failed.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x004311e0
int __cdecl D3D::BppToDDBD(int nBpp)
{
    switch (nBpp)
    {
        case 1:
            return DDBD_1;

        case 2:
            return DDBD_2;

        case 4:
            return DDBD_4;

        case 8:
            return DDBD_8;

        case 16:
            return DDBD_16;

        case 24:
            return DDBD_24;

        case 32:
            return DDBD_32;
    }

    return TEXFORMAT_NONE;
}

// FUNCTION: HYPERX 0x00431270
int D3D::EnumDrivers(void)
{
    g_d3dState.nD3DDriverCount = 0;

    g_hrD3DResult = g_d3dState.pDirect3D->EnumDevices(D3D::EnumDriversCallback, 0);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004bd570
        g_gameApp.Error("Enumeration of drivers failed.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    g_d3dState.nD3DDriverIndex = 0;

    return 1;
}

// The count is also the index, and the enumeration stops at five records.
// FUNCTION: HYPERX 0x004312d0
HRESULT __stdcall D3D::EnumDriversCallback(GUID* pGuid, char* pszDescription, char* pszName, D3DDEVICEDESC* pHardwareDesc, D3DDEVICEDESC* pEmulationDesc, void*)
{
    if (pHardwareDesc->dcmColorModel != 0 && g_d3dState.bOnlyEmulation != 0)
    {
        return D3DENUMRET_OK;
    }

    g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].guid = *pGuid;

    strcpy(g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].szDescription, pszDescription);
    strcpy(g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].szDeviceName, pszName);

    if (pHardwareDesc->dcmColorModel != 0)
    {
        g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].bIsHardware = 1;

        memcpy(&g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].deviceDesc, pHardwareDesc, sizeof(D3DDeviceDesc6));
    }
    else
    {
        g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].bIsHardware = 0;

        memcpy(&g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].deviceDesc, pEmulationDesc, sizeof(D3DDeviceDesc6));
    }

    g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].bDoesTextures =
        g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].deviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE;

    g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].bDoesZBuffer =
        g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].deviceDesc.dwDeviceZBufferBitDepth != 0;

    g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].bCanDoWindow =
        (D3D::BppToDDBD(g_d3dState.nDesktopBpp) & g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].deviceDesc.dwDeviceRenderBitDepth) != 0;

    if (g_d3dState.bIsPrimary == 0)
    {
        g_d3dState.aD3DDrivers[g_d3dState.nD3DDriverCount].bCanDoWindow = 0;
    }

    g_d3dState.nD3DDriverCount++;

    return g_d3dState.nD3DDriverCount != D3D_DRIVER_LIMIT;
}

// FUNCTION: HYPERX 0x00431a00
int __cdecl D3D::GetCurrentDisplayMode(struct IDirectDraw* pDDraw)
{
    DDSURFACEDESC desc;

    g_d3dState.pDDraw = pDDraw;

    memset(&desc, 0, sizeof(desc));

    desc.dwSize = sizeof(desc);

    g_hrD3DResult = g_d3dState.pDDraw->GetDisplayMode(&desc);

    if (g_hrD3DResult != 0)
    {
        // STRING: HYPERX 0x004bd64c
        g_gameApp.Error("Getting the current display mode failed.\n%s",
                        DDraw::ErrorToString(g_hrD3DResult));

        return 0;
    }

    g_d3dState.nDesktopWidth  = desc.dwWidth;
    g_d3dState.nDesktopHeight = desc.dwHeight;
    g_d3dState.nDesktopBpp    = desc.ddpfPixelFormat.dwRGBBitCount;

    return 1;
}

// Both return one; -100 in the out parameter means nothing was found.
// FUNCTION: HYPERX 0x00431a80
int __cdecl D3D::FindDeviceByFormat(int* pnDriver, unsigned int nFormatMask)
{
    int nBest;

    for (nBest = 0; nBest < g_d3dState.nD3DDriverCount; nBest++)
    {
        if ((g_d3dState.aD3DDrivers[nBest].deviceDesc.dwDeviceRenderBitDepth & nFormatMask) != 0)
        {
            break;
        }
    }

    if (nBest >= g_d3dState.nD3DDriverCount)
    {
        *pnDriver = SELECT_NONE;

        return 1;
    }

    for (int i = 0; i < g_d3dState.nD3DDriverCount; i++)
    {
        if ((g_d3dState.aD3DDrivers[i].deviceDesc.dwDeviceRenderBitDepth & nFormatMask) != 0)
        {
            if (g_d3dState.aD3DDrivers[i].bIsHardware != 0 && g_d3dState.aD3DDrivers[nBest].bIsHardware == 0)
            {
                nBest = i;
            }
            else if (g_d3dState.aD3DDrivers[i].bIsHardware == g_d3dState.aD3DDrivers[nBest].bIsHardware && (g_d3dState.aD3DDrivers[i].deviceDesc.dcmColorModel & D3DCOLOR_RGB) != 0 && (g_d3dState.aD3DDrivers[nBest].deviceDesc.dcmColorModel & D3DCOLOR_RGB) == 0)
            {
                nBest = i;
            }
        }
    }

    if (nBest >= g_d3dState.nD3DDriverCount)
    {
        *pnDriver = SELECT_NONE;

        return 1;
    }

    *pnDriver = nBest;

    return 1;
}

// Prefers 640x480 at 16 bits.
// FUNCTION: HYPERX 0x00431b50
int __cdecl D3D::FindDisplayMode(int* pnMode, unsigned int nDepthMask)
{
    int nMode;

    for (nMode = 0; nMode < g_displayState.nDisplayModeCount; nMode++)
    {
        if ((nDepthMask & D3D::BppToDDBD(g_displayState.aDisplayModes[nMode].nBpp)) != 0)
        {
            break;
        }
    }

    int nBest = nMode;

    while (nMode < g_displayState.nDisplayModeCount)
    {
        if ((nDepthMask & D3D::BppToDDBD(g_displayState.aDisplayModes[nMode].nBpp)) != 0 && g_displayState.aDisplayModes[nMode].nWidth == PREFERRED_WIDTH && g_displayState.aDisplayModes[nMode].nHeight == PREFERRED_HEIGHT && g_displayState.aDisplayModes[nMode].nBpp == PREFERRED_BPP)
        {
            nBest = nMode;
            break;
        }

        nMode++;
    }

    if (nBest >= g_displayState.nDisplayModeCount)
    {
        *pnMode = SELECT_NONE;

        return 1;
    }

    *pnMode = nBest;

    return 1;
}

// FUNCTION: HYPERX 0x00431c00
int __cdecl D3D::SelectDisplayMode(int* pnDriver, int* pnMode)
{
    *pnDriver = DRIVER_ANY;
    *pnMode   = MODE_ANY;

    if (D3DDevice::SelectDriverAndMode(pnDriver, pnMode) != 0)
    {
        g_d3dState.nD3DDriverIndex = *pnDriver;

        return 1;
    }

    return 0;
}

// The last 16-bit RGB format wins, and nothing resets the counter between enumerations.
// FUNCTION: HYPERX 0x00433700
HRESULT __stdcall D3D::EnumTextureFormatsCallback(DDSURFACEDESC* pFormat, void* pContext)
{
    int* pnChosen = (int*)pContext;

#ifdef HYPERX_BRINGUP
    // The original does not check: an overflow writes over the device pointer and the viewports.
    if (g_d3dState.nTextureFormatsSeen >= TEXTURE_FORMAT_MAX)
    {
        return DDENUMRET_CANCEL;
    }
#endif

    DDSURFACEDESC* pSlot = &g_d3dState.aTextureFormats[g_d3dState.nTextureFormatsSeen];

    *pSlot = *pFormat;

    if (*pnChosen == -1)
    {
        *pnChosen = g_d3dState.nTextureFormatsSeen;
    }

    if (g_d3dState.aTextureFormats[g_d3dState.nTextureFormatsSeen].ddpfPixelFormat.dwFlags == DDPF_RGB && g_d3dState.aTextureFormats[g_d3dState.nTextureFormatsSeen].ddpfPixelFormat.dwRGBBitCount == 16)
    {
        *pnChosen = g_d3dState.nTextureFormatsSeen;

        g_d3dState.nTextureFormatsSeen++;

        return DDENUMRET_OK;
    }

    g_d3dState.nTextureFormatsSeen++;

    return DDENUMRET_OK;
}

struct IDirectDraw;
