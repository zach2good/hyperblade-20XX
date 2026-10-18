// The input events the menus and the match read. The values are the Win32 messages and MK_ flags
// they began as, which the screens' handlers still compare and pass along.

#ifndef HYPERBLADE_INPUTEVENT_H
#define HYPERBLADE_INPUTEVENT_H

constexpr unsigned int kInputKeyDown   = 0x100;
constexpr unsigned int kInputKeyUp     = 0x101;
constexpr unsigned int kInputMouseMove = 0x200;
constexpr unsigned int kInputLeftDown  = 0x201;
constexpr unsigned int kInputLeftUp    = 0x202;
constexpr unsigned int kInputRightDown = 0x204;
constexpr unsigned int kInputRightUp   = 0x205;

// The buttons and modifiers held during a mouse event. A match reads the word as a key code, so
// kMouseLeft lands on the left button's.
constexpr unsigned int kMouseLeft    = 0x01;
constexpr unsigned int kMouseRight   = 0x02;
constexpr unsigned int kMouseShift   = 0x04;
constexpr unsigned int kMouseControl = 0x08;
constexpr unsigned int kMouseMiddle  = 0x10;
constexpr unsigned int kMouseX1      = 0x20;
constexpr unsigned int kMouseX2      = 0x40;

#endif
