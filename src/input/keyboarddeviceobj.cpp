#include "common/compat.h"
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

// A held binding fires once its key stays down past the double-tap window; a release drops it.
// FUNCTION: HYPERX 0x0045cb10
void KeyboardDeviceObj::CheckButtonTimeouts()
{
    unsigned int nBit    = 1;
    unsigned int nSticky = m_nStickyMask;
    short        nCount  = (short)m_pBindingTable->m_nKeyCount;
    short        nKey    = 0;

    if (nSticky != 0)
    {
        do
        {
            if ((nSticky & 1) != 0)
            {
                if ((m_nHeldMask & nBit) != 0 && (unsigned int)m_aReleaseTime[nKey] <= (unsigned int)m_aPressTime[nKey])
                {
                    if (g_pGameEngine->m_clock.nTickMs - m_aPressTime[nKey] > m_nDoubleTapMs)
                    {
                        ActionBinding* const pBinding = m_apStickyBinding[nKey];

                        if (pBinding != 0)
                        {
                            m_pEntity->DispatchSceneMessage(
                                MSG_NEW_INPUT, pBinding->m_nKey, DECISION_LOG_FRESH, m_pEntity->GetSlotWord(), 0);
                        }

                        m_nStickyMask ^= nBit;
                        m_apStickyBinding[nKey] = 0;
                    }
                }
                else
                {
                    m_apStickyBinding[nKey] = 0;
                    m_nStickyMask ^= nBit;
                }
            }

            nSticky >>= 1;
            nBit <<= 1;
            nKey++;
        } while (nSticky != 0 && --nCount != 0);
    }
}

#define DOUBLE_TAP_MS  0xfa
#define REPEAT_STEP_MS 0x32

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
        m_pBindingTable = m_pSlots;
    }

    m_nDoubleTapMs = DOUBLE_TAP_MS;

    for (int nInputSlot = 0; nInputSlot < INPUT_SLOT_COUNT; nInputSlot++)
    {
        const unsigned int dwNow = (unsigned int)SDL_GetTicks();

        m_aPressTime[(short)nInputSlot]   = dwNow;
        m_aReleaseTime[(short)nInputSlot] = dwNow + REPEAT_STEP_MS;
    }
}

#define TABLE_KEY_COUNT 8

// An up entry whose action folds to slot zero is reported and the build keeps going.
// FUNCTION: HYPERX 0x0045cd20
char KeyboardDeviceObj::BuildActionBindingTable(ActionBindingGroup** ppTableOut, short* pnGroupsOut, KeyboardDeviceObj* pDevice)
{
    int anUp[INPUT_SLOT_COUNT];
    int anDown[INPUT_SLOT_COUNT];
    int nEntry;
    int nSlot;
    int nIndex;

    ActionBindingGroup* const pTable = new ActionBindingGroup;

    *ppTableOut         = pTable;
    pTable->m_nKeyCount = TABLE_KEY_COUNT;
    pTable->m_nCount    = ACTION_BINDING_COUNT;

    const ActionBinding* apSource[1];

    apSource[0] = g_aActionBindings;

    *pnGroupsOut = 1;

    pDevice->CommitKeyBindings(KeyboardDeviceObj::LoadBindingsFromIni(), KEY_BINDING_COUNT);

    ActionBindingGroup* pGroup = pTable;

    for (int nGroup = 0; nGroup < *pnGroupsOut; nGroup++, pGroup++)
    {
        ActionBinding* pWork = new ActionBinding[pGroup->m_nCount];

        for (nEntry = 0; nEntry < pGroup->m_nCount; nEntry++)
        {
            pWork[nEntry] = *apSource[nGroup]++;
        }

        for (nSlot = 0; nSlot < INPUT_SLOT_COUNT; nSlot++)
        {
            anUp[nSlot] = 0;
        }

        for (nSlot = 0; nSlot < INPUT_SLOT_COUNT; nSlot++)
        {
            anDown[nSlot] = 0;
        }

        for (nEntry = 0; nEntry < pGroup->m_nCount; nEntry++)
        {
            if ((pWork[nEntry].m_nAction & ACTION_BINDING_UP) != 0)
            {
                nIndex = KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & ACTION_BINDING_MASK);

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
                pWork[nEntry].m_nCombine |= pWork[nEntry].m_nAction & ACTION_BINDING_MASK;

                anDown[KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & ACTION_BINDING_MASK)]++;
            }
        }

        memset(pGroup->m_apUp, 0, sizeof(pGroup->m_apUp));
        memset(pGroup->m_apDown, 0, sizeof(pGroup->m_apDown));

        for (nSlot = 0; nSlot < INPUT_SLOT_COUNT; nSlot++)
        {
            pGroup->m_anUp[nSlot] = anUp[nSlot];

            if (anUp[nSlot] != 0)
            {
                pGroup->m_apUp[nSlot] = new ActionBinding[anUp[nSlot]];
            }

            pGroup->m_anDown[nSlot] = anDown[nSlot];

            if (anDown[nSlot] != 0)
            {
                pGroup->m_apDown[nSlot] = new ActionBinding[anDown[nSlot]];
            }
        }

        for (nSlot = 0; nSlot < INPUT_SLOT_COUNT; nSlot++)
        {
            anUp[nSlot] = 0;
        }

        for (nSlot = 0; nSlot < INPUT_SLOT_COUNT; nSlot++)
        {
            anDown[nSlot] = 0;
        }

        for (nEntry = 0; nEntry < pGroup->m_nCount; nEntry++)
        {
            pWork[nEntry].m_nClear =
                ~(~(pWork[nEntry].m_nCombine & ~ACTION_BINDING_HIGH_HALF) & pWork[nEntry].m_nClear);

            if ((pWork[nEntry].m_nAction & ACTION_BINDING_UP) != 0)
            {
                nIndex = KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & ACTION_BINDING_MASK);

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
                nIndex = KeyboardDeviceObj::GetActionBitIndex(pWork[nEntry].m_nAction & ACTION_BINDING_MASK);

                pGroup->m_apDown[nIndex][anDown[nIndex]++] = pWork[nEntry];
            }
        }

        delete[] pWork;
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

// Each slot owns its up and down binding arrays.
// FUNCTION: HYPERX 0x0045d0b0
KeyboardDeviceObj::~KeyboardDeviceObj()
{
    ActionBindingGroup* const pSlots = m_pSlots;

    while (m_nSlots-- != 0)
    {
        for (int nKey = 0; nKey < INPUT_SLOT_COUNT; nKey++)
        {
            if (pSlots[m_nSlots].m_apUp[nKey] != 0)
            {
                delete[] pSlots[m_nSlots].m_apUp[nKey];
            }

            if (pSlots[m_nSlots].m_apDown[nKey] != 0)
            {
                delete[] pSlots[m_nSlots].m_apDown[nKey];
            }
        }
    }

    delete[] m_pSlots;
}

// Each record's copy of the keys is the identity with four and five exchanged, at most ten.
// FUNCTION: HYPERX 0x0045d190
void KeyboardDeviceObj::CommitKeyBindings(const unsigned int* pnKeys, int nCount)
{
    int nLimit;
    int nIndex;
    int nWrite;

    ActionBindingGroup*       pRecord = m_pSlots;
    ActionBindingGroup* const pEnd    = pRecord + m_nSlots;

    while (pRecord < pEnd)
    {
        nLimit = (std::min)(KEY_BINDING_COUNT, nCount);

        for (nIndex = 0; nIndex < nLimit; nIndex++)
        {
            g_anKeyBindingDefaults[nIndex] = pnKeys[nIndex];

            if (nIndex < pRecord->m_nKeyCount)
            {
                pRecord->m_anKey[nIndex] = (unsigned short)pnKeys[g_anKeyBindingOrder[nIndex]];
            }
        }

        pRecord++;
    }

    m_nHeldMask = 0;

    nWrite = (std::min)(KEY_BINDING_COUNT, nCount);

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
unsigned int* KeyboardDeviceObj::LoadBindingsFromIni(void)
{
    for (int nKey = 0; nKey < KEY_BINDING_COUNT; nKey++)
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
    if (nButton < (unsigned int)m_pBindingTable->m_nKeyCount && (m_nHeldMask & (1 << nButton)) != 0)
    {
        m_aReleaseTime[nButton] = m_aPressTime[nButton] + 1;

        return HandleKeyDown(m_pBindingTable->m_anKey[nButton]);
    }

    return -1;
}

// Both key handlers copy m_nHeldMask into m_nPrevHeldMask, and nothing reads the copy.

// FUNCTION: HYPERX 0x0045d320
int KeyboardDeviceObj::HandleKeyDown(unsigned int nKey)
{
    int nIndex          = 0;
    m_nPrevHeldMask     = m_nHeldMask;
    const short nCount  = (short)m_pBindingTable->m_nKeyCount;
    int         nResult = -1;

    for (; nIndex < nCount; nIndex++)
    {
        if (nKey == m_pBindingTable->m_anKey[nIndex])
        {
            unsigned int   nBit;
            unsigned int   nMask;
            unsigned int   nStick;
            ActionBinding* pBinding;
            int            nBindingCount;
            int            nBudget;
            int            i;
            unsigned int   dwNow;
            int            nActionIndex;

            nBit    = 1 << nIndex;
            nResult = 0;

            if ((unsigned int)m_aReleaseTime[nIndex] >= (unsigned int)m_aPressTime[nIndex])
            {
                m_aPrevPressTime[(short)nIndex] = m_aPressTime[(short)nIndex];
                dwNow                           = (unsigned int)SDL_GetTicks();
                m_aPressTime[(short)nIndex]     = dwNow;

                if (dwNow - (unsigned int)m_aReleaseTime[(short)nIndex] < m_nDoubleTapMs && (unsigned int)(m_aReleaseTime[(short)nIndex] - m_aPrevPressTime[(short)nIndex]) < m_nDoubleTapMs)
                {
                    nBit = ACTION_BINDING_DOUBLE_TAP << nIndex;
                }

                nMask = nBit & ACTION_BINDING_MASK;
                m_nHeldMask |= nMask;

                nActionIndex = KeyboardDeviceObj::GetActionBitIndex(nMask);
                pBinding     = m_pBindingTable->m_apDown[nActionIndex];

                if (pBinding != 0)
                {
                    nBudget       = ((nBit & ACTION_BINDING_HIGH_HALF) != 0) + 1;
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
                                if ((pBinding->m_nAction & ACTION_BINDING_HIGH_HALF) == 0 || (nBit & ACTION_BINDING_HIGH_HALF) != 0)
                                {
                                    m_pEntity->DispatchSceneMessage(
                                        MSG_NEW_INPUT, pBinding->m_nKey, DECISION_LOG_FRESH, m_pEntity->GetSlotWord(), 0);
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
    int nIndex = 0;

    m_nPrevHeldMask     = m_nHeldMask;
    const short nCount  = (short)m_pBindingTable->m_nKeyCount;
    int         nResult = -1;

    for (; nIndex < nCount; nIndex++)
    {
        if (nKey == m_pBindingTable->m_anKey[nIndex])
        {
            unsigned int   nBit;
            unsigned int   nClearMask;
            unsigned int   nSticky;
            unsigned int   nHeld;
            ActionBinding* pBinding;
            int            nBindingCount;
            int            nBudget;
            int            i;
            unsigned int   dwNow;
            unsigned int   nElapsed;
            int            nActionIndex;

            nResult                           = 0;
            m_aPrevReleaseTime[(short)nIndex] = m_aReleaseTime[(short)nIndex];
            dwNow                             = (unsigned int)SDL_GetTicks();
            m_aReleaseTime[(short)nIndex]     = dwNow;
            nElapsed                          = dwNow - (unsigned int)m_aPressTime[(short)nIndex];

            if (nElapsed < m_nDoubleTapMs && (unsigned int)(m_aPressTime[(short)nIndex] - m_aPrevReleaseTime[(short)nIndex]) < m_nDoubleTapMs)
            {
                nBit = (ACTION_BINDING_DOUBLE_TAP << nIndex) | ACTION_BINDING_UP | ACTION_BINDING_QUICK;
            }
            else
            {
                // Not a double tap; QUICK is still set when the release came inside the window.
                nBit = (1 << nIndex) | ACTION_BINDING_UP;

                if (nElapsed < m_nDoubleTapMs)
                {
                    nBit |= ACTION_BINDING_UP | ACTION_BINDING_QUICK;
                }
            }

            nHeld                     = m_nHeldMask;
            nClearMask                = ~(nBit & ACTION_BINDING_MASK);
            nSticky                   = m_nStickyMask;
            m_nHeldMask               = nHeld & nClearMask;
            m_nStickyMask             = nSticky & nClearMask;
            m_apStickyBinding[nIndex] = 0;

            nBudget      = ((nBit & ACTION_BINDING_HIGH_HALF) != 0) + 1;
            nActionIndex = KeyboardDeviceObj::GetActionBitIndex(nBit & ACTION_BINDING_MASK);
            pBinding     = m_pBindingTable->m_apUp[nActionIndex];

            if (pBinding != 0)
            {
                nBindingCount = m_pBindingTable->m_anUp[nActionIndex];

                for (i = 0; i < nBindingCount; i++, pBinding++)
                {
                    if ((pBinding->m_nAction == (unsigned int)nActionIndex || pBinding->m_nAction == (nBit & (ACTION_BINDING_UP | ACTION_BINDING_QUICK | ACTION_BINDING_MASK)) || pBinding->m_nAction == nBit) && pBinding->m_nCombine == (pBinding->m_nClear & m_nHeldMask))
                    {
                        m_pEntity->DispatchSceneMessage(
                            MSG_NEW_INPUT, pBinding->m_nKey, DECISION_LOG_FRESH, m_pEntity->GetSlotWord(), 0);
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

const char* g_apszKeyBindingNames[KEY_BINDING_COUNT] = {
    "Forward", "Stop", "RotateLeft", "RotateRight", "Attack", "SpeedBurst", "Shoot", "Pass", "QuickShot", "Jump"
};

unsigned int g_anKeyBindingDefaults[KEY_BINDING_COUNT] = {
    Key::UP, Key::DOWN, Key::LEFT, Key::RIGHT, 'S', 'D', Key::SPACE, 'C', 'F', 'E'
};

int g_anKeyBindingOrder[KEY_BINDING_COUNT] = { 0, 1, 2, 3, 5, 4, 6, 7, 8, 9 };

unsigned int g_anKeyBindings[KEY_BINDING_COUNT] = {
    Key::UP, Key::DOWN, Key::LEFT, Key::RIGHT, 'S', 'D', Key::SPACE, 'C', 'F', 'E'
};
