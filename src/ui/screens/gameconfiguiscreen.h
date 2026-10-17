#ifndef HYPERBLADE_GAMECONFIGUISCREEN_H
#define HYPERBLADE_GAMECONFIGUISCREEN_H

#include "common/compat.h"
#include "ui/uiscreen.h"

// The options page, appended after the chrome. A row's display index is also its option id.
enum GameConfigWidget
{
    GAME_CONFIG_PANEL = UI_WIDGET_OWN_FIRST,
    GAME_CONFIG_CUSTOMIZE_KEYS,
    GAME_CONFIG_JOYSTICK_INFO,
    GAME_CONFIG_ATTACK_MODE,
    GAME_CONFIG_TEAMMATE_BIAS,
    GAME_CONFIG_PERIOD_LENGTH,
    GAME_CONFIG_MUSIC_VOLUME,
    GAME_CONFIG_SFX_VOLUME,
    GAME_CONFIG_BRUTALITY,
    GAME_CONFIG_DIFFICULTY,
    GAME_CONFIG_ATTACK_MODE_LEFT,
    GAME_CONFIG_ATTACK_MODE_RIGHT,
    GAME_CONFIG_TEAMMATE_BIAS_LEFT,
    GAME_CONFIG_TEAMMATE_BIAS_RIGHT,
    GAME_CONFIG_PERIOD_LENGTH_LEFT,
    GAME_CONFIG_PERIOD_LENGTH_RIGHT,
    GAME_CONFIG_MUSIC_VOLUME_LEFT,
    GAME_CONFIG_MUSIC_VOLUME_RIGHT,
    GAME_CONFIG_SFX_VOLUME_LEFT,
    GAME_CONFIG_SFX_VOLUME_RIGHT,
    GAME_CONFIG_BRUTALITY_LEFT,
    GAME_CONFIG_BRUTALITY_RIGHT,
    GAME_CONFIG_DIFFICULTY_LEFT,
    GAME_CONFIG_DIFFICULTY_RIGHT,

    GAME_CONFIG_BUTTON_COUNT = 2,
    GAME_CONFIG_ROW_COUNT    = 7,
    GAME_CONFIG_ARROW_COUNT  = 14
};

// The constructor calls UIScreen's at 0x0044d540.
// VTABLE: HYPERX 0x004b3da0
class GameConfigUIScreen : public UIScreen
{
public:
    void OnOkayClick() override;
    void OnCancelClick() override;
    void OnOtherButton(int nMsg, unsigned int nId) override;
    GameConfigUIScreen();
    ~GameConfigUIScreen() override;

    void SetupWidgets();

    const char* GetOptionBitmapName(int nId);
    void        LoadOptionBitmap(int nOption);

public:
    int m_nReturnState; // 0x474, the engine state to return to
};

// SYNTHETIC: HYPERX 0x00479ae0
// GameConfigUIScreen::`scalar deleting destructor'

#endif
