// One enumeration per dialog: a control id means nothing outside its dialog.

#ifndef HYPERBLADE_DIALOGIDS_H
#define HYPERBLADE_DIALOGIDS_H

enum DebugDialogId
{
    IDC_ENERGY_ON       = 0x40e,
    IDC_VICTIM_CAM      = 0x41e,
    IDC_NAME_A0         = 0x468,
    IDC_NAME_A1         = 0x469,
    IDC_NAME_A2         = 0x46a,
    IDC_NAME_A3         = 0x46b,
    IDC_NAME_A4         = 0x46c,
    IDC_NAME_A5         = 0x46d,
    IDC_NAME_B0         = 0x46e,
    IDC_NAME_B1         = 0x46f,
    IDC_NAME_B2         = 0x470,
    IDC_NAME_B3         = 0x471,
    IDC_NAME_B4         = 0x472,
    IDC_NAME_B5         = 0x473,
    IDC_LOOK_HEIGHT     = 0x4a9,
    IDC_BUNGEE_LENGTH   = 0x4aa,
    IDC_SMART_CAM       = 0x4ab,
    IDC_SCANNING_MODES  = 0x4ac,
    IDC_TRACKING_HEIGHT = 0x4ad,
    IDC_MIN_HEIGHT      = 0x4ae,
    IDC_STEADY_CAM      = 0x4af,
    IDC_SOLAR_WIND      = 0x4b0,
    IDC_POSITION_X      = 0x4b5,
    IDC_POSITION_Y      = 0x4b6,
    IDC_POSITION_Z      = 0x4b7,
    IDC_WATCH_ROK       = 0x4b8,
    IDC_OFFSET_X        = 0x4b9,
    IDC_OFFSET_Y        = 0x4ba,
    IDC_OFFSET_Z        = 0x4bb,
    IDC_DIRECTION_X     = 0x4be,
    IDC_DIRECTION_Y     = 0x4bf,
    IDC_DIRECTION_Z     = 0x4c0
};

enum KeyconfigDialogId
{
    IDC_ACTION_COMBO = 0x3f0,
    IDC_KEY_COMBO    = 0x3f1,
    IDC_DEFAULTS     = 0x3f3,
    IDC_CLEAR        = 0x3f4,
    IDC_SHIFT        = 0x3f6,
    IDC_CONTROL      = 0x3f7,
    IDC_ALTERNATE    = 0x3fc,
    IDC_PRIMARY      = 0x3fd,
    IDC_SAVE_ON_EXIT = 0x41a,
    IDC_FILE_LABEL   = 0x41c
};

enum PhysicsDialogId
{
    IDC_SAVE_AS            = 0x3f3,
    IDC_FORCE_INCREMENT    = 0x3f9,
    IDC_MAX_ACCELERATION   = 0x3fa,
    IDC_MAX_SPEED          = 0x3fb,
    IDC_FORCE_RELAXATION   = 0x3fe,
    IDC_STOP_FORCE_FACTOR  = 0x3ff,
    IDC_SPEED_BURST_FACTOR = 0x400,
    IDC_BANDWIDTH          = 0x40d,
    IDC_NET_TRACKING       = 0x411,
    IDC_VIEW_STEEPNESS     = 0x414,
    IDC_PLAYER_LIST        = 0x41f,
    IDC_RATING_VALUE       = 0x423,
    IDC_CATEGORY_LIST      = 0x424
};

#endif
