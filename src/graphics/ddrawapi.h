#ifndef HYPERBLADE_DDRAWAPI_H
#define HYPERBLADE_DDRAWAPI_H

#include "common/compat.h"
#include <ddraw.h>

struct DDrawCaps;

// The low bits of a DirectDraw error, which the failure messages print.
#define DDRAW_ERROR_CODE_MASK 0xfff

// The card's video memory in bytes, as DDCAPS gives it.
struct VideoMemory
{
    int nTotal;
    int nFree;
};

namespace DDraw
{

int __cdecl            FailInitialization(int nError);
char*                  ErrorToString(int hr);
int __cdecl            SetFullscreenMode(HWND hWnd, int nBpp);
HRESULT WINAPI         EnumDisplayModesCallback(DDSURFACEDESC* pDesc, void* pUnusedContext);
int __cdecl            CompareDisplayModes(const void* pLeft, const void* pRight);
int __cdecl            EnumDisplayModes(struct IDirectDraw* pDDraw);
unsigned int __cdecl   DetectFlipCapability(void);
unsigned int           BlitBackToWindow(HWND hWnd, int bMenu);
int                    GetCurrentDisplayMode(struct IDirectDraw* pDDraw);
BOOL CALLBACK          EnumDriversCallback(GUID* pGuid, char* pszDesc, char* pszName, void* pCtx);
void                   ClearDeviceState(void);
unsigned int __stdcall CreatePrimaryAndBackBuffer(void);
void __stdcall         DestroyAllSurfaces(void);
int                    GetLastError(void);
int __cdecl            ClearBackBuffer(unsigned int nColour);
int __stdcall          ClearAllBuffers(void);
LPDIRECTDRAWSURFACE    CreateOffscreenSurface(int nWidth, int nHeight, int bSystemMemory, int bColourKey);
unsigned int __cdecl   ColourToPixel(IDirectDrawSurface* pSurface, unsigned int nColour);
int __cdecl            GetSurfaceDesc(DDSURFACEDESC* pDesc, struct IDirectDrawSurface* pSurface);
void __stdcall         LockBackBuffer(void);
void __stdcall         PaintSurface(void);
int __stdcall          BlitPrimaryToScreen(void);
HBITMAP                SurfaceToGDIBitmap(struct IDirectDrawSurface* pSurface);
int __cdecl            GetCaps(struct DDrawCaps* pDriver, struct DDrawCaps* pHel);
int __stdcall          Flip(void);
int __stdcall          BltFrontFromBack(void);
VideoMemory*           GetVideoMemory(VideoMemory* pMemory, struct IDirectDraw* pDevice);
void __stdcall         UnlockBackBuffer(void);
HDC __stdcall          GetBackBufferDC(void);
void                   ReleaseBackBufferDC(HDC hdc);
int __stdcall          CheckDisplayMode(void);
int                    FlipToGDISurface(int bShowGDI);
int __stdcall          FlipOrBlt(void);
int __stdcall          RestoreAllSurfaces(void);
unsigned int           InitDeviceAndSurfaces(HPALETTE hPalette);
int __cdecl            ChangeDisplayMode(int nMode);

} // namespace DDraw

#endif
