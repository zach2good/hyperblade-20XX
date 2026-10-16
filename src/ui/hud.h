// The base's constructor is inlined; its destructor exists for the unwind funclet.

#ifndef HYPERBLADE_HUD_H
#define HYPERBLADE_HUD_H

#include "infobar.h"
#include "graphics/renderer.h"
#include "uiglobals.h"
#include "ui/infobarbase.h"
#include "graphics/viewextent.h"

class CDDTextSurface;
class String;

// Table 0x004b3004 is the renderer interface's; the destructor restores InfoBarBase's 0x004b3068.
// VTABLE: HYPERX 0x004b3004
class HUD : public InfoBarBase
{
public:
    // Free functions in the original.
    static int __cdecl    CreateSurfaces(void);
    static int __stdcall  CreateFPSText(void);
    static int __cdecl    CreateSpeedMeter(void);
    static int __cdecl    CreateDamageAndRadar(void);
    static void __stdcall PlaceSurfaces(void);
    static void __stdcall UpdateOwnScoreboard(void);
    static void __stdcall UpdateFoeScoreboard(void);

    explicit HUD(int nUnused);
    ~HUD();

    // Each forwards to the body the table names, except the four whose bodies are this class's own.
    virtual void Unused()
    {
    }

    virtual void     LayoutSurfaces();                                         // 0x0043e140
    virtual void     DrawScene(int bDraw);                                     // 0x0043e150
    virtual void     DrawHUD(int bDisplayHUDArt, HDC hUnused);                 // 0x0043e0f0
    virtual void     SetMessageText(const char* pszText, unsigned long nCode); // 0x0043ee50
    virtual void     ShowMessage();                                            // 0x0043eeb0
    virtual void     HideMessage();                                            // 0x0043eec0
    virtual void     SetHomeLogo(char* pTeamData);                             // 0x0043eed0
    virtual void     SetAwayLogo(char* pTeamData);                             // 0x0043eef0
    virtual Texture* GetHeadCamSurface();                                      // 0x0043ef10
    virtual void     ShowHeadCam();                                            // 0x0043ef20

    // The four that answer with a point are stand-ins declared as ViewExtent.
    virtual void       SetHeadCamText(const char* pszText);
    virtual ViewExtent GetHeadCamSize();
    virtual ViewExtent GetHeadCamOrigin();
    virtual void       SetShotPower(float flPower);
    virtual void       ShowShotPower();
    virtual void       SetReticlePos(int x, int y);
    virtual ViewExtent GetReticleSize();
    virtual void       SetReticleState(int nMode);
    virtual ViewExtent GetPassBarSize();
    virtual void       SetPassBarMarker(int nPos);
    virtual void       ShowPassBar();
    virtual void       ShowQuitPrompt();
    virtual void       HideQuitPrompt();
    void               BeginTick() override;

public:
    int m_bMessageVisible; // 0x0c
    // One-frame requests: the draw pass hides a widget whose flag is still nought, then clears it.
    int m_bShotPowerWanted; // 0x10, g_hudSurfaces.pShotPowerMeter
    int m_bHeadCamWanted;   // 0x14, g_hudSurfaces.pHeadCamFrame
    int m_bPassBarWanted;   // 0x18, g_hudSurfaces.pPassBar

private:
    // Inlined at every call in the original.
    static void SetLabelText(CDDTextSurface* pLabel, const char* pszText);
    static void AssignName(String* pName, const char* psz);
    static void AssignName(CDDTextSurface* pLabel, const char* psz);
    static int  RowColour(int nRow);
};

// SYNTHETIC: HYPERX 0x0043e100
// HUD::`scalar deleting destructor'

#endif
