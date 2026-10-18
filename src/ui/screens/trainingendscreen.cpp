#include "common/compat.h"
#include "input/inputevent.h"
#include "globals.h"
#include "ui/uiglobals.h"
#include "ui/widget.h"
#include "ui/screen.h"
#include "engine/gameengine.h"
#include "ui/pausemenu.h"
#include "app/gameapp.h"
#include "net/gamesession.h"
#include "ui/screens/trainingendscreen.h"
#include "graphics/gdi.h"
#include "engine/game.h"

// The training screen's background is picked by difficulty level at construction.
// FUNCTION: HYPERX 0x00443210
TrainingEndScreen::TrainingEndScreen()
{
    m_nSelectionMode = 0;
    m_nLastWidget    = -1;

    m_hBackground = GDI::LoadBitmapAsBackground(
        g_apszTrainingBackgrounds[static_cast<int>(g_pGameEngine->GetDifficultyLevel())],
        &m_bPaletted);
}

// FUNCTION: HYPERX 0x004432b0
TrainingEndScreen::~TrainingEndScreen() = default;

// Past the last credit page the engine goes back to single player and the lobby.
// FUNCTION: HYPERX 0x004432c0
void TrainingEndScreen::OnWidgetMessage(int nMsg, unsigned int nId)
{
    if (nId >= kWidgetIDMouse && nId <= kWidgetIDReturn)
    {
        if (nMsg == kInputKeyUp || nMsg == kInputKeyDown || (m_bPointerOnWidget == 0 && nMsg == kInputLeftUp))
        {
            if (m_nSelectionMode == g_nCreditPageCount)
            {
                g_pGameEngine->m_bMatchOver = 1;

                g_pGameEngine->ResetPeriodToFirst();

                if (g_bSessionActive != 0)
                {
                    GameSession::Disconnect();
                }

                GameApp::ResetToSinglePlayer();
                Game::SetEngineState(kEstateLobby);
                return;
            }

            Screen::ClearBlack();

            GDI::SafeDeleteBitmap(m_hBackground);

            m_hBackground = nullptr;

            m_bPaletted = 0;

            m_hBackground = GDI::LoadBitmapAsBackground(
                g_apszTrainingBackgrounds[kTrainingCreditFirst + m_nSelectionMode],
                &m_bPaletted);

            SetRepaintBackground(1);

            InitFrame();

            m_nSelectionMode++;
        }
    }
}

const char* g_apszTrainingBackgrounds[8] = {
    "endtrain", "endpro", "endstar", "credit1", "credit2", "credit3", "credit4", "credit5"
};

int g_nCreditPageCount = 5;
