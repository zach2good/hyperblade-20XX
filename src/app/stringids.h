#ifndef HYPERBLADE_STRINGIDS_H
#define HYPERBLADE_STRINGIDS_H

// The string table's ids, named for their text.
constexpr auto STRING_ALERT                = 0x23;
constexpr auto STRING_OUT_OF_MEMORY        = 0x24;
constexpr auto STRING_HYPERBLADE           = 0x43;
constexpr auto STRING_ZERO_NORMAL_IN_PLANE = 0x5b;
constexpr auto STRING_INSERT_CD_OR_RETURN  = 0x61;
constexpr auto STRING_CD_NOT_FOUND         = 0x62;
constexpr auto STRING_HYPERBLADE_UPPERCASE = 0x63;
constexpr auto STRING_DISK_FULL            = 0x64;
constexpr auto STRING_ERROR_DISK_FULL      = 0x65;
constexpr auto STRING_INSERT_CD_OR_EXIT    = 0x66;
constexpr auto STRING_EXIT_MATCH           = 0x9f;
constexpr auto STRING_CONTINUE             = 0xa0;
constexpr auto STRING_VS                   = 0xa1;
// The first of three quit prompts.
constexpr auto STRING_REALLY_QUIT            = 0xa2;
constexpr auto STRING_QUIT                   = 0xa5;
constexpr auto STRING_JOYSTICK_CONTROL_PANEL = 0xa6;
constexpr auto STRING_HIGH_COLOR             = 0xa8;
constexpr auto STRING_GAME_PAD_HELP          = 0xa9;
constexpr auto STRING_TEXTURE_LOAD_FAILED    = 0xaa;
// The disk write failure follows it.
constexpr auto STRING_CD_FILE_OPEN_FAILED = 0xab;
constexpr auto STRING_ERROR               = 0xad;

#endif
