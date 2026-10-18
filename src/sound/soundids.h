#ifndef HYPERBLADE_SOUNDIDS_H
#define HYPERBLADE_SOUNDIDS_H

// Sound ids: positions in SNDFILES.LST, counting only the sound lines, named for their files.
constexpr auto kSoundSxhhite2 = 0;
constexpr auto kSoundSxxamba2 = 0x0e;
constexpr auto kSoundSxxambb2 = 0x0f;
constexpr auto kSoundSxvgrma3 = 0x18;
constexpr auto kSoundSasrles2 = 0x45;
constexpr auto kSoundInjure1  = 0xbc;
constexpr auto kSoundSxstmo1  = 0xbe;
constexpr auto kSoundScreenon = 0x100;
constexpr auto kSoundSxbutt1  = 0x121;
constexpr auto kSoundSxbutt2  = 0x122;
constexpr auto kSoundSxbutt3  = 0x123;
constexpr auto kSoundSxbutt5  = 0x125;
constexpr auto kSoundDooropn1 = 0x126;
constexpr auto kSoundDoorcls1 = 0x127;
constexpr auto kSoundDooropn2 = 0x128;
constexpr auto kSoundDoorcls2 = 0x129;
constexpr auto kSoundEndsir   = 0x14b;

// SNDFILES.LST's group column. The menu group is held loaded outside a match; the ambient group loops.
constexpr auto kSoundGroupLooping = 0x29a;
constexpr auto kSoundGroupPreload = 0x309;

#endif
