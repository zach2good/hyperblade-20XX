#ifndef HYPERBLADE_WINDOW_H
#define HYPERBLADE_WINDOW_H

#include "common/compat.h"
#include "common/handle.h"
#include "math/point.h"
#include <cstdint>

namespace Window
{

void InvalidateAndUpdate(void);
void ApplyFullscreenStyle();

// The menu canvas's pixels, 0x00RRGGBB, top row first.
struct MenuCanvas
{
    std::uint32_t* pBits;
    int            nWidth;
    int            nHeight;
};

MenuCanvas MenuPixels();
void       PresentMenu();
// A client point on the 640x480 menu canvas.
Point ToMenuPoint(int nX, int nY);
void  OnSizeMenus();
void  OnSize(int cx, int cy);

// SDL owns the window and its event loop. Window events reach the game as they happen.
bool Create(const char* pszTitle);

// The window as the library that owns it sees it, for the renderer.
using NativeWindow = Handle<struct NativeWindowTag>;

NativeWindow Native();
void         Show();
void         Destroy();

// SDL draws the cursor over the window from its own state, so the game sets it only here.
void SetBusyCursor(bool bBusy);

// The engine ignores the window's size while it loads, so it is handed over once loading is done.
void ApplyCurrentSize();

enum class MessageButtons
{
    OK,
    OK_CANCEL,
    YES_NO,
};

enum class MessageIcon
{
    INFO,
    WARNING,
    STOP,
};

enum class MessageAnswer
{
    OK,
    CANCEL,
    YES,
    NO,
};

// Escape picks the last button.
MessageAnswer ShowMessage(const char* pszText, const char* pszCaption, MessageButtons buttons, MessageIcon icon);

bool AskYesNo(const char* pszText, const char* pszCaption);

// Handles what has arrived; false once the game has asked to quit.
bool Pump();
void WaitWhileInactive();

} // namespace Window

#endif
