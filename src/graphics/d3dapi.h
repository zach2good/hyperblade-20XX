#ifndef HYPERBLADE_D3DAPI_H
#define HYPERBLADE_D3DAPI_H

#include "common/compat.h"
#include <ddraw.h>

struct IDirectDraw;
struct _D3DDeviceDesc;

namespace D3D
{

int __cdecl       Initialize(IDirectDraw* pDDraw);
HRESULT __stdcall EnumDriversCallback(GUID* pGuid, char* pszDescription, char* pszName, _D3DDeviceDesc* pHardwareDesc, _D3DDeviceDesc* pEmulationDesc, void* pContext);
int __cdecl       BppToDDBD(int nBpp);
int __cdecl       FindDeviceByFormat(int* pnDriver, unsigned int nFormatMask);
int __cdecl       FindDisplayMode(int* pnMode, unsigned int nDepthMask);
int __cdecl       GetCurrentDisplayMode(struct IDirectDraw* pDDraw);
int               EnumDrivers(void);
int __cdecl       SelectDisplayMode(int* pnDriver, int* pnMode);
HRESULT __stdcall EnumTextureFormatsCallback(DDSURFACEDESC* pFormat, void* pContext);

} // namespace D3D

#endif
