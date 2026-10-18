#ifndef HYPERBLADE_MENUIDS_H
#define HYPERBLADE_MENUIDS_H

// The developer menu's commands, named by the menu resource's labels where nothing else names them.
constexpr auto kIDMenuAbout         = 1;
constexpr auto kIDMenuBirdsEye      = 0x65;
constexpr auto kIDMenuConfigureKeys = 0x6a;
constexpr auto kIDMenuSoundOff      = 0x6b;
constexpr auto kIDMenuOverShoulder  = 0x7b;
constexpr auto kIDMenuShowGoalies   = 0x7d;
constexpr auto kIDMenuDebugDialog   = 0x84;
constexpr auto kIDMenuRatings       = 0x88;
constexpr auto kIDMenuTwoPlayer     = 0x9b;
constexpr auto kIDMenuMIDIOff       = 0xcc;
constexpr auto kIDMenuExit          = 2;
constexpr auto kIDMenuFrameTimes    = 3;
constexpr auto kIDMenuSizeLarge     = 4;
constexpr auto kIDMenuSizeSmall     = 5;
constexpr auto kIDMenuSizeMedium    = 0x66;
constexpr auto kIDMenuPhysicsConfig = 0x78;
constexpr auto kIDMenuNewGame       = 0x7c;
constexpr auto kIDMenuPlanEditor    = 0x86;
constexpr auto kIDMenuDragOff       = 0x87;
constexpr auto kIDMenuDisplayHUDArt = 0x9e;

// The render menu's toggles, which WndProc::HandleRenderMenuCommand flips and checks.
constexpr auto kIDRenderMDMTextures    = 0x8e;
constexpr auto kIDRenderBilinearFilter = 0x9c;
constexpr auto kIDRenderPerspective    = 0x9d;

#endif
