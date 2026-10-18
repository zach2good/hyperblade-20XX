// The key codes the game and its saved bindings use, which are Windows' virtual-key codes. Letters
// and digits are their capital ASCII characters.

#ifndef HYPERBLADE_KEYCODES_H
#define HYPERBLADE_KEYCODES_H

namespace Key
{

constexpr unsigned int kLbutton   = 0x01;
constexpr unsigned int kRbutton   = 0x02;
constexpr unsigned int kBack      = 0x08;
constexpr unsigned int kTab       = 0x09;
constexpr unsigned int kReturn    = 0x0d;
constexpr unsigned int kShift     = 0x10;
constexpr unsigned int kControl   = 0x11;
constexpr unsigned int kPause     = 0x13;
constexpr unsigned int kCapital   = 0x14;
constexpr unsigned int kEscape    = 0x1b;
constexpr unsigned int kSpace     = 0x20;
constexpr unsigned int kPrior     = 0x21;
constexpr unsigned int kNext      = 0x22;
constexpr unsigned int kEnd       = 0x23;
constexpr unsigned int kHome      = 0x24;
constexpr unsigned int kLeft      = 0x25;
constexpr unsigned int kUp        = 0x26;
constexpr unsigned int kRight     = 0x27;
constexpr unsigned int kDown      = 0x28;
constexpr unsigned int kSnapshot  = 0x2c;
constexpr unsigned int kInsert    = 0x2d;
constexpr unsigned int kDel       = 0x2e;
constexpr unsigned int kLwin      = 0x5b;
constexpr unsigned int kRwin      = 0x5c;
constexpr unsigned int kApps      = 0x5d;
constexpr unsigned int kNumpad0   = 0x60;
constexpr unsigned int kNumpad1   = 0x61;
constexpr unsigned int kNumpad2   = 0x62;
constexpr unsigned int kNumpad3   = 0x63;
constexpr unsigned int kNumpad4   = 0x64;
constexpr unsigned int kNumpad5   = 0x65;
constexpr unsigned int kNumpad6   = 0x66;
constexpr unsigned int kNumpad7   = 0x67;
constexpr unsigned int kNumpad8   = 0x68;
constexpr unsigned int kNumpad9   = 0x69;
constexpr unsigned int kMultiply  = 0x6a;
constexpr unsigned int kAdd       = 0x6b;
constexpr unsigned int kSubtract  = 0x6d;
constexpr unsigned int kDecimal   = 0x6e;
constexpr unsigned int kDivide    = 0x6f;
constexpr unsigned int kF1        = 0x70;
constexpr unsigned int kF2        = 0x71;
constexpr unsigned int kF3        = 0x72;
constexpr unsigned int kF4        = 0x73;
constexpr unsigned int kF5        = 0x74;
constexpr unsigned int kF6        = 0x75;
constexpr unsigned int kF7        = 0x76;
constexpr unsigned int kF8        = 0x77;
constexpr unsigned int kF9        = 0x78;
constexpr unsigned int kF10       = 0x79;
constexpr unsigned int kF11       = 0x7a;
constexpr unsigned int kF12       = 0x7b;
constexpr unsigned int kNumlock   = 0x90;
constexpr unsigned int kScroll    = 0x91;
constexpr unsigned int kOEM1      = 0xba;
constexpr unsigned int kOEMPlus   = 0xbb;
constexpr unsigned int kOEMComma  = 0xbc;
constexpr unsigned int kOEMMinus  = 0xbd;
constexpr unsigned int kOEMPeriod = 0xbe;
constexpr unsigned int kOEM2      = 0xbf;
constexpr unsigned int kOEM3      = 0xc0;
constexpr unsigned int kOEM4      = 0xdb;
constexpr unsigned int kOEM5      = 0xdc;
constexpr unsigned int kOEM6      = 0xdd;
constexpr unsigned int kOEM7      = 0xde;
constexpr unsigned int kOEM102    = 0xe2;

} // namespace Key

#endif
