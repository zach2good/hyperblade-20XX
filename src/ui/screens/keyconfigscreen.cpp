#include "common/compat.h"
#include "input/keycodes.h"
#include "input/inputevent.h"
#include "ui/keyconfigstate.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "graphics/gdi.h"
class KeyConfigScreen;
#include "ui/pausemenu.h"
#include "input/keyboarddeviceobj.h"
#include "input/inputglobals.h"
#include "ui/framepos.h"
#include "ui/screen.h"
#include "ui/widget.h"
#include "engine/devicector.h"
#include <cstring>
#include "ui/screens/keyconfigscreen.h"
#include "sound/audiomgr.h"
#include "ui/displaywidget.h"
#include "engine/game.h"
#include "sound/soundids.h"
#include "graphics/presenter.h"
#include "input/gamepad.h"
#include <SDL3/SDL.h>
#include <array>
#include <vector>

// A row bound to a key the bitmap table lacks falls back to "key-".

constexpr auto KEY_ACTION_COUNT = 12;
constexpr auto KEY_BITMAP_COUNT = 50;

// The twelve action buttons plus the ten key pictures, so the last slot in use is 21.
constexpr auto KEY_LAST_WIDGET = 0x15;

// Twelve entries in, which is what the original's base address says and nothing else explains.
constexpr auto KEY_START_FRAME = 12;

// GLOBAL: HYPERX 0x004bd090
extern KeyBitmapEntry g_aKeyBitmaps[KEY_BITMAP_COUNT];

// GLOBAL: HYPERX 0x004cb7d8
extern unsigned int g_anKeyConfigKeys[KEY_BINDING_COUNT];

// Written and never read.
// GLOBAL: HYPERX 0x004cb7b0
extern unsigned int g_anKeyConfigAccepted[KEY_BINDING_COUNT];

// The pad control bound to an action, or null when none is.
static const char* PadControl(const char* pszAction)
{
    for (int i = 0; i < Gamepad::BindingCount(); i++)
    {
        const auto binding = Gamepad::GetBinding(i);

        if (SDL_strcasecmp(binding.pszAction, pszAction) == 0)
        {
            return binding.pszControl;
        }
    }

    return nullptr;
}

// Each row's pad control, drawn small over the top right corner of its key picture.
static void ShowPadIcons(int nKeyWidth, int nKeyHeight)
{
    constexpr std::array<const char*, 4> STICK = { "StickUp", "StickDown", "StickLeft", "StickRight" };

    const auto nSize = nKeyHeight / 2;

    auto icons = std::vector<Presenter::CanvasIcon>();

    for (int i = 0; i < KEY_BINDING_COUNT; i++)
    {
        const int nSlot = g_anKeyConfigDefaultSlots[i];

        // The left stick steers, so the four directions show it.
        const auto* const pszControl = [&]
        {
            if (nSlot < static_cast<int>(STICK.size()))
            {
                return STICK[static_cast<size_t>(nSlot)];
            }

            return PadControl(g_apszKeyBindingNames[nSlot]);
        }();

        if (pszControl != nullptr)
        {
            icons.push_back(Presenter::CanvasIcon{ pszControl, g_aKeyDisplayPos[i].nX + nKeyWidth - nSize * 3 / 4, g_aKeyDisplayPos[i].nY - nSize / 4, nSize });
        }
    }

    Presenter::SetCanvasIcons(std::move(icons));
}

// FUNCTION: HYPERX 0x0042f9d0
KeyConfigScreen::KeyConfigScreen()
{
    Widget*      pWidget;
    unsigned int nIndex;
    int          i;

    m_nActiveRemap = -1;

    m_hBackground = GDI::LoadBitmapAsBackground(g_pszKeyConfigBackground, &m_bPaletted);
    m_nLastWidget = KEY_LAST_WIDGET;

    for (i = 0; i < KEY_ACTION_COUNT; i++)
    {
        m_hBitmap = GDI::LoadBitmapHandle(g_aKeyActionButtons[i].pszNormal);
        GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
        pWidget = AppendWidget(new ButtonWidget(2, g_aKeyActionButtons[i].nX, g_aKeyActionButtons[i].nY, m_bitmap.nWidth, m_bitmap.nHeight, 0, 0, 2, 0, 1, 2, this, -1));
        pWidget->AppendFrame(m_hBitmap);
        pWidget->AppendFrame(GDI::LoadBitmapHandle(g_aKeyActionButtons[i].pszRollover));
        pWidget->AppendFrame(GDI::LoadBitmapHandle(g_aKeyActionButtons[i].pszDepressed));
    }

    const unsigned int* pnDefaults = g_pInputDevice->GetDefaultKeyBindings();

    for (i = 0; i < KEY_BINDING_COUNT; i++)
    {
        g_anKeyConfigAccepted[i] = g_anKeyConfigKeys[i] = pnDefaults[g_anKeyConfigDefaultSlots[i]];
    }

    for (i = 0; i < KEY_BINDING_COUNT; i++)
    {
        m_hBitmap = GDI::LoadBitmapHandle(g_aKeyBitmaps[i + KEY_START_FRAME].pszBitmap);
        GDI::GetBitmapDimensions(m_hBitmap, &m_bitmap);
        pWidget = AppendWidget(new DisplayWidget(g_aKeyDisplayPos[i].nX,
                                                 g_aKeyDisplayPos[i].nY,
                                                 0,
                                                 0,
                                                 this,
                                                 -1));

        for (nIndex = 0; nIndex < KEY_BITMAP_COUNT; nIndex++)
        {
            if (g_anKeyConfigKeys[i] == g_aKeyBitmaps[nIndex].nKey)
            {
                break;
            }
        }

        if (nIndex >= KEY_BITMAP_COUNT)
        {
            // STRING: HYPERX 0x004bd4e8
            pWidget->AppendFrame(GDI::LoadBitmapHandle("key-"));
        }
        else
        {
            pWidget->AppendFrame(GDI::LoadBitmapHandle(g_aKeyBitmaps[nIndex].pszBitmap));
        }
    }

    InitAllWidgets();

    ShowPadIcons(m_bitmap.nWidth, m_bitmap.nHeight);
}

constexpr auto WIDGET_ACCEPT = 10;
constexpr auto WIDGET_CANCEL = 11;

// The pictures start at this widget; the rows start at widget nought.
constexpr auto KEY_BITMAP_SLOT = 12;

// Screen row to device action slot: 0 1 2 3 5 6 8 7 9 4.
// GLOBAL: HYPERX 0x004b2c38
extern const int g_anKeyConfigActionSlots[KEY_BINDING_COUNT];

// FUNCTION: HYPERX 0x0042fc30
KeyConfigScreen::~KeyConfigScreen()
{
    Presenter::SetCanvasIcons({});
}

// FUNCTION: HYPERX 0x0042fc40
void KeyConfigScreen::OnWidgetMessage(int nMsg, unsigned int nWidget)
{
    FramePos     posNamed;
    unsigned int nIndex;

    switch (nWidget)
    {
        case WIDGET_ACCEPT:
        case WIDGET_CANCEL:
            if (nMsg == INPUT_LEFT_UP)
            {
                if (nWidget == WIDGET_ACCEPT)
                {
                    memcpy(g_anKeyConfigAccepted, g_anKeyConfigKeys, sizeof(g_anKeyConfigKeys));

                    if (g_pInputDevice != nullptr)
                    {
                        unsigned int anOrdered[KEY_BINDING_COUNT];

                        for (int i = 0; i < KEY_BINDING_COUNT; i++)
                        {
                            anOrdered[i] = g_anKeyConfigKeys[g_anKeyConfigActionSlots[i]];
                        }

                        g_pInputDevice->CommitKeyBindings(anOrdered, KEY_BINDING_COUNT);
                    }
                }

                Game::SetEngineState(ESTATE_GAME_CONFIG);
            }

            break;

        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
            if (nMsg == INPUT_LEFT_UP)
            {
                const int nActive = m_nActiveRemap;

                if (nActive != -1)
                {
                    Widget* const pOld = m_apWidgets[nActive];

                    pOld->ClearFrames();

                    const unsigned int nKey = g_anKeyConfigKeys[nActive - KEY_BITMAP_SLOT];

                    for (nIndex = 0; nIndex < KEY_BITMAP_COUNT; nIndex++)
                    {
                        if (nKey == g_aKeyBitmaps[nIndex].nKey)
                        {
                            break;
                        }
                    }

                    if (nIndex >= KEY_BITMAP_COUNT)
                    {
                        // STRING: HYPERX 0x004bd4e8
                        pOld->AppendFrame(GDI::LoadBitmapHandle("key-"));
                    }
                    else
                    {
                        posNamed = pOld->AppendFrame(GDI::LoadBitmapHandle(g_aKeyBitmaps[nIndex].pszBitmap));
                    }

                    pOld->SetDirtyFlag(1);

                    m_nActiveRemap = -1;
                }

                const int     nSlot = nWidget + KEY_BITMAP_SLOT;
                Widget* const pNew  = m_apWidgets[nWidget + KEY_BITMAP_SLOT];

                pNew->ClearFrames();

                // STRING: HYPERX 0x004bd4e8
                pNew->AppendFrame(GDI::LoadBitmapHandle("key-"));

                pNew->SetDirtyFlag(1);

                m_nActiveRemap = nSlot;

                InitFrame();
            }

            break;
    }
}

inline int KeyConfigScreen::FindActiveRow()
{
    for (int i = 0; i <= m_nLastWidget; i++)
    {
        if (m_apWidgets[i]->GetDepressed() != 0)
        {
            return i;
        }
    }

    return -1;
}

// Tab and Escape keep the key. A duplicate or unknown key plays a refusal.
// FUNCTION: HYPERX 0x0042fdc0
int KeyConfigScreen::OnKeyMessage(unsigned int nKeyCode, unsigned int)
{
    unsigned int nIndex;

    const int nRow = FindActiveRow();

    if (nRow == -1 || nRow > KEY_BINDING_COUNT - 1)
    {
        return 0;
    }

    if (nKeyCode == Key::TAB || nKeyCode == Key::ESCAPE)
    {
        nKeyCode = g_anKeyConfigKeys[nRow];
    }
    else
    {
        for (int j = 0; j < KEY_BINDING_COUNT; j++)
        {
            if (j != nRow && g_anKeyConfigKeys[j] == nKeyCode)
            {
                g_pAudioMgr->PlaySpecific(SOUND_ENDSIR);

                return 1;
            }
        }

        const KeyBitmapEntry* pEntry;

        for (pEntry = g_aKeyBitmaps; pEntry < g_aKeyBitmaps + KEY_BITMAP_COUNT; pEntry++)
        {
            if (nKeyCode == pEntry->nKey)
            {
                break;
            }
        }

        // A key with no bitmap cannot be shown, so it is refused.
        if (pEntry >= g_aKeyBitmaps + KEY_BITMAP_COUNT)
        {
            g_pAudioMgr->PlaySpecific(SOUND_ENDSIR);

            return 1;
        }

        g_anKeyConfigKeys[nRow] = nKeyCode;
    }

    Widget* const pBitmap = m_apWidgets[nRow + KEY_BITMAP_SLOT];

    for (nIndex = 0; nIndex < KEY_BITMAP_COUNT; nIndex++)
    {
        if (nKeyCode == g_aKeyBitmaps[nIndex].nKey)
        {
            break;
        }
    }

    pBitmap->ClearFrames();

    if (nIndex >= KEY_BITMAP_COUNT)
    {
        // STRING: HYPERX 0x004bd4e8
        pBitmap->AppendFrame(GDI::LoadBitmapHandle("key-"));
    }
    else
    {
        pBitmap->AppendFrame(GDI::LoadBitmapHandle(g_aKeyBitmaps[nIndex].pszBitmap));
    }

    pBitmap->SetDirtyFlag(1);

    Widget* const pRow = m_apWidgets[nRow];

    pRow->SetDepressed(0);
    pRow->SetDirtyFlag(1);

    InitFrame();

    return 1;
}

// Key-config screen row to input action slot.
const int g_anKeyConfigActionSlots[KEY_BINDING_COUNT] = { 0, 1, 2, 3, 5, 6, 8, 7, 9, 4 };

const int g_anKeyConfigDefaultSlots[KEY_BINDING_COUNT] = { 0, 1, 2, 3, 9, 4, 5, 7, 6, 8 };

// Adjacent on purpose: the button loop's end bound lies inside the display positions.
// STRING: HYPERX 0x004bd4e0
const char* g_pszKeyConfigBackground = "KEYOPS";

KeyActionButton g_aKeyActionButtons[12] = {
    // STRING: HYPERX 0x004bd4d8
    { "OPFRWD",
      // STRING: HYPERX 0x004bd4d0
      "OPFRWDD",
      // STRING: HYPERX 0x004bd4c8
      "OPFRWDR",
      122,
      113 },
    // STRING: HYPERX 0x004bd4c0
    { "OPSTOP",
      // STRING: HYPERX 0x004bd4b8
      "OPSTOPD",
      // STRING: HYPERX 0x004bd4b0
      "OPSTOPR",
      173,
      152 },
    // STRING: HYPERX 0x004bd4a8
    { "ROTALT",
      // STRING: HYPERX 0x004bd4a0
      "ROTALTD",
      // STRING: HYPERX 0x004bd498
      "ROTALTR",
      75,
      190 },
    // STRING: HYPERX 0x004bd490
    { "ROTART",
      // STRING: HYPERX 0x004bd488
      "ROTARTD",
      // STRING: HYPERX 0x004bd480
      "ROTARTR",
      56,
      228 },
    // STRING: HYPERX 0x004bd478
    { "OPJUMP",
      // STRING: HYPERX 0x004bd470
      "OPJUMPD",
      // STRING: HYPERX 0x004bd468
      "OPJUMPR",
      52,
      264 },
    // STRING: HYPERX 0x004bd460
    { "ATTACK",
      // STRING: HYPERX 0x004bd458
      "ATTACKD",
      // STRING: HYPERX 0x004bd450
      "ATTACKR",
      389,
      114 },
    // STRING: HYPERX 0x004bd448
    { "BURST",
      // STRING: HYPERX 0x004bd440
      "BURSTD",
      // STRING: HYPERX 0x004bd438
      "BURSTR",
      388,
      152 },
    // STRING: HYPERX 0x004bd430
    { "OPPASS",
      // STRING: HYPERX 0x004bd428
      "OPPASSD",
      // STRING: HYPERX 0x004bd420
      "OPPASSR",
      386,
      188 },
    // STRING: HYPERX 0x004bd418
    { "OPPSHOT",
      // STRING: HYPERX 0x004bd40c
      "OPPSHOTD",
      // STRING: HYPERX 0x004bd400
      "OPPSHOTR",
      386,
      226 },
    // STRING: HYPERX 0x004bd3f8
    { "OPQSHOT",
      // STRING: HYPERX 0x004bd3ec
      "OPQSHOTD",
      // STRING: HYPERX 0x004bd3e0
      "OPQSHOTR",
      386,
      264 },
    // STRING: HYPERX 0x004bd3d8
    { "OPOKAY",
      // STRING: HYPERX 0x004bd3d0
      "OPOKAYD",
      // STRING: HYPERX 0x004bd3c8
      "OPOKAYR",
      170,
      305 },
    // STRING: HYPERX 0x004bd3c0
    { "OPCANC",
      // STRING: HYPERX 0x004bd3b8
      "OPCANCD",
      // STRING: HYPERX 0x004bd3b0
      "OPCANCR",
      344,
      305 }
};

ConfigPoint g_aKeyDisplayPos[10] = {
    { 261, 112 }, { 261, 150 }, { 261, 188 }, { 261, 226 }, { 262, 264 }, { 330, 112 }, { 330, 150 }, { 330, 188 }, { 330, 226 }, { 330, 264 }
};

// Last in the file: the bitmap-name literals must follow it, and the key searches bound on its end.
KeyBitmapEntry g_aKeyBitmaps[50] = {
    { 0x41, "keya" },
    { 0x42, "keyb" },
    { 0x43, "keyc" },
    { 0x44, "keyd" },
    { 0x45, "keye" },
    { 0x46, "keyf" },
    { 0x47, "keyg" },
    { 0x48, "keyh" },
    { 0x49, "keyi" },
    { 0x4a, "keyj" },
    { 0x4b, "keyk" },
    { 0x4c, "keyl" },
    { 0x4d, "keym" },
    { 0x4e, "keyn" },
    { 0x4f, "keyo" },
    { 0x50, "keyp" },
    { 0x51, "keyq" },
    { 0x52, "keyr" },
    { 0x53, "keys" },
    { 0x54, "keyt" },
    { 0x55, "keyu" },
    { 0x56, "keyv" },
    { 0x57, "keyw" },
    { 0x58, "keyx" },
    { 0x59, "keyy" },
    { 0x5a, "keyz" },
    { 0x25, "keyarlt" },
    { 0x27, "keyarrt" },
    { 0x26, "keyarup" },
    { 0x28, "keyardn" },
    { 0x21, "keypgup" },
    { 0x22, "keypgdn" },
    { 0x20, "keyspac" },
    { 0x10, "keyshif" },
    { 0x09, "keytab" },
    { 0x0d, "keyentr" },
    { 0x24, "keyhome" },
    { 0x23, "keyend" },
    { 0x08, "keyback" },
    { 0x11, "keyctrl" },
    { 0x2d, "keyins" },
    { 0x2e, "keydel" },
    { 0x6a, "keyastr" },
    { 0x6f, "keyfrsl" },
    { 0xdc, "keybasl" },
    { 0xbc, "keycoma" },
    { 0xba, "keysemi" },
    { 0xbe, "keyperi" },
    { 0xdb, "keylbrk" },
    // The first string after the table, so 0x004bd220 is both the end bound and this literal.
    // STRING: HYPERX 0x004bd220
    { 0xdd, "keyrbrk" },
};
