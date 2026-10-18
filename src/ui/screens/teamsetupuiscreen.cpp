#include "common/compat.h"
#include "ui/buttonwidget.h"
#include "globals.h"
#include "ui/widget.h"
#include "ui/pausemenu.h"
#include "ui/screen.h"
class TeamSetupUIScreen;
#include "ui/torchbitmaps.h"
#include "engine/gameengine.h"
#include "ui/saveobject.h"
#include "ui/screens/leaguesetupuiscreen.h"

// Team one is a slot: the team is the quotient by four and the remainder a position.
// FUNCTION: HYPERX 0x00458ed0
void TeamSetupUIScreen::StepHomeSelection(int nDirection)
{
    short nTeam;

    auto nLocked    = 0;
    auto nRemaining = m_nArenaCount * 3;

    BuildLockedFlagsArray();

    if (m_nLockedCount != 0)
    {
        nLocked = m_pLockedFlags[m_nTeamSlot / 4];
    }

    if (nDirection == 0)
    {
        while (nLocked != 0 && nRemaining != 0)
        {
            m_nTeamSlot--;

            if (m_nTeamSlot < 0)
            {
                m_nTeamSlot = m_nTeamSlotCount - 1;
            }

            nTeam = g_pGameEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(m_nTeamSlot / 4), 0);

            m_nTeamSlot = m_nTeamSlot % 4 + nTeam * 4;

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[m_nTeamSlot / 4];
            }

            nRemaining--;
        }
    }
    else
    {
        while (nLocked != 0 && nRemaining != 0)
        {
            m_nTeamSlot++;

            if (m_nTeamSlot >= m_nTeamSlotCount)
            {
                m_nTeamSlot = 0;
            }

            nTeam = g_pGameEngine->m_pSaveObject->WrapToNextValidTeam(static_cast<short>(m_nTeamSlot / 4), 1);

            m_nTeamSlot = m_nTeamSlot % 4 + nTeam * 4;

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[m_nTeamSlot / 4];
            }

            nRemaining--;
        }
    }
}

// FUNCTION: HYPERX 0x00459050
void TeamSetupUIScreen::StepArenaSelection(int nDirection)
{
    int nTeam;

    auto nLocked    = 0;
    auto nRemaining = m_nArenaCount;

    BuildLockedFlagsArray();

    if (m_nLockedCount != 0)
    {
        nLocked = m_pLockedFlags[m_nArena];
    }

    if (nDirection == 0)
    {
        while (nLocked != 0 && nRemaining != 0)
        {
            m_nArena--;

            if (m_nArena < 0)
            {
                m_nArena = m_nArenaCount - 1;
            }

            nTeam = g_pGameEngine->m_pSaveObject
                        ->WrapToNextValidTeam(static_cast<short>(m_nArena), 0);

            m_nArena = nTeam;

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[nTeam];
            }

            nRemaining--;
        }
    }
    else
    {
        while (nLocked != 0 && nRemaining != 0)
        {
            m_nArena++;

            if (m_nArena >= m_nArenaCount)
            {
                m_nArena = 0;
            }

            nTeam = g_pGameEngine->m_pSaveObject
                        ->WrapToNextValidTeam(static_cast<short>(m_nArena), nDirection);

            m_nArena = nTeam;

            nLocked = 0;
            if (m_nLockedCount != 0)
            {
                nLocked = m_pLockedFlags[nTeam];
            }

            nRemaining--;
        }
    }
}
