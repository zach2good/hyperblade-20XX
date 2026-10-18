#ifndef HYPERBLADE_SCREEN_H
#define HYPERBLADE_SCREEN_H

#include "common/compat.h"
#include "graphics/gdi.h"
#include "globals.h"
#include "common/gametime.h"

class Widget;

// Not widget slots: Screen sends these ids after every mouse button message and after Return.
constexpr auto kWidgetIDMouse  = 100;
constexpr auto kWidgetIDReturn = 101;

// The three virtuals are the whole of 0x004b4040; two are folded across a dozen tables.
// VTABLE: HYPERX 0x004b4040
class Screen
{
public:
    // Free functions in the original.
    // Callers pass the palette or nothing; the body reads neither.
    static void ClearBlack(...);

public:
    void InitAllWidgets();
    void InitFrame();

    void SetWidgetDepressedByType(int nType, int nValue);

    virtual int OnKeyMessage(unsigned int wParam, unsigned int lParam);
    virtual ~Screen();
    // The original's slot two is the pure-call handler itself.
    virtual void OnWidgetMessage(int nMsg, unsigned int nId) = 0;

    Screen();

    Widget* AppendWidget(Widget* pWidget);
    int     HasPalette() const;

    void ShowMatchInfo();
    int  OnMouseMessage(unsigned int nMsg, unsigned int nButtons, int nX, int nY);
    int  PaintToWindow();
    int  TickAnimations();

    void SetRepaintBackground(int nValue);
    int  HandleKeyMessage(int nMsg, unsigned int wParam, unsigned int lParam);
    int  AddBitmap(const char* pszName);
    int  RemoveBitmap(const char* pszName);
    void InitBitmapCache();
    void FlushBitmapCache();
    void ResetAllHighlights(int nValue);
    void ResetAllDepressedStates(int nValue);
    int  IsButtonWidget(int nSlot);

    void         MoveFocusPrev();
    void         MoveFocusNext();
    int          FindHighlightedWidget(int* pnIndex);
    int          FindFocusedWidget();
    GDI::Bitmap* CacheLookup(const char* pszName, int* pbCached);

protected:
    // A frame the screen owns is compared against m_hBackground so it is not deleted twice.
    friend class Widget;

    GDI::Bitmap* m_hBackground; // 0x004
    int          m_bPaletted;   // 0x008
    char*        m_pszCaption;  // 0x00c
    int          m_nCaptionX;   // 0x010
    int          m_nCaptionY;   // 0x014
    // In the order AppendWidget added them.
    Widget* m_apWidgets[256];     // 0x018
    int     m_nLastWidget;        // 0x418, the highest slot in use, not a count
    int     m_bRepaintBackground; // 0x41c, set to one by whatever last refreshed the screen
    int     m_bFramePaintPending; // 0x420, a request to repaint, cleared on the way through
    int     m_nCaptured;          // 0x424, the slot the mouse went down on, or -1
    // Seeded to one, so the first release after a screen opens belongs to no one.
    int             m_bPointerOnWidget; // 0x428
    GDI::BitmapSize m_bitmap;           // 0x42c
    GDI::Bitmap*    m_hBitmap;          // 0x444

    // The tick record at 0x458 is twelve bytes, so the run ends at UIScreen's first field, 0x464.
    int m_nUnusedA; // 0x448
    int m_nUnusedB; // 0x44c
    // Not m_nLastWidget, the highest slot in use.
    int m_nWidgetCount; // 0x450
    int m_nUnusedC;     // 0x454
    // When the animation tick last ran. TickAnimations refuses to run again inside 50ms.
    SystemTimePair m_lastTick; // 0x458
};

// SYNTHETIC: HYPERX 0x0048bfc0
// Screen::`scalar deleting destructor'

#endif
