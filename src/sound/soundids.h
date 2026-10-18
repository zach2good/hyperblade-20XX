#ifndef HYPERBLADE_SOUNDIDS_H
#define HYPERBLADE_SOUNDIDS_H

// Sound ids: positions in SNDFILES.LST, counting only the sound lines, named for their files.
constexpr auto SOUND_SXHHITE2 = 0;
constexpr auto SOUND_SXXAMBA2 = 0x0e;
constexpr auto SOUND_SXXAMBB2 = 0x0f;
constexpr auto SOUND_SXVGRMA3 = 0x18;
constexpr auto SOUND_SASRLES2 = 0x45;
constexpr auto SOUND_INJURE1  = 0xbc;
constexpr auto SOUND_SXSTMO1  = 0xbe;
constexpr auto SOUND_SCREENON = 0x100;
constexpr auto SOUND_SXBUTT1  = 0x121;
constexpr auto SOUND_SXBUTT2  = 0x122;
constexpr auto SOUND_SXBUTT3  = 0x123;
constexpr auto SOUND_SXBUTT5  = 0x125;
constexpr auto SOUND_DOOROPN1 = 0x126;
constexpr auto SOUND_DOORCLS1 = 0x127;
constexpr auto SOUND_DOOROPN2 = 0x128;
constexpr auto SOUND_DOORCLS2 = 0x129;
constexpr auto SOUND_ENDSIR   = 0x14b;

// SNDFILES.LST's group column. The menu group is held loaded outside a match; the ambient group loops.
constexpr auto SOUND_GROUP_LOOPING = 0x29a;
constexpr auto SOUND_GROUP_PRELOAD = 0x309;

#endif
