#ifndef HYPERBLADE_D3DDEVICE_H
#define HYPERBLADE_D3DDEVICE_H

// The driver and mode pickers' answers: draw in the window, pick one, or none fits.
#define MODE_USE_WINDOW (-24)
#define MODE_ANY        (-25)
#define DRIVER_ANY      (-25)
#define SELECT_NONE     (-100)

struct IDirectDrawSurface;

// The device setup and scene bracketing; the device itself is g_d3dState.pDevice.
namespace D3DDevice
{

int __stdcall EndDeviceScene(void);
int __stdcall BeginScene(void);
int __stdcall EndScene(void);
int __cdecl   SelectDriverAndMode(int* pnDriver, int* pnMode);
int __cdecl   Init(IDirectDrawSurface* pSurface);

} // namespace D3DDevice

#include "graphics/ddrawapi.h"

#endif
