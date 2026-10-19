#include "common/compat.h"
#include "common/clock.h"
#include <algorithm>
#include "input/keycodes.h"
#include <SDL3/SDL.h>
#include "engine/devicector.h"
#include "globals.h"
#include "keyboarddeviceobj.h"
#include "actionbinding.h"
#include "app/gameapp.h"
#include "graphics/renderer.h"
#include "inputglobals.h"
#include "engine/messagequeue.h"
#include "ui/screen.h"
#include "ui/widget.h"
#include "cheatstate.h"
#include "engine/engineobject.h"
#include "engine/objectclasses.h"
#include "engine/gameoptions.h"
#include "engine/game.h"
#include "input/input.h"
#include "app/wndproc.h"
#include "engine/gameengine.h"
#include "engine/messageids.h"
#include "planvm/decisionlog.h"
#include <memory>

// A held binding fires once its key stays down past the double-tap window; a release drops it.
// FUNCTION: HYPERX 0x0045cb10
void KeyboardDeviceObj::CheckButtonTimeouts()
{
    unsigned int nBit    = 1;
    auto         nSticky = m_nStickyMask;
    auto         nCount  = static_cast<short>(m_pBindingTable->m_nKeyCount);
    short        nKey    = 0;

    if (nSticky != 0)
    {
        do
        {
            if ((nSticky & 1) != 0)
            {
                if ((m_nHeldMask & nBit) != 0 && m_aReleaseTime[nKey] <= m_aPressTime[nKey])
                {
                    if (g_pGameEngine->m_clock.tick - m_aPressTime[nKey] > m_doubleTap)
                    {
                        auto* const pBinding = m_apStickyBinding[nKey];

                        if (pBinding != nullptr)
                        {
                            m_pEntity->DispatchSceneMessage(
                                kMsgNewInput, pBinding->m_nKey, kDecisionLogFresh, m_pEntity->GetSlotWord(), 0);
                        }

                        m_nStickyMask ^= nBit;
                        m_apStickyBinding[nKey] = nullptr;
                    }
                }
                else
                {
                    m_apStickyBinding[nKey] = nullptr;
                    m_nStickyMask ^= nBit;
                }
            }

            nSticky >>= 1;
            nBit <<= 1;
            nKey++;
        } while (nSticky != 0 && --nCount != 0);
    }
}

constexpr auto kDoubleTap  = Clock::Milliseconds{ 0xfa };
constexpr auto kRepeatStep = Clock::Milliseconds{ 0x32 };

// Each release time starts fifty milliseconds past its press, so a first press is never a double tap.
// FUNCTION: HYPERX 0x0045cc10
KeyboardDeviceObj::KeyboardDeviceObj(int nSlot, class ObjectDef* pDef)
: DeviceObject(nSlot, pDef)
{
    m_nStickyMask   = 0;
    m_nPrevHeldMask = 0;
    m_nHeldMask     = 0;

    memset(m_aPressTime, 0, sizeof(m_aPressTime));

    memset(m_aPrevPressTime, 0, sizeof(m_aPrevPressTime));

    memset(m_aReleaseTime, 0, sizeof(m_aReleaseTime));

    memset(m_aPrevReleaseTime, 0, sizeof(m_aPrevReleaseTime));

    memset(m_apStickyBinding, 0, sizeof(m_apStickyBinding));

    KeyboardDeviceObj::BuildActionBindingTable(&m_pSlots,
                                               &m_nSlots,
                                               this);

    if (m_nSlots > 0)
    {
        m_wUnused       = 0;
        m_pBindingTable = m_pSlots.get();
    }

    m_doubleTap = kDoubleTap;

    for (int nInputSlot = 0; nInputSlot < kInputSlotCount; nInputSlot++)
    {
        const auto nNow = Clock::Now();

        m_aPressTime[static_cast<short>(nInputSlot)]   = nNow;
        m_aReleaseTime[static_cast<short>(nInputSlot)] = nNow + kRepeatStep;
    }
}

constexpr auto kTableKeyCount = 8;

// An up entry whose action folds to slot zero is reported and the build keeps going.
// FUNCTION: HYPERX 0x0045cd20
char KeyboardDeviceObj::BuildActionBindingTable(std::unique_ptr<ActionBindingGroup>* ppTableOut, short* pnGroupsOut, KeyboardDeviceObj* pDevice)
{
    int anUp[kInputSlotCount];
    int anDown[kInputSlotCount];
    int nEntry;
    int nSlot;
    int nIndex;

    *ppTableOut = std::make_unique<ActionBindingGroup>();

    auto* const pTable  = ppTableOut->get();
    pTable->m_nKeyCount = kTableKeyCount;
    pTable->m_nCount    = kActionBindingCount;

    const ActionBinding* apSource[1];

    apSource[0] = g_aActionBindings;

    *pnGroupsOut = 1;

    pDevice->CommitKeyBindings(KeyboardDeviceObj::LoadBindingsFromIni(), kKeyBindingCount);

    auto* pGroup = pTable;

    for (int nGroup = 0; nGroup < *pnGroupsOut; nGroup++, pGroup++)
    {
        const auto pWork = std::make_unique_for_overwrite<ActionBinding[]>(pGroup->m_nCount);

        for (nEntry = 0; nEntry < pGroup->m_nCount; nEntry++)
        {
            pWork[nEntry] = *apSource[nGroup]++;
        }

        for (nSlot = 0; nSlot < kInputSlotCount; nSlot++)
        {
            anUp[nSlot] = 0;
        }

        for (nSlot = 0; nSlot < kInputSlotCount; nSlot++)
        {
            anDown[nSlot] = 0;
        }

        for (nEntry = 0; nEntry < pGroup->m_nCount; nEntry++)
        {
            if ((pWork[nEntry].m_nAction & kActionBindingUp) != 0)
            {
                nIndex = KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & kActionBindingMask);

                if (nIndex != 0)
                {
                    anUp[nIndex]++;
                }
                else
                {
                    // STRING: HYPERX 0x004c4458
                    g_gameApp.Error("NULL KEY UP KeyInput EVENT ENCOUNTERED IN InitKeyInput\n");
                }
            }
            else
            {
                pWork[nEntry].m_nCombine |= pWork[nEntry].m_nAction & kActionBindingMask;

                anDown[KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & kActionBindingMask)]++;
            }
        }

        for (nSlot = 0; nSlot < kInputSlotCount; nSlot++)
        {
            pGroup->m_anUp[nSlot] = anUp[nSlot];

            if (anUp[nSlot] != 0)
            {
                pGroup->m_apUp[nSlot] = std::make_unique_for_overwrite<ActionBinding[]>(anUp[nSlot]);
            }

            pGroup->m_anDown[nSlot] = anDown[nSlot];

            if (anDown[nSlot] != 0)
            {
                pGroup->m_apDown[nSlot] = std::make_unique_for_overwrite<ActionBinding[]>(anDown[nSlot]);
            }
        }

        for (nSlot = 0; nSlot < kInputSlotCount; nSlot++)
        {
            anUp[nSlot] = 0;
        }

        for (nSlot = 0; nSlot < kInputSlotCount; nSlot++)
        {
            anDown[nSlot] = 0;
        }

        for (nEntry = 0; nEntry < pGroup->m_nCount; nEntry++)
        {
            pWork[nEntry].m_nClear =
                ~(~(pWork[nEntry].m_nCombine & ~kActionBindingHighHalf) & pWork[nEntry].m_nClear);

            if ((pWork[nEntry].m_nAction & kActionBindingUp) != 0)
            {
                nIndex = KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & kActionBindingMask);

                if (nIndex != 0)
                {
                    pGroup->m_apUp[nIndex][anUp[nIndex]++] = pWork[nEntry];
                }
                else
                {
                    g_gameApp.Error("NULL KEY UP KeyInput EVENT ENCOUNTERED IN InitKeyInput\n");
                }
            }
            else
            {
                nIndex = KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & kActionBindingMask);

                pGroup->m_apDown[nIndex][anDown[nIndex]++] = pWork[nEntry];
            }
        }
    }

    return 1;
}

// Returns the one-based index of the highest set bit, or zero for an empty mask.

// FUNCTION: HYPERX 0x0045cff0
int KeyboardDeviceObj::GetActionBitIndex(unsigned int nMask)
{
    if ((nMask & 0xff00) == 0)
    {
        if ((nMask & 0xf0) != 0)
        {
            if ((nMask & 0xc0) != 0)
            {
                if ((nMask & 0x80) != 0)
                {
                    return 8;
                }

                return 7;
            }

            if ((nMask & 0x20) != 0)
            {
                return 6;
            }

            return 5;
        }

        if ((nMask & 0x0f) != 0)
        {
            if ((nMask & 0x0c) != 0)
            {
                if ((nMask & 0x08) != 0)
                {
                    return 4;
                }

                return 3;
            }

            if ((nMask & 0x02) != 0)
            {
                return 2;
            }

            return 1;
        }

        return 0;
    }

    if ((nMask & 0xf000) != 0)
    {
        if ((nMask & 0xc000) != 0)
        {
            if ((nMask & 0x8000) != 0)
            {
                return 16;
            }

            return 15;
        }

        if ((nMask & 0x2000) != 0)
        {
            return 14;
        }

        return 13;
    }

    if ((nMask & 0x0f00) != 0)
    {
        if ((nMask & 0x0c00) != 0)
        {
            if ((nMask & 0x0800) != 0)
            {
                return 12;
            }

            return 11;
        }

        if ((nMask & 0x0200) != 0)
        {
            return 10;
        }

        return 9;
    }

    return 0;
}

// Each record's copy of the keys is the identity with four and five exchanged, at most ten.
// FUNCTION: HYPERX 0x0045d190
void KeyboardDeviceObj::CommitKeyBindings(const unsigned int* pnKeys, int nCount)
{
    int nLimit;
    int nIndex;
    int nWrite;

    auto*       pRecord = m_pSlots.get();
    auto* const pEnd    = pRecord + m_nSlots;

    while (pRecord < pEnd)
    {
        nLimit = (std::min)(kKeyBindingCount, nCount);

        for (nIndex = 0; nIndex < nLimit; nIndex++)
        {
            g_anKeyBindingDefaults[nIndex] = pnKeys[nIndex];

            if (nIndex < pRecord->m_nKeyCount)
            {
                pRecord->m_anKey[nIndex] = static_cast<unsigned short>(pnKeys[g_anKeyBindingOrder[nIndex]]);
            }
        }

        pRecord++;
    }

    m_nHeldMask = 0;

    nWrite = (std::min)(kKeyBindingCount, nCount);

    for (nIndex = 0; nIndex < nWrite; nIndex++)
    {
        // STRING: HYPERX 0x004c41d8
        g_pGameOptions->WriteInt("Keys", g_apszKeyBindingNames[nIndex], pnKeys[nIndex]);
    }
}

// FUNCTION: HYPERX 0x0045d270
unsigned int* KeyboardDeviceObj::GetDefaultKeyBindings() const
{
    return g_anKeyBindingDefaults;
}

// FUNCTION: HYPERX 0x0045d280
unsigned int* KeyboardDeviceObj::LoadBindingsFromIni()
{
    for (int nKey = 0; nKey < kKeyBindingCount; nKey++)
    {
        g_anKeyBindings[nKey] = g_pGameOptions
                                    // STRING: HYPERX 0x004c41d8
                                    ->ReadInt("Keys", g_apszKeyBindingNames[nKey], g_anKeyBindingDefaults[nKey]);
    }

    return g_anKeyBindings;
}

// FUNCTION: HYPERX 0x0045d2c0
int KeyboardDeviceObj::OnButtonDown(unsigned int nButton)
{
    if (nButton < static_cast<unsigned int>(m_pBindingTable->m_nKeyCount) && (m_nHeldMask & (1 << nButton)) != 0)
    {
        m_aReleaseTime[nButton] = m_aPressTime[nButton] + Clock::Milliseconds{ 1 };

        return HandleKeyDown(m_pBindingTable->m_anKey[nButton]);
    }

    return -1;
}

// Both key handlers copy m_nHeldMask into m_nPrevHeldMask, and nothing reads the copy.

// FUNCTION: HYPERX 0x0045d320
int KeyboardDeviceObj::HandleKeyDown(unsigned int nKey)
{
    auto nIndex        = 0;
    m_nPrevHeldMask    = m_nHeldMask;
    const auto nCount  = static_cast<short>(m_pBindingTable->m_nKeyCount);
    auto       nResult = -1;

    for (; nIndex < nCount; nIndex++)
    {
        if (nKey == m_pBindingTable->m_anKey[nIndex])
        {
            unsigned int        nBit;
            unsigned int        nMask;
            unsigned int        nStick;
            ActionBinding*      pBinding;
            int                 nBindingCount;
            int                 nBudget;
            int                 i;
            Clock::Milliseconds nNow;
            int                 nActionIndex;

            nBit    = 1 << nIndex;
            nResult = 0;

            if (m_aReleaseTime[nIndex] >= m_aPressTime[nIndex])
            {
                m_aPrevPressTime[static_cast<short>(nIndex)] = m_aPressTime[static_cast<short>(nIndex)];
                nNow                                         = Clock::Now();
                m_aPressTime[static_cast<short>(nIndex)]     = nNow;

                if (nNow - m_aReleaseTime[static_cast<short>(nIndex)] < m_doubleTap && m_aReleaseTime[static_cast<short>(nIndex)] - m_aPrevPressTime[static_cast<short>(nIndex)] < m_doubleTap)
                {
                    nBit = kActionBindingDoubleTap << nIndex;
                }

                nMask = nBit & kActionBindingMask;
                m_nHeldMask |= nMask;

                nActionIndex = KeyboardDeviceObj::GetActionBitIndex(nMask);
                pBinding     = m_pBindingTable->m_apDown[nActionIndex].get();

                if (pBinding != nullptr)
                {
                    nBudget       = ((nBit & kActionBindingHighHalf) != 0) + 1;
                    nBindingCount = m_pBindingTable->m_anDown[nActionIndex];

                    for (i = 0; i < nBindingCount; i++, pBinding++)
                    {
                        if (pBinding->m_nCombine == (pBinding->m_nClear & m_nHeldMask))
                        {
                            if ((pBinding->m_nAction & ACTION_BINDING_HELD) != 0)
                            {
                                nStick                    = nMask;
                                m_apStickyBinding[nIndex] = pBinding;
                                m_nStickyMask |= nStick;
                                nBudget--;

                                if (nBudget == 0)
                                {
                                    break;
                                }
                            }
                            else
                            {
                                if ((pBinding->m_nAction & kActionBindingHighHalf) == 0 || (nBit & kActionBindingHighHalf) != 0)
                                {
                                    m_pEntity->DispatchSceneMessage(
                                        kMsgNewInput, pBinding->m_nKey, kDecisionLogFresh, m_pEntity->GetSlotWord(), 0);
                                    nResult = 1;
                                    nBudget--;

                                    if (nBudget == 0)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            break;
        }
    }

    return nResult;
}

// FUNCTION: HYPERX 0x0045d4c0
int KeyboardDeviceObj::HandleKeyUp(unsigned int nKey)
{
    auto nIndex = 0;

    m_nPrevHeldMask    = m_nHeldMask;
    const auto nCount  = static_cast<short>(m_pBindingTable->m_nKeyCount);
    auto       nResult = -1;

    for (; nIndex < nCount; nIndex++)
    {
        if (nKey == m_pBindingTable->m_anKey[nIndex])
        {
            unsigned int        nBit;
            unsigned int        nClearMask;
            unsigned int        nSticky;
            unsigned int        nHeld;
            ActionBinding*      pBinding;
            int                 nBindingCount;
            int                 nBudget;
            int                 i;
            Clock::Milliseconds nNow;
            Clock::Milliseconds nElapsed;
            int                 nActionIndex;

            nResult                                        = 0;
            m_aPrevReleaseTime[static_cast<short>(nIndex)] = m_aReleaseTime[static_cast<short>(nIndex)];
            nNow                                           = Clock::Now();
            m_aReleaseTime[static_cast<short>(nIndex)]     = nNow;
            nElapsed                                       = nNow - m_aPressTime[static_cast<short>(nIndex)];

            if (nElapsed < m_doubleTap && m_aPressTime[static_cast<short>(nIndex)] - m_aPrevReleaseTime[static_cast<short>(nIndex)] < m_doubleTap)
            {
                nBit = (kActionBindingDoubleTap << nIndex) | kActionBindingUp | kActionBindingQuick;
            }
            else
            {
                // Not a double tap; QUICK is still set when the release came inside the window.
                nBit = (1 << nIndex) | kActionBindingUp;

                if (nElapsed < m_doubleTap)
                {
                    nBit |= kActionBindingUp | kActionBindingQuick;
                }
            }

            nHeld                     = m_nHeldMask;
            nClearMask                = ~(nBit & kActionBindingMask);
            nSticky                   = m_nStickyMask;
            m_nHeldMask               = nHeld & nClearMask;
            m_nStickyMask             = nSticky & nClearMask;
            m_apStickyBinding[nIndex] = nullptr;

            nBudget      = ((nBit & kActionBindingHighHalf) != 0) + 1;
            nActionIndex = KeyboardDeviceObj::GetActionBitIndex(nBit & kActionBindingMask);
            pBinding     = m_pBindingTable->m_apUp[nActionIndex].get();

            if (pBinding != nullptr)
            {
                nBindingCount = m_pBindingTable->m_anUp[nActionIndex];

                for (i = 0; i < nBindingCount; i++, pBinding++)
                {
                    if ((pBinding->m_nAction == static_cast<unsigned int>(nActionIndex) || pBinding->m_nAction == (nBit & (kActionBindingUp | kActionBindingQuick | kActionBindingMask)) || pBinding->m_nAction == nBit) && pBinding->m_nCombine == (pBinding->m_nClear & m_nHeldMask))
                    {
                        m_pEntity->DispatchSceneMessage(
                            kMsgNewInput, pBinding->m_nKey, kDecisionLogFresh, m_pEntity->GetSlotWord(), 0);
                        nResult = 1;
                        nBudget--;

                        if (nBudget != 0)
                        {
                            break;
                        }
                    }
                }
            }

            break;
        }
    }

    return nResult;
}

const char* g_apszKeyBindingNames[kKeyBindingCount] = {
    "Forward", "Stop", "RotateLeft", "RotateRight", "Attack", "SpeedBurst", "Shoot", "Pass", "QuickShot", "Jump"
};

unsigned int g_anKeyBindingDefaults[kKeyBindingCount] = {
    Key::kUp, Key::kDown, Key::kLeft, Key::kRight, 'S', 'D', Key::kSpace, 'C', 'F', 'E'
};

int g_anKeyBindingOrder[kKeyBindingCount] = { 0, 1, 2, 3, 5, 4, 6, 7, 8, 9 };

unsigned int g_anKeyBindings[kKeyBindingCount] = {
    Key::kUp, Key::kDown, Key::kLeft, Key::kRight, 'S', 'D', Key::kSpace, 'C', 'F', 'E'
};
