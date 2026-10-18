#ifndef HYPERBLADE_SAVELOADUISCREEN_H
#define HYPERBLADE_SAVELOADUISCREEN_H

#include "common/compat.h"
#include "engine/game.h"
#include "ui/uiscreen.h"

// The save and load page, appended after the chrome. Slot n's button and label are ten apart.
enum SaveLoadWidget
{
    SAVELOAD_HEADER      = UI_WIDGET_OWN_FIRST,
    SAVELOAD_SLOT_FIRST  = 24,
    SAVELOAD_SAVE        = 34,
    SAVELOAD_LOAD        = 35,
    SAVELOAD_LABEL_FIRST = 36,
    SAVELOAD_LAST_WIDGET = 45,
    SAVELOAD_SLOT_COUNT  = 10
};

// VTABLE: HYPERX 0x004b3868
class SaveLoadUIScreen : public UIScreen
{
public:
    // FUNCTION: HYPERX 0x0046eb40
    void OnOkayClick() override
    {
        Game::SetEngineState(static_cast<unsigned int>(m_nReturnState));
    }

    // The linker folded this body onto OnOkayClick, which carries the marker.
    void OnCancelClick() override
    {
        Game::SetEngineState(static_cast<unsigned int>(m_nReturnState));
    }

    void RefreshFrames() override;
    void OnOtherButton(int nMsg, unsigned int nId) override;
    SaveLoadUIScreen();
    ~SaveLoadUIScreen() override;

    void RefreshSlotLabels();

    void SaveGame();
    void FormatSaveDescription(char* pszOut, int nMax);

    void LoadGame();

private:
public:
    // Nought to nine, or kSlotNone.
    int m_nSlot;        // 0x474
    int m_nReturnState; // 0x478
    // Minus one until a slot is clicked.
    int m_nSelectedLabelWidget; // 0x47c
};

// SYNTHETIC: HYPERX 0x0046e930
// SaveLoadUIScreen::`scalar deleting destructor'

#endif
