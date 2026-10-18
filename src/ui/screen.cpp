#include "common/compat.h"
#include "input/keycodes.h"
#include "input/inputevent.h"
#include <algorithm>
#include "globals.h"
#include "ui/screen.h"
#include "common/gametime.h"
#include "sound/audiomgr.h"
#include "input/inputglobals.h"
#include "input/input.h"
#include "graphics/gdi.h"
struct GameApp;
#include "uiglobals.h"
#include "pausemenu.h"
#include "widget.h"
#include "app/gameapp.h"
#include "torchbitmaps.h"
#include "graphics/graphicsglobals.h"
#include "framepos.h"

#include <cstring>
#include "engine/game.h"
#include "app/window.h"
#include "common/stub.h"

constexpr auto TICK_INTERVAL_MS = 50;

// Slot zero is the shared answer-nothing at 0x0041bf40; slot two is the pure-virtual trap.
// FUNCTION: HYPERX 0x0041bf40
int Screen::OnKeyMessage(unsigned int, unsigned int)
{
    return 0;
}

// FUNCTION: HYPERX 0x0048bf00
Screen::Screen()
{
    if (g_nBitmapCacheReady != 0)
    {
        InitBitmapCache();
    }

    m_hBackground = nullptr;
    m_bPaletted   = 0;
    m_pszCaption  = nullptr;
    m_nCaptionX   = 0;
    m_nCaptionY   = 0;
    m_nLastWidget = -1;

    for (int nSlot = 0; nSlot < 0x100; nSlot++)
    {
        m_apWidgets[nSlot] = nullptr;
    }

    m_nUnusedC           = 0;
    m_bRepaintBackground = 1;
    m_bPointerOnWidget   = 1;
    m_nWidgetCount       = 0;
    m_bitmap.nWidth      = 0;
    m_bitmap.nHeight     = 0;
    m_hBitmap            = nullptr;
    m_nUnusedA           = 0;
    m_nUnusedB           = 0;
    m_nCaptured          = -1;
    m_bFramePaintPending = 0;

    GameTime::Now(&m_lastTick);
}

// The original's link folded it with every other empty two-argument member.
// FUNCTION: HYPERX 0x00480fb0
void Screen::OnWidgetMessage(int, unsigned int)
{
}

// FUNCTION: HYPERX 0x0048bfe0
Screen::~Screen()
{
    for (int nSlot = 0; nSlot <= m_nLastWidget; nSlot++)
    {
        // Every widget's destructor is empty, so the base's non-virtual one destroys it fully.
        delete m_apWidgets[nSlot];

        m_apWidgets[nSlot] = nullptr;
    }

    delete[] m_pszCaption;
    m_pszCaption = nullptr;

    GDI::SafeDeleteBitmap(m_hBackground);
    m_hBackground = nullptr;

    m_bPaletted = 0;
}

// FUNCTION: HYPERX 0x0048c060
void Screen::InitFrame()
{
    m_bFramePaintPending = 1;

    Window::InvalidateAndUpdate();
}

// FUNCTION: HYPERX 0x0048c070
int Screen::OnMouseMessage(unsigned int nMsg, unsigned int, int nX, int nY)
{
    if (nMsg == INPUT_LEFT_UP)
    {
        if (m_nCaptured != -1 && IsButtonWidget(m_nCaptured) && m_apWidgets[m_nCaptured]->HitTest(nX, nY) == 0)
        {
            m_apWidgets[m_nCaptured]->SetHighlighted(0);
        }

        m_nCaptured = -1;
    }

    int bHandled = 0;

    for (int i = 0; i <= m_nLastWidget && bHandled == 0; i++)
    {
        if (m_apWidgets[i]->HitTest(nX, nY) != 0)
        {
            bHandled = 1;

            switch (nMsg)
            {
                case INPUT_MOUSE_MOVE:
                    if (m_nCaptured != -1 && IsButtonWidget(i))
                    {
                        if (m_nCaptured == i)
                        {
                            m_apWidgets[i]->SetDepressed(1);
                            m_apWidgets[i]->SetDirtyFlag(1);

                            InitFrame();
                        }
                    }
                    else if (m_apWidgets[i]->TryActivate(nX, nY) != 0)
                    {
                        InitFrame();
                    }

                    OnWidgetMessage(INPUT_MOUSE_MOVE, i);
                    break;

                case INPUT_LEFT_DOWN:
                    m_nCaptured = i;

                    if (m_apWidgets[i]->OnMouseDown() != 0)
                    {
                        InitFrame();

                        const int nSound = m_apWidgets[i]->m_flipbook.m_nSoundAtMark;

                        if (nSound > -1)
                        {
                            g_pAudioMgr->PlaySpecific(nSound);
                        }
                    }
                    break;

                case INPUT_LEFT_UP:
                    if (m_apWidgets[i]->OnMouseUp() != 0)
                    {
                        InitFrame();
                    }

                    OnWidgetMessage(INPUT_LEFT_UP, i);
                    break;
            }
        }
        else if (m_nCaptured == i && IsButtonWidget(i))
        {
            m_apWidgets[i]->SetDepressed(0);
            m_apWidgets[i]->SetDirtyFlag(1);
        }
    }

    if (m_bPointerOnWidget != 0 && bHandled == 0)
    {
        ResetAllHighlights(0);
        ResetAllDepressedStates(0);

        if (m_nCaptured != -1 && IsButtonWidget(m_nCaptured))
        {
            m_apWidgets[m_nCaptured]->SetHighlighted(1);
        }

        InitFrame();
    }

    m_bPointerOnWidget = bHandled;

    if (nMsg != INPUT_MOUSE_MOVE)
    {
        OnWidgetMessage(nMsg, WIDGET_ID_MOUSE);
    }

    return 1;
}

// FUNCTION: HYPERX 0x0048c2a0
int Screen::HandleKeyMessage(int nMsg, unsigned int wParam, unsigned int lParam)
{
    int i;

    if (nMsg != INPUT_KEY_DOWN)
    {
        return 1;
    }

    if (OnKeyMessage(wParam, lParam))
    {
        return 1;
    }

    const int nFocused = FindFocusedWidget();

    if (nFocused != -1)
    {
        Widget* const pWidget = m_apWidgets[nFocused];

        if (pWidget->OnKeyPress(wParam))
        {
            pWidget->SetDirtyFlag(1);
            pWidget->SetHidden(0);
            InitFrame();
            return 1;
        }
    }

    switch (wParam)
    {
        case Key::TAB:
            if (Input::IsShiftKeyDown())
            {
                MoveFocusPrev();
                InitFrame();
                return 1;
            }

            MoveFocusNext();
            InitFrame();
            return 1;

        case Key::LEFT:
            MoveFocusPrev();
            InitFrame();
            return 1;

        case Key::RIGHT:
            break;

        case Key::ESCAPE:
            Screen::ClearBlack();

            if (g_gameApp.m_nEngineState == ESTATE_PAUSE_FULL || g_gameApp.m_nEngineState == ESTATE_PAUSE_MINIMAL || g_gameApp.m_nEngineState == ESTATE_PAUSE_NO_SAVE)
            {
                Game::SetEngineState(g_nLastEscState);
                return 1;
            }

            if ((g_gameApp.m_nEngineState == ESTATE_JUMBOTRON && g_bSessionActive == 0) || (g_gameApp.m_nEngineState == ESTATE_DROME_EXTERIOR && g_nPlayerMode == PLAYER_MODE_EXHIBITION))
            {
                g_nJumbotronEscState = Game::SetEngineState(ESTATE_PAUSE_NO_SAVE);
                g_nLastEscState      = g_nJumbotronEscState;
                return 1;
            }

            if (g_gameApp.m_nEngineState == ESTATE_DROME_EXTERIOR)
            {
                g_nPostMatchEscState = Game::SetEngineState(ESTATE_PAUSE_FULL);
                g_nLastEscState      = g_nPostMatchEscState;
                return 1;
            }

            g_nDefaultEscState = Game::SetEngineState(ESTATE_PAUSE_MINIMAL);
            g_nLastEscState    = g_nDefaultEscState;
            return 1;

        case Key::RETURN:
            for (i = 0; i <= m_nLastWidget; i++)
            {
                if (m_apWidgets[i]->GetHighlighted() && m_apWidgets[i]->IsDisplayOnly() == 0 && m_apWidgets[i]->GetHidden() == 0)
                {
                    if (m_apWidgets[i]->OnMouseUp())
                    {
                        InitFrame();
                    }

                    OnWidgetMessage(INPUT_LEFT_UP, i);
                }
            }

            InitFrame();
            OnWidgetMessage(INPUT_KEY_DOWN, WIDGET_ID_RETURN);
            return 1;

        default:
            return 1;
    }

    MoveFocusNext();
    InitFrame();

    return 1;
}

// The backward walk gives up after a full turn; the forward one does not and can spin forever.
// FUNCTION: HYPERX 0x0048c4f0
void Screen::MoveFocusPrev()
{
    int nGuard;
    int i;

    for (i = 0; i <= m_nLastWidget; i++)
    {
        if (m_apWidgets[i]->GetHighlighted() == 1 && m_apWidgets[i]->IsDisplayOnly() == 0 && m_apWidgets[i]->GetHidden() == 0)
        {
            m_apWidgets[i]->SetHighlighted(0);

            i--;

            if (i < 0)
            {
                i = m_nLastWidget;
            }

            nGuard = 0;

            while (m_apWidgets[i]->IsDisplayOnly() != 0 || m_apWidgets[i]->GetHidden() != 0)
            {
                i--;

                if (i < 0)
                {
                    i = m_nLastWidget;
                }

                if (nGuard > m_nLastWidget)
                {
                    break;
                }

                nGuard++;
            }

            if (m_apWidgets[i]->TryActivate(0, 0) != 0)
            {
                InitFrame();
            }

            OnWidgetMessage(INPUT_MOUSE_MOVE, i);

            return;
        }
    }

    i      = 0;
    nGuard = 0;

    while (m_apWidgets[i]->IsDisplayOnly() != 0 || m_apWidgets[i]->GetHidden() != 0)
    {
        i++;

        if (i > m_nLastWidget)
        {
            i = 0;
        }

        if (nGuard > m_nLastWidget)
        {
            break;
        }

        nGuard++;
    }

    if (m_apWidgets[i]->TryActivate(0, 0) != 0)
    {
        InitFrame();
    }

    OnWidgetMessage(INPUT_MOUSE_MOVE, i);
}

inline int Screen::FindHighlightedWidget(int* pnIndex)
{
    int i = 0;

    do
    {
        if (m_apWidgets[i]->GetHighlighted() != 0 && m_apWidgets[i]->IsDisplayOnly() == 0 && m_apWidgets[i]->GetHidden() == 0)
        {
            *pnIndex = i;
            return 1;
        }

        i++;
    } while (i <= m_nLastWidget);

    return 0;
}

// FUNCTION: HYPERX 0x0048c600
void Screen::MoveFocusNext()
{
    int i;

    if (FindHighlightedWidget(&i) == 0)
    {
        for (i = 0; i <= m_nLastWidget; i++)
        {
            if (m_apWidgets[i]->IsDisplayOnly() == 0 && m_apWidgets[i]->GetHidden() == 0)
            {
                if (m_apWidgets[i]->TryActivate(0, 0) != 0)
                {
                    InitFrame();
                }

                OnWidgetMessage(INPUT_MOUSE_MOVE, i);

                return;
            }
        }

        return;
    }

    ResetAllHighlights(0);

    i++;

    if (i > m_nLastWidget)
    {
        i = 0;
    }

    while (m_apWidgets[i]->IsDisplayOnly() != 0 || m_apWidgets[i]->GetHidden() != 0)
    {
        i++;

        if (i > m_nLastWidget)
        {
            i = 0;
        }
    }

    if (m_apWidgets[i]->TryActivate(0, 0) != 0)
    {
        InitFrame();
    }

    OnWidgetMessage(INPUT_MOUSE_MOVE, i);
}

// Twenty frames a second: below fifty milliseconds nothing moves.
// FUNCTION: HYPERX 0x0048c710
int Screen::TickAnimations()
{
    SystemTimePair now;

    int bDirty = 0;

    GameTime::Now(&now);

    SystemTimePair* const pTick = &m_lastTick;

    if ((now.nSeconds - pTick->nSeconds) * 1000 - static_cast<unsigned short>(m_lastTick.nMillis) + (now.nMillis & 0xffff) < TICK_INTERVAL_MS)
    {
        return 1;
    }

    GameTime::Now(pTick);

    for (int nSlot = 0; nSlot <= m_nLastWidget; nSlot++)
    {
        if (m_apWidgets[nSlot]->AdvanceFrame() != 0)
        {
            bDirty = 1;
        }
    }

    if (bDirty == 1)
    {
        InitFrame();
    }

    return 1;
}

// m_bRepaintBackground gates the background and caption.
// FUNCTION: HYPERX 0x0048c7c0
int Screen::PaintToWindow()
{
    GDI::BitmapSize bitmap;
    int             nSlot;

    if (g_pActiveScreen == nullptr)
    {
        return 0;
    }

    GDI::GetBitmapDimensions(m_hBackground, &bitmap);

    if (m_bFramePaintPending == 0)
    {
        m_bRepaintBackground = 1;
    }

    m_bFramePaintPending = 0;

    if (m_bRepaintBackground == 1)
    {
        GameApp::SetCursorVisible(0);

        if (m_hBackground != nullptr)
        {
            GDI::Blit(0, 0, bitmap.nWidth, bitmap.nHeight, m_hBackground, 0, 0);
        }

        const char* pszCaption = m_pszCaption;
        m_bRepaintBackground   = 0;

        if (pszCaption != nullptr)
        {
            GDI::DrawTransparentText(0xffff, m_nCaptionX, m_nCaptionY, pszCaption, static_cast<int>(strlen(pszCaption)), 0);
        }

        for (nSlot = 0; nSlot <= m_nLastWidget; nSlot++)
        {
            m_apWidgets[nSlot]->SetDirtyFlag(1);
        }

        g_nSavedCursorVisible = GameApp::SetCursorVisible(1);
    }

    for (nSlot = 0; nSlot <= m_nLastWidget; nSlot++)
    {
        m_apWidgets[nSlot]->Draw(nSlot);
    }

    Window::PresentMenu();

    return 1;
}

// FUNCTION: HYPERX 0x0048c9e0
void Screen::SetWidgetDepressedByType(int nType, int nValue)
{
    for (int nSlot = 0; nSlot <= m_nLastWidget; nSlot++)
    {
        if (m_apWidgets[nSlot]->GetTypeID() == nType)
        {
            m_apWidgets[nSlot]->SetDepressed(nValue);
            m_apWidgets[nSlot]->SetDirtyFlag(1);
        }
    }
}

// FUNCTION: HYPERX 0x0048ca30
void Screen::ResetAllDepressedStates(int nValue)
{
    for (int i = 0; i <= m_nLastWidget; i++)
    {
        if (m_apWidgets[i]->GetTypeID() != WIDGET_BUTTON_0 && m_apWidgets[i]->GetTypeID() != WIDGET_BUTTON_1 && m_apWidgets[i]->GetTypeID() != WIDGET_BUTTON_2 && m_apWidgets[i]->GetTypeID() != WIDGET_BUTTON_3 && m_apWidgets[i]->GetTypeID() != WIDGET_LOOP_FLIPBOOK)
        {
            m_apWidgets[i]->SetDepressed(nValue);
            m_apWidgets[i]->SetDirtyFlag(1);
        }
    }
}

// FUNCTION: HYPERX 0x0048cab0
void Screen::ResetAllHighlights(int nValue)
{
    for (int i = 0; i <= m_nLastWidget; i++)
    {
        if (m_apWidgets[i]->GetTypeID() != WIDGET_LOOP_FLIPBOOK)
        {
            m_apWidgets[i]->SetHighlighted(nValue);
            m_apWidgets[i]->SetDirtyFlag(1);
        }
    }
}

// FUNCTION: HYPERX 0x0048cb00
int Screen::IsButtonWidget(int nSlot)
{
    if (m_apWidgets[nSlot]->GetTypeID() == WIDGET_BUTTON_0 || m_apWidgets[nSlot]->GetTypeID() == WIDGET_BUTTON_1 || m_apWidgets[nSlot]->GetTypeID() == WIDGET_BUTTON_2 || m_apWidgets[nSlot]->GetTypeID() == WIDGET_BUTTON_3)
    {
        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0048cb60
void Screen::SetRepaintBackground(int nValue)
{
    m_bRepaintBackground = nValue;
}

// Nothing stops the array being overrun: the bounds check goes to a stub that discards it.
// FUNCTION: HYPERX 0x0048cb70
Widget* Screen::AppendWidget(Widget* pWidget)
{
    RELEASE_ASSERT(m_nWidgetCount <= m_nLastWidget);

    m_apWidgets[m_nWidgetCount] = pWidget;

    m_nWidgetCount++;

    return pWidget;
}

// The original's linker folded this onto Widget::GetPositionX, so it has no address of its own.
int Screen::HasPalette() const
{
    return m_bPaletted;
}

// The consistency check is handed to a stub that discards it.
// FUNCTION: HYPERX 0x0048cbc0
void Screen::InitAllWidgets()
{
    RELEASE_ASSERT(m_nWidgetCount == m_nLastWidget + 1);

    for (int nSlot = 0; nSlot <= m_nLastWidget; nSlot++)
    {
        m_apWidgets[nSlot]->InitAfterLoad();
    }
}

constexpr auto BITMAP_CACHE_SLOTS = 128;

// FUNCTION: HYPERX 0x0048cc10
void Screen::InitBitmapCache()
{
    FlushBitmapCache();

    g_pBitmapCache = new BitmapCacheEntry[BITMAP_CACHE_SLOTS];

    if (g_pBitmapCache != nullptr)
    {
        for (int i = 0; i < BITMAP_CACHE_SLOTS; i++)
        {
            g_pBitmapCache[i].hBitmap = nullptr;
            g_pBitmapCache[i].pszName = nullptr;
        }
    }

    g_nBitmapCacheReady = 0;
}

// FUNCTION: HYPERX 0x0048cc60
int Screen::AddBitmap(const char* pszName)
{
    g_nBitmapCacheCount++;

    if (g_nBitmapCacheCount < BITMAP_CACHE_SLOTS)
    {
        g_pBitmapCache[g_nBitmapCacheCount].hBitmap = GDI::LoadBitmapWithPalette(pszName, nullptr);
        g_pBitmapCache[g_nBitmapCacheCount].pszName = pszName;

        return 1;
    }

    g_nBitmapCacheCount--;

    return 0;
}

// FUNCTION: HYPERX 0x0048ccc0
int Screen::RemoveBitmap(const char* pszName)
{
    if (g_nBitmapCacheCount != 0)
    {
        for (int i = 0; i < BITMAP_CACHE_SLOTS; i++)
        {
            if (g_pBitmapCache[i].pszName == pszName && g_pBitmapCache[i].hBitmap != nullptr)
            {
                GDI::SafeDeleteBitmap(g_pBitmapCache[i].hBitmap);

                for (int j = i; j < 127; j++)
                {
                    g_pBitmapCache[j].pszName = g_pBitmapCache[j + 1].pszName;
                    g_pBitmapCache[j].hBitmap = g_pBitmapCache[j + 1].hBitmap;
                }

                g_pBitmapCache[127].pszName = nullptr;
                g_pBitmapCache[127].hBitmap = nullptr;

                g_nBitmapCacheCount = (std::max)(0, g_nBitmapCacheCount);

                return 1;
            }
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x0048cd80
GDI::Bitmap* Screen::CacheLookup(const char* pszName, int* pbCached)
{
    for (int nSlot = 0; nSlot < BITMAP_CACHE_SLOTS; nSlot++)
    {
        if (g_pBitmapCache[nSlot].pszName != nullptr)
        {
            if (strcmp(g_pBitmapCache[nSlot].pszName,
                       pszName) == 0)
            {
                *pbCached = 1;

                return g_pBitmapCache[nSlot].hBitmap;
            }
        }
    }

    *pbCached = 0;

    return GDI::LoadBitmapWithPalette(pszName, nullptr);
}

// FUNCTION: HYPERX 0x0048cde0
void Screen::FlushBitmapCache()
{
    if (g_pBitmapCache != nullptr)
    {
        for (int i = 0; i < BITMAP_CACHE_SLOTS; i++)
        {
            if (g_pBitmapCache[i].hBitmap != nullptr)
            {
                GDI::SafeDeleteBitmap(g_pBitmapCache[i].hBitmap);

                g_pBitmapCache[i].hBitmap = nullptr;
                g_pBitmapCache[i].pszName = nullptr;
            }
        }

        delete[] g_pBitmapCache;

        g_pBitmapCache = nullptr;
    }

    g_nBitmapCacheReady = 1;
}

// Ignores its argument; the two call sites disagree about the arity.

// FUNCTION: HYPERX 0x0048ce60
int Screen::FindFocusedWidget()
{
    for (int i = 0; i < m_nLastWidget + 1; i++)
    {
        if (m_apWidgets[i]->GetDepressed() != 0 && m_apWidgets[i]->GetTypeID() != WIDGET_LOOP_FLIPBOOK)
        {
            return i;
        }
    }

    return -1;
}

// A member: its callers load this into ECX though the body never reads it.
// FUNCTION: HYPERX 0x0048ef20
void Screen::ShowMatchInfo()
{
    Game::SetEngineState(ESTATE_MATCH_INFO);
}

// FUNCTION: HYPERX 0x0048ef30
void Screen::ClearBlack(...)
{
    const auto canvas = Window::MenuPixels();

    std::fill(canvas.pBits, canvas.pBits + canvas.nWidth * canvas.nHeight, 0u);
    Window::PresentMenu();
}

unsigned int g_nPostMatchEscState = ESTATE_LOBBY;
unsigned int g_nDefaultEscState   = ESTATE_LOBBY;
unsigned int g_nJumbotronEscState = ESTATE_LOBBY;
unsigned int g_nLastEscState      = ESTATE_LOBBY;
int          g_nBitmapCacheReady  = 1;
