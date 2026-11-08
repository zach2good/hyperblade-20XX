// The static members are free functions in the original; the slots are named from their call sites.

#ifndef HYPERBLADE_RENDERER_H
#define HYPERBLADE_RENDERER_H

#include "common/compat.h"

class Texture;

#include "texture.h"
#include "graphics/viewextent.h"
#include "graphics/renderoptions.h"

// The grey a frame with nothing behind it is cleared to.
#define CLEAR_COLOUR 0x505050

struct ViewExtent;

struct LayoutState;

class Renderer
{
public:
    static void __stdcall        DrawSceneDispatch(void);
    static int __cdecl           SetDromeTextured(int nEnable);
    static int __cdecl           ToggleDromeTextured(void);
    static void                  SetRenderOption(RenderOption nOption, int nValue);
    static void                  SetLoadingMode(void);
    static int __stdcall         PresentFrame(void);
    static int __stdcall         BeginScene(void);
    static int __stdcall         EndScene(void);
    static int __cdecl           RenderFrame(const Texture* pTarget);
    static void                  ClearFrame(void);
    static void                  DrawAllMeshes(void);
    static void __stdcall        DrawWholeScene(void);
    static RenderOptions __cdecl GetRenderOptions();
    static void                  SetCameraTransformAndViewport(LayoutState block);
    static void __stdcall        DestroyBackBuffer(void);

    static void __stdcall InitializeLightDirections(void);
};

#endif
