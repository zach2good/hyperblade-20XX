#ifndef HYPERBLADE_MENUIDS_H
#define HYPERBLADE_MENUIDS_H

// The developer menu's commands, named by the menu resource's labels where nothing else names them.
constexpr auto ID_MENU_ABOUT           = 1;
constexpr auto ID_MENU_BIRDS_EYE       = 0x65;
constexpr auto ID_MENU_CONFIGURE_KEYS  = 0x6a;
constexpr auto ID_MENU_SOUND_OFF       = 0x6b;
constexpr auto ID_MENU_OVER_SHOULDER   = 0x7b;
constexpr auto ID_MENU_SHOW_GOALIES    = 0x7d;
constexpr auto ID_MENU_DEBUG_DIALOG    = 0x84;
constexpr auto ID_MENU_RATINGS         = 0x88;
constexpr auto ID_MENU_TWO_PLAYER      = 0x9b;
constexpr auto ID_MENU_MIDI_OFF        = 0xcc;
constexpr auto ID_MENU_EXIT            = 2;
constexpr auto ID_MENU_FRAME_TIMES     = 3;
constexpr auto ID_MENU_SIZE_LARGE      = 4;
constexpr auto ID_MENU_SIZE_SMALL      = 5;
constexpr auto ID_MENU_SIZE_MEDIUM     = 0x66;
constexpr auto ID_MENU_PHYSICS_CONFIG  = 0x78;
constexpr auto ID_MENU_NEW_GAME        = 0x7c;
constexpr auto ID_MENU_PLAN_EDITOR     = 0x86;
constexpr auto ID_MENU_DRAG_OFF        = 0x87;
constexpr auto ID_MENU_DISPLAY_HUD_ART = 0x9e;

// The render menu's toggles, which WndProc::HandleRenderMenuCommand flips and checks.
constexpr auto ID_RENDER_MDM_TEXTURES    = 0x8e;
constexpr auto ID_RENDER_BILINEAR_FILTER = 0x9c;
constexpr auto ID_RENDER_PERSPECTIVE     = 0x9d;

#endif
