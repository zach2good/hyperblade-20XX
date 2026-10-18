// In their own unit so the menu's switch cannot inline them.

#include "common/compat.h"
#include "pausemenu.h"
#include "globals.h"
#include "engine/gameengine.h"
#include "app/gameapp.h"
#include "net/gamesession.h"
#include "engine/game.h"

// FUNCTION: HYPERX 0x00443930
void PauseMenu::ActionSave()
{
    g_nSetupReturnState = Game::SetEngineState(kEstateSaveLoad);
}

// The state comes from a global, so the return state is configurable at run time.
// FUNCTION: HYPERX 0x00443940
void PauseMenu::ActionReturn()
{
    Game::SetEngineState(g_nPostMatchEscState);
}

// FUNCTION: HYPERX 0x00443c70
void PauseMenu::ActionReturnTraining()
{
    Game::SetEngineState(g_nDefaultEscState);
}

// FUNCTION: HYPERX 0x00444120
void PauseMenu::ActionOptions()
{
    Game::SetEngineState(kEstateGameConfig);
}

// The post-match flag goes up first, so the match reads as finished rather than abandoned.
// FUNCTION: HYPERX 0x00444130
void PauseMenu::ActionExitToMenu()
{
    g_pGameEngine->m_bMatchOver = 1;

    g_pGameEngine->ResetPeriodToFirst();

    if (g_bSessionActive != 0)
    {
        GameSession::Disconnect();
    }

    GameApp::ResetToSinglePlayer();

    Game::SetEngineState(kEstateMainMenu);
}

// FUNCTION: HYPERX 0x00444170
void PauseMenu::ActionReturnNoSave()
{
    Game::SetEngineState(g_nJumbotronEscState);
}

// FUNCTION: HYPERX 0x00444180
void PauseMenu::ActionQuit()
{
    Game::SetEngineState(kEstateQuit);
}
