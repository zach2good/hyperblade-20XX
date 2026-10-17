// The key codes the game and its saved bindings use, which are Windows' virtual-key codes. Letters
// and digits are their capital ASCII characters.

#ifndef HYPERBLADE_KEYCODES_H
#define HYPERBLADE_KEYCODES_H

namespace Key
{

constexpr unsigned int LBUTTON    = 0x01;
constexpr unsigned int RBUTTON    = 0x02;
constexpr unsigned int BACK       = 0x08;
constexpr unsigned int TAB        = 0x09;
constexpr unsigned int RETURN     = 0x0d;
constexpr unsigned int SHIFT      = 0x10;
constexpr unsigned int CONTROL    = 0x11;
constexpr unsigned int PAUSE      = 0x13;
constexpr unsigned int CAPITAL    = 0x14;
constexpr unsigned int ESCAPE     = 0x1b;
constexpr unsigned int SPACE      = 0x20;
constexpr unsigned int PRIOR      = 0x21;
constexpr unsigned int NEXT       = 0x22;
constexpr unsigned int END        = 0x23;
constexpr unsigned int HOME       = 0x24;
constexpr unsigned int LEFT       = 0x25;
constexpr unsigned int UP         = 0x26;
constexpr unsigned int RIGHT      = 0x27;
constexpr unsigned int DOWN       = 0x28;
constexpr unsigned int SNAPSHOT   = 0x2c;
constexpr unsigned int INSERT     = 0x2d;
constexpr unsigned int DEL        = 0x2e;
constexpr unsigned int LWIN       = 0x5b;
constexpr unsigned int RWIN       = 0x5c;
constexpr unsigned int APPS       = 0x5d;
constexpr unsigned int NUMPAD0    = 0x60;
constexpr unsigned int NUMPAD1    = 0x61;
constexpr unsigned int NUMPAD2    = 0x62;
constexpr unsigned int NUMPAD3    = 0x63;
constexpr unsigned int NUMPAD4    = 0x64;
constexpr unsigned int NUMPAD5    = 0x65;
constexpr unsigned int NUMPAD6    = 0x66;
constexpr unsigned int NUMPAD7    = 0x67;
constexpr unsigned int NUMPAD8    = 0x68;
constexpr unsigned int NUMPAD9    = 0x69;
constexpr unsigned int MULTIPLY   = 0x6a;
constexpr unsigned int ADD        = 0x6b;
constexpr unsigned int SUBTRACT   = 0x6d;
constexpr unsigned int DECIMAL    = 0x6e;
constexpr unsigned int DIVIDE     = 0x6f;
constexpr unsigned int F1         = 0x70;
constexpr unsigned int F2         = 0x71;
constexpr unsigned int F3         = 0x72;
constexpr unsigned int F4         = 0x73;
constexpr unsigned int F5         = 0x74;
constexpr unsigned int F6         = 0x75;
constexpr unsigned int F7         = 0x76;
constexpr unsigned int F8         = 0x77;
constexpr unsigned int F9         = 0x78;
constexpr unsigned int F10        = 0x79;
constexpr unsigned int F11        = 0x7a;
constexpr unsigned int F12        = 0x7b;
constexpr unsigned int NUMLOCK    = 0x90;
constexpr unsigned int SCROLL     = 0x91;
constexpr unsigned int OEM_1      = 0xba;
constexpr unsigned int OEM_PLUS   = 0xbb;
constexpr unsigned int OEM_COMMA  = 0xbc;
constexpr unsigned int OEM_MINUS  = 0xbd;
constexpr unsigned int OEM_PERIOD = 0xbe;
constexpr unsigned int OEM_2      = 0xbf;
constexpr unsigned int OEM_3      = 0xc0;
constexpr unsigned int OEM_4      = 0xdb;
constexpr unsigned int OEM_5      = 0xdc;
constexpr unsigned int OEM_6      = 0xdd;
constexpr unsigned int OEM_7      = 0xde;
constexpr unsigned int OEM_102    = 0xe2;

} // namespace Key

#endif
