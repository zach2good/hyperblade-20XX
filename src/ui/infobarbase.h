#ifndef HYPERBLADE_INFOBARBASE_H
#define HYPERBLADE_INFOBARBASE_H

#include "common/compat.h"
#include "graphics/renderer.h"

// The renderer interface: a destructor, one no-op and twenty three pure slots. A root class, so
// VC6 inlines the destructor into the deleter.
// SLOT TABLE 0x004b3068
// VTABLE: HYPERX 0x004b3068
class InfoBarBase
{
public:
    InfoBarBase()
    : m_bQuitPromptVisible(0)
    {
    }

    // FUNCTION: HYPERX100 0x004bd820
    virtual ~InfoBarBase()
    {
    }

    // Slot one is Object::Trace in the original, a shared free function.
    // FUNCTION: HYPERX100 0x004bd830
    virtual void Unused()
    {
    }

    virtual void LayoutSurfaces()                                         = 0;
    virtual void DrawScene(int nFlags)                                    = 0;
    virtual void DrawHUD(int nFlags, HDC hDC)                             = 0;
    virtual void SetMessageText(const char* pszText, unsigned long nCode) = 0;
    virtual void ShowMessage()                                            = 0;

    virtual void HideMessage() = 0;

    virtual void SetHomeLogo(char* pTeamData) = 0;
    virtual void SetAwayLogo(char* pTeamData) = 0;
    // Nought when the renderer has no head cam.
    virtual class Texture* GetHeadCamSurface()                 = 0;
    virtual void           ShowHeadCam()                       = 0;
    virtual void           SetHeadCamText(const char* pszText) = 0;
    virtual ViewExtent     GetHeadCamSize()                    = 0;
    virtual ViewExtent     GetHeadCamOrigin()                  = 0;

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

    // src/app/gameapp.cpp reads the flag to route Enter and Esc.
    int          m_bQuitPromptVisible; // 0x004
    unsigned int m_nQuitPromptTick;    // 0x008

    static InfoBarBase* __cdecl Create(int nKind, HDC hDC);
};

// SYNTHETIC: HYPERX 0x0043e0d0
// InfoBarBase::`scalar deleting destructor'

#endif
