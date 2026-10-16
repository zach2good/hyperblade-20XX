#ifndef HYPERBLADE_D3DDEVICEINIT_H
#define HYPERBLADE_D3DDEVICEINIT_H
#include "common/compat.h"

#include "graphicsglobals.h"
#include <stddef.h>
#include "graphics/d3dapi.h"
#include <d3d.h>

// The DirectX 6 device description, which carries fields past VC98's d3dcaps.h.
struct D3DDeviceDesc6 : D3DDEVICEDESC
{
    DWORD    dwMaxTextureRepeat;
    DWORD    dwMaxTextureAspectRatio;
    DWORD    dwMaxAnisotropy;
    D3DVALUE dvGuardBandLeft;
    D3DVALUE dvGuardBandTop;
    D3DVALUE dvGuardBandRight;
    D3DVALUE dvGuardBandBottom;
    D3DVALUE dvExtentsAdjust;
    DWORD    dwStencilCaps;
    DWORD    dwFVFCaps;
    DWORD    dwTextureOpCaps;
    WORD     wMaxTextureBlendStages;
    WORD     wMaxSimultaneousTextures;
};

static_assert(sizeof(D3DDeviceDesc6) == 0xfc);

// The DirectX 5 d3dapp sample's driver record, 0x3b4 bytes.
struct D3DDriverInfo
{
    char           szDeviceName[0x1e];  // 0x000, the enumeration's device name, copied whole
    char           szDescription[0x82]; // 0x01e, the enumeration's description, copied whole
    D3DDeviceDesc6 deviceDesc;          // 0x0a0
    // Two desc sizes that no instruction in the image reads or writes.
    D3DDeviceDesc6 aUnusedDesc[2]; // 0x19c
    GUID           guid;           // 0x394, the interface the QueryInterface asks for
    int            bIsHardware;    // 0x3a4
    int            bDoesTextures;  // 0x3a8
    int            bDoesZBuffer;   // 0x3ac
    int            bCanDoWindow;   // 0x3b0, the desktop depth is a render depth
};

static_assert(sizeof(D3DDriverInfo) == 0x3b4);
static_assert(offsetof(D3DDriverInfo, deviceDesc) == 0x0a0);
static_assert(offsetof(D3DDriverInfo, aUnusedDesc) == 0x19c);
static_assert(offsetof(D3DDriverInfo, guid) == 0x394);
static_assert(offsetof(D3DDriverInfo, bCanDoWindow) == 0x3b0);

struct DisplayModeEntry
{
    int nWidth;
    int nHeight;
    int nBpp;
    int nUnused;
};

// DDraw::ClearDeviceState clears all 0xd9 words with one rep stosd.
struct DisplayStateBlock
{
    int anUnusedA[5];
    // The 640x480 16-bit mode. Written and never read.
    int              nDefaultDisplayModeIndex;
    DisplayModeEntry aDisplayModes[50];
    DisplayModeEntry displayMode;
    int              nDisplayModeCount;
    int              nDisplayModeIndex;
    int              anUnusedB[5];
};

static_assert(offsetof(DisplayStateBlock, nDefaultDisplayModeIndex) == 0x14);
static_assert(offsetof(DisplayStateBlock, displayMode) == 0x338);
static_assert(offsetof(DisplayStateBlock, nDisplayModeIndex) == 0x34c);
static_assert(sizeof(DisplayStateBlock) == 0xd9 * 4);

// GLOBAL: HYPERX 0x004cd6a8
extern DisplayStateBlock g_displayState;

#endif
