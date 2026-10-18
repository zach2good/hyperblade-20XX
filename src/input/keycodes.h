// The key codes the game and its saved bindings use, which are Windows' virtual-key codes. Letters
// and digits are their capital ASCII characters.

#ifndef HYPERBLADE_KEYCODES_H
#define HYPERBLADE_KEYCODES_H

namespace Key
{

constexpr auto kLButton   = 0x01u;
constexpr auto kRButton   = 0x02u;
constexpr auto kBack      = 0x08u;
constexpr auto kTab       = 0x09u;
constexpr auto kReturn    = 0x0du;
constexpr auto kShift     = 0x10u;
constexpr auto kControl   = 0x11u;
constexpr auto kPause     = 0x13u;
constexpr auto kCapital   = 0x14u;
constexpr auto kEscape    = 0x1bu;
constexpr auto kSpace     = 0x20u;
constexpr auto kPrior     = 0x21u;
constexpr auto kNext      = 0x22u;
constexpr auto kEnd       = 0x23u;
constexpr auto kHome      = 0x24u;
constexpr auto kLeft      = 0x25u;
constexpr auto kUp        = 0x26u;
constexpr auto kRight     = 0x27u;
constexpr auto kDown      = 0x28u;
constexpr auto kSnapshot  = 0x2cu;
constexpr auto kInsert    = 0x2du;
constexpr auto kDel       = 0x2eu;
constexpr auto kLWin      = 0x5bu;
constexpr auto kRWin      = 0x5cu;
constexpr auto kApps      = 0x5du;
constexpr auto kNumpad0   = 0x60u;
constexpr auto kNumpad1   = 0x61u;
constexpr auto kNumpad2   = 0x62u;
constexpr auto kNumpad3   = 0x63u;
constexpr auto kNumpad4   = 0x64u;
constexpr auto kNumpad5   = 0x65u;
constexpr auto kNumpad6   = 0x66u;
constexpr auto kNumpad7   = 0x67u;
constexpr auto kNumpad8   = 0x68u;
constexpr auto kNumpad9   = 0x69u;
constexpr auto kMultiply  = 0x6au;
constexpr auto kAdd       = 0x6bu;
constexpr auto kSubtract  = 0x6du;
constexpr auto kDecimal   = 0x6eu;
constexpr auto kDivide    = 0x6fu;
constexpr auto kF1        = 0x70u;
constexpr auto kF2        = 0x71u;
constexpr auto kF3        = 0x72u;
constexpr auto kF4        = 0x73u;
constexpr auto kF5        = 0x74u;
constexpr auto kF6        = 0x75u;
constexpr auto kF7        = 0x76u;
constexpr auto kF8        = 0x77u;
constexpr auto kF9        = 0x78u;
constexpr auto kF10       = 0x79u;
constexpr auto kF11       = 0x7au;
constexpr auto kF12       = 0x7bu;
constexpr auto kNumLock   = 0x90u;
constexpr auto kScroll    = 0x91u;
constexpr auto kOEM1      = 0xbau;
constexpr auto kOEMPlus   = 0xbbu;
constexpr auto kOEMComma  = 0xbcu;
constexpr auto kOEMMinus  = 0xbdu;
constexpr auto kOEMPeriod = 0xbeu;
constexpr auto kOEM2      = 0xbfu;
constexpr auto kOEM3      = 0xc0u;
constexpr auto kOEM4      = 0xdbu;
constexpr auto kOEM5      = 0xdcu;
constexpr auto kOEM6      = 0xddu;
constexpr auto kOEM7      = 0xdeu;
constexpr auto kOEM102    = 0xe2u;

} // namespace Key

#endif
