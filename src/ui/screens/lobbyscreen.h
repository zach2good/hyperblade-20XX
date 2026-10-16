// The lobby screen: the doors and signs the player picks a destination from.

#ifndef HYPERBLADE_LOBBYSCREEN_H
#define HYPERBLADE_LOBBYSCREEN_H

#include "ui/pausemenu.h"

// The play door and the sign above it light together and lead to the main menu.
enum LobbyWidget
{
    LOBBY_DATA_DOOR,
    LOBBY_PLAY_DOOR,
    LOBBY_OPTIONS_DOOR,
    LOBBY_LEFT_LIGHT,
    LOBBY_RIGHT_LIGHT,
    LOBBY_PLAY_SIGN
};

// VTABLE: HYPERX 0x004b404c
class LobbyScreen : public Screen
{
public:
    LobbyScreen();
    ~LobbyScreen();
    virtual void OnWidgetMessage(int nMsg, unsigned int nId);
};

static_assert(sizeof(LobbyScreen) == 0x464);

// SYNTHETIC: HYPERX 0x0048cee0
// LobbyScreen::`scalar deleting destructor'

#endif
