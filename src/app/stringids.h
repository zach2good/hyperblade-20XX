#ifndef HYPERBLADE_STRINGIDS_H
#define HYPERBLADE_STRINGIDS_H

// The string table's ids, named for their text.
constexpr auto kStringAlert               = 0x23;
constexpr auto kStringOutOfMemory         = 0x24;
constexpr auto kStringHyperBlade          = 0x43;
constexpr auto kStringZeroNormalInPlane   = 0x5b;
constexpr auto kStringInsertCDOrReturn    = 0x61;
constexpr auto kStringCDNotFound          = 0x62;
constexpr auto kStringHyperBladeUppercase = 0x63;
constexpr auto kStringDiskFull            = 0x64;
constexpr auto kStringErrorDiskFull       = 0x65;
constexpr auto kStringInsertCDOrExit      = 0x66;
constexpr auto kStringExitMatch           = 0x9f;
constexpr auto kStringContinue            = 0xa0;
constexpr auto kStringVs                  = 0xa1;
// The first of three quit prompts.
constexpr auto kStringReallyQuit           = 0xa2;
constexpr auto kStringQuit                 = 0xa5;
constexpr auto kStringJoystickControlPanel = 0xa6;
constexpr auto kStringHighColor            = 0xa8;
constexpr auto kStringGamePadHelp          = 0xa9;
constexpr auto kStringTextureLoadFailed    = 0xaa;
// The disk write failure follows it.
constexpr auto kStringCDFileOpenFailed = 0xab;
constexpr auto kStringError            = 0xad;

#endif
