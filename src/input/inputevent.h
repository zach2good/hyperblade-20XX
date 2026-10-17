// The input events the menus and the match read. The values are the Win32 messages and MK_ flags
// they began as, which the screens' handlers still compare and pass along.

#ifndef HYPERBLADE_INPUTEVENT_H
#define HYPERBLADE_INPUTEVENT_H

constexpr unsigned int INPUT_KEY_DOWN   = 0x100;
constexpr unsigned int INPUT_KEY_UP     = 0x101;
constexpr unsigned int INPUT_MOUSE_MOVE = 0x200;
constexpr unsigned int INPUT_LEFT_DOWN  = 0x201;
constexpr unsigned int INPUT_LEFT_UP    = 0x202;
constexpr unsigned int INPUT_RIGHT_DOWN = 0x204;
constexpr unsigned int INPUT_RIGHT_UP   = 0x205;

// The buttons and modifiers held during a mouse event. A match reads the word as a key code, so
// MOUSE_LEFT lands on the left button's.
constexpr unsigned int MOUSE_LEFT    = 0x01;
constexpr unsigned int MOUSE_RIGHT   = 0x02;
constexpr unsigned int MOUSE_SHIFT   = 0x04;
constexpr unsigned int MOUSE_CONTROL = 0x08;
constexpr unsigned int MOUSE_MIDDLE  = 0x10;
constexpr unsigned int MOUSE_X1      = 0x20;
constexpr unsigned int MOUSE_X2      = 0x40;

#endif
