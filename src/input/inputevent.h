// The input events the menus and the match read. The values are the Win32 messages and MK_ flags
// they began as, which the screens' handlers still compare and pass along.

#ifndef HYPERBLADE_INPUTEVENT_H
#define HYPERBLADE_INPUTEVENT_H

constexpr auto kInputKeyDown   = 0x100u;
constexpr auto kInputKeyUp     = 0x101u;
constexpr auto kInputMouseMove = 0x200u;
constexpr auto kInputLeftDown  = 0x201u;
constexpr auto kInputLeftUp    = 0x202u;
constexpr auto kInputRightDown = 0x204u;
constexpr auto kInputRightUp   = 0x205u;

// The buttons and modifiers held during a mouse event. A match reads the word as a key code, so
// kMouseLeft lands on the left button's.
constexpr auto kMouseLeft    = 0x01u;
constexpr auto kMouseRight   = 0x02u;
constexpr auto kMouseShift   = 0x04u;
constexpr auto kMouseControl = 0x08u;
constexpr auto kMouseMiddle  = 0x10u;
constexpr auto kMouseX1      = 0x20u;
constexpr auto kMouseX2      = 0x40u;

#endif
