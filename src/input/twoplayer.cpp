#include "input/twoplayer.h"
#include "app/wndproc.h"
#include "common/sdlmemory.h"
#include "engine/engineobject.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/spectator.h"
#include "globals.h"
#include "input/keyboarddeviceobj.h"
#include <algorithm>
#include <array>

namespace
{

// What the keyboard device keeps per player at the keys.
struct DeviceState
{
    unsigned int                                       nHeldMask     = 0;
    unsigned int                                       nPrevHeldMask = 0;
    unsigned int                                       nStickyMask   = 0;
    std::array<Clock::Milliseconds, kInputSlotCount>   aPrevPressTime{};
    std::array<Clock::Milliseconds, kInputSlotCount>   aPressTime{};
    std::array<Clock::Milliseconds, kInputSlotCount>   aPrevReleaseTime{};
    std::array<Clock::Milliseconds, kInputSlotCount>   aReleaseTime{};
    std::array<struct ActionBinding*, kInputSlotCount> apStickyBinding{};
};

bool s_bOn = false;

int s_nSecondTeam = 1;

// The player not at the keys right now: their device state and their dromer.
DeviceState s_other;
short       s_nOtherViewed = 1;

bool s_bSecondActive = false;

void SwapDevice(KeyboardDeviceObj& device)
{
    std::swap(device.m_nHeldMask, s_other.nHeldMask);
    std::swap(device.m_nPrevHeldMask, s_other.nPrevHeldMask);
    std::swap(device.m_nStickyMask, s_other.nStickyMask);
    std::swap_ranges(std::begin(device.m_aPrevPressTime), std::end(device.m_aPrevPressTime), s_other.aPrevPressTime.begin());
    std::swap_ranges(std::begin(device.m_aPressTime), std::end(device.m_aPressTime), s_other.aPressTime.begin());
    std::swap_ranges(std::begin(device.m_aPrevReleaseTime), std::end(device.m_aPrevReleaseTime), s_other.aPrevReleaseTime.begin());
    std::swap_ranges(std::begin(device.m_aReleaseTime), std::end(device.m_aReleaseTime), s_other.aReleaseTime.begin());
    std::swap_ranges(std::begin(device.m_apStickyBinding), std::end(device.m_apStickyBinding), s_other.apStickyBinding.begin());
}

// Hands the keys and the view owner to the other player.
void SwapPlayers()
{
    SwapDevice(*g_pInputDevice);
    std::swap(g_pGameEngine->m_nViewedSlot, s_nOtherViewed);

    s_bSecondActive = !s_bSecondActive;
}

bool Ready()
{
    return s_bOn && g_pGameEngine != nullptr && g_pInputDevice != nullptr;
}

// Runs fn with player two at the keys.
template <typename Fn>
void AsSecond(Fn fn)
{
    const auto bSwap = Ready() && !s_bSecondActive;

    if (bSwap)
    {
        SwapPlayers();
    }

    fn();

    if (bSwap)
    {
        SwapPlayers();
    }
}

} // namespace

bool TwoPlayer::On()
{
    return s_bOn;
}

bool TwoPlayer::Set(bool bOn)
{
    if (bOn && (g_bSessionActive != 0 || Spectator::AutopilotSetting()))
    {
        return false;
    }

    if (s_bSecondActive)
    {
        SwapPlayers();
    }

    s_bOn = bOn;

    return true;
}

void TwoPlayer::Reset()
{
    if (!Ready())
    {
        return;
    }

    if (s_bSecondActive)
    {
        SwapPlayers();
    }

    s_nSecondTeam  = (g_pGameEngine->m_nViewedSlot & 1) ^ 1;
    s_nOtherViewed = static_cast<short>(s_nSecondTeam);
    s_other        = DeviceState();

    Track(g_pGameEngine->m_nViewedSlot);
    Track(s_nOtherViewed);
}

bool TwoPlayer::IsSecondPlayers(int nSlot)
{
    return s_bOn && nSlot >= kSlotDromer0 && nSlot <= kSlotDromer3 && (nSlot & 1) == s_nSecondTeam;
}

bool TwoPlayer::PadIsSecond(SDL_JoystickID nPad)
{
    if (!s_bOn)
    {
        return false;
    }

    auto nCount = 0;

    const auto pPads = SDLPtr<SDL_JoystickID>(SDL_GetGamepads(&nCount));

    if (pPads == nullptr || nCount < 2)
    {
        return true;
    }

    return pPads.get()[0] != nPad;
}

void TwoPlayer::KeyDown(unsigned int nKey)
{
    AsSecond(
        [&]
        {
            WndProc::OnKeyDown(nKey);
        });
}

void TwoPlayer::KeyUp(unsigned int nKey)
{
    AsSecond(
        [&]
        {
            WndProc::OnKeyUp(nKey);
        });
}

void TwoPlayer::CheckSecondTimeouts()
{
    if (!Ready())
    {
        return;
    }

    AsSecond(
        []
        {
            g_pInputDevice->CheckButtonTimeouts();
        });
}

void TwoPlayer::Track(int nSlot)
{
    auto* const pObject = g_pGameEngine->m_ppSlots[nSlot];

    if (s_bOn)
    {
        g_pGameEngine->m_apTracked[nSlot & 1] = pObject;

        return;
    }

    g_pGameEngine->m_apTracked[0] = pObject;
    g_pGameEngine->m_apTracked[1] = pObject;
}

TwoPlayer::Scope::Scope(EngineObject* pObject)
{
    if (!Ready())
    {
        return;
    }

    const auto nSlot = pObject->GetSlot();

    if (nSlot < kSlotDromer0 || nSlot > kSlotDromer3)
    {
        return;
    }

    if (IsSecondPlayers(nSlot) != s_bSecondActive)
    {
        SwapPlayers();

        m_bSwapped = true;
    }
}

TwoPlayer::Scope::~Scope()
{
    if (m_bSwapped)
    {
        SwapPlayers();
    }
}
