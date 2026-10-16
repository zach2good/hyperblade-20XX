// The static members are free functions in the original; the slots are named from their call sites.

#ifndef HYPERBLADE_RENDERER_H
#define HYPERBLADE_RENDERER_H

#include "common/compat.h"

#include "graphics/viewextent.h"
#include "graphics/renderoptions.h"

// The grey a frame with nothing behind it is cleared to.
#define CLEAR_COLOUR 0x505050

struct ViewExtent;

struct LayoutState;

class __declspec(novtable) Renderer
{
public:
    static void __stdcall DrawSceneDispatch(void);
    static int __cdecl    SetDromeTextured(int nEnable);
    static int __cdecl    ToggleDromeTextured(void);
    static void           SetRenderOption(RenderOption nOption, int nValue);
    static void           SetLoadingMode(void);
    static int __stdcall  PresentFrame(void);

    static void                  ClearFrame(void);
    static void                  DrawAllMeshes(void);
    static void __stdcall        DrawWholeScene(void);
    static RenderOptions __cdecl GetRenderOptions();
    static void                  SetCameraTransformAndViewport(LayoutState block);
    static void __stdcall        DestroyBackBuffer(void);

    virtual ~Renderer()
    {
    }

    // Filled with the shared empty body at 0x0043e130; nothing calls it.
    virtual void Unused()                                                 = 0;
    virtual void LayoutSurfaces()                                         = 0;
    virtual void DrawScene(int nFlags)                                    = 0;
    virtual void DrawHUD(int nFlags, HDC hDC)                             = 0;
    virtual void SetMessageText(const char* pszText, unsigned long nCode) = 0;
    virtual void ShowMessage()                                            = 0;

    virtual void HideMessage() = 0;

    virtual void       SetHomeLogo(char* pTeamData)        = 0;
    virtual void       SetAwayLogo(char* pTeamData)        = 0;
    virtual void       ShowHeadCam()                       = 0;
    virtual void       SetHeadCamText(const char* pszText) = 0;
    virtual ViewExtent GetHeadCamSize()                    = 0;
    virtual ViewExtent GetHeadCamOrigin()                  = 0;

    // A camera going to render mode one hands it zero.
    virtual void SetShotPower(float flPower) = 0;
    virtual void ShowShotPower()             = 0;

    virtual void       SetReticlePos(int x, int y) = 0;
    virtual ViewExtent GetReticleSize()            = 0;

    // The modes are Reticle_State; nought hides the reticle.
    virtual void SetReticleState(int nMode) = 0;

    // CDisplayPassBar is the only caller.
    virtual ViewExtent GetPassBarSize()           = 0;
    virtual void       SetPassBarMarker(int nPos) = 0;
    virtual void       ShowPassBar()              = 0;
    virtual void       ShowQuitPrompt()           = 0;
    virtual void       HideQuitPrompt()           = 0;

    // Before each simulation tick: drops what the last tick asked to show, so every frame drawn
    // between two ticks shows the same.
    virtual void BeginTick() = 0;

    static void __stdcall InitializeLightDirections(void);
};

#endif
