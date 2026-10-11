#ifndef HYPERBLADE_MENUIDS_H
#define HYPERBLADE_MENUIDS_H

// The developer menu's commands, named by the menu resource's labels where nothing else names them.
#define ID_MENU_ABOUT           1
#define ID_MENU_BIRDS_EYE       0x65
#define ID_MENU_CONFIGURE_KEYS  0x6a
#define ID_MENU_SOUND_OFF       0x6b
#define ID_MENU_OVER_SHOULDER   0x7b
#define ID_MENU_SHOW_GOALIES    0x7d
#define ID_MENU_DEBUG_DIALOG    0x84
#define ID_MENU_RATINGS         0x88
#define ID_MENU_TWO_PLAYER      0x9b
#define ID_MENU_MIDI_OFF        0xcc
#define ID_MENU_EXIT            2
#define ID_MENU_FRAME_TIMES     3
#define ID_MENU_SIZE_LARGE      4
#define ID_MENU_SIZE_SMALL      5
#define ID_MENU_SIZE_MEDIUM     0x66
#define ID_MENU_PHYSICS_CONFIG  0x78
#define ID_MENU_NEW_GAME        0x7c
#define ID_MENU_PLAN_EDITOR     0x86
#define ID_MENU_DRAG_OFF        0x87
#define ID_MENU_DISPLAY_HUD_ART 0x9e

// The render menu's toggles, which WndProc::HandleRenderMenuCommand flips and checks.
#define ID_RENDER_MDM_TEXTURES    0x8e
#define ID_RENDER_BILINEAR_FILTER 0x9c
#define ID_RENDER_PERSPECTIVE     0x9d

#endif
