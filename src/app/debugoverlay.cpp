#ifdef HYPERX_V100
#include "common/compat.h"
#include <stdio.h>
#include <string.h>
#include "globals.h"
#include "graphics/graphicsglobals.h"
#include "engine/gameengine.h"
#include "engine/playerobject.h"
#include "app/debugoverlay.h"

#define DEBUG_LOG_LINES      10
#define DEBUG_LOG_LINE_BYTES 0x96
#define DEBUG_LOG_PREFIX     6
#define DEBUG_LOG_SEQ_MAX    9999
#define DEBUG_TEXT_STACK     0x28
#define DEBUG_BATCH_STACK    0xf

// Not a colour GetNearestColor answers, so the first batch record always sets the colour.
#define DEBUG_NO_COLOUR 999999

// TODO: Size guessed; the next known global is not near it.
#define DEBUG_TEXT_BYTES 0x100

// GLOBAL: HYPERX100 0x005131f0
char g_bShowDromerEnergy = 1;

// GLOBAL: HYPERX100 0x005131e4
char g_bShowPolyCounts;

// GLOBAL: HYPERX100 0x005131ec
char g_bShowLogA;

// GLOBAL: HYPERX100 0x005131e8
char g_bShowLogB;

// GLOBAL: HYPERX100 0x00523410
char g_szDebugText[DEBUG_TEXT_BYTES];

// TODO: The software renderer's counters, presumably; nothing in v1.0.0.1 keeps them.
// GLOBAL: HYPERX100 0x005122bc
unsigned int g_nPolysTotal;

// GLOBAL: HYPERX100 0x005122c0
unsigned int g_nPolysInView;

// GLOBAL: HYPERX100 0x005122c4
unsigned int g_nPolysDrawn;

// GLOBAL: HYPERX100 0x005122c8
unsigned int g_nPolysBackFace;

// GLOBAL: HYPERX100 0x005122cc
unsigned int g_nPolysClipped;

// GLOBAL: HYPERX100 0x005122d0
unsigned int g_nPolysClippedOut;

// GLOBAL: HYPERX100 0x005122d4
unsigned int g_nPolysClippedIn;

// GLOBAL: HYPERX100 0x005253c0
char g_aszLogA[DEBUG_LOG_LINES][DEBUG_LOG_LINE_BYTES];

// GLOBAL: HYPERX100 0x00525ae0
UINT g_anLogALength[DEBUG_LOG_LINES];

// GLOBAL: HYPERX100 0x00517ad0
short g_nLogACount;

// GLOBAL: HYPERX100 0x00517ad4
short g_nLogAHead;

// GLOBAL: HYPERX100 0x00517ad8
short g_nLogASequence;

// GLOBAL: HYPERX100 0x00517b20
short g_nLogAX = 10;

// TODO: Nothing in v1.0.0.0 adds to the second log.
// GLOBAL: HYPERX100 0x00524de0
char g_aszLogB[DEBUG_LOG_LINES][DEBUG_LOG_LINE_BYTES];

// GLOBAL: HYPERX100 0x00525ab0
UINT g_anLogBLength[DEBUG_LOG_LINES];

// GLOBAL: HYPERX100 0x00517acc
short g_nLogBCount;

// GLOBAL: HYPERX100 0x00517b1c
short g_nLogBX = 300;

// GLOBAL: HYPERX100 0x005259c8
TextOutRecord* g_apTextStack[DEBUG_TEXT_STACK];

// GLOBAL: HYPERX100 0x00517adc
short g_nTextStackDepth;

// GLOBAL: HYPERX100 0x00525a70
TextOutRecord* g_apBatchStack[DEBUG_BATCH_STACK];

// GLOBAL: HYPERX100 0x005259a8
short g_anBatchCount[DEBUG_BATCH_STACK];

// GLOBAL: HYPERX100 0x00517ae0
short g_nBatchStackDepth;

// TODO: The screen size globals are matched to the back buffer's by use.
// FUNCTION: HYPERX100 0x00475ed0
void __cdecl DebugOverlay::DrawDromerEnergy(HDC hDC)
{
    if (g_bShowDromerEnergy == 0)
    {
        return;
    }

    const int      nY      = (short)((short)g_nBackBufferHeight - 20);
    EngineObject** ppSlots = g_pGameEngine->m_ppSlots;

    UINT nLength = sprintf(g_szDebugText,
                           "Dr0 E: %2.1f, Dr2 E: %2.1f",
                           ((PlayerObject*)ppSlots[0])->m_stats.m_aStats[0],
                           ((PlayerObject*)ppSlots[2])->m_stats.m_aStats[0]);

    ExtTextOutA(hDC, 10, nY, 0, 0, g_szDebugText, nLength, 0);

    ppSlots = g_pGameEngine->m_ppSlots;

    nLength = sprintf(g_szDebugText,
                      "Dr1 E: %2.1f, Dr3 E: %2.1f",
                      ((PlayerObject*)ppSlots[1])->m_stats.m_aStats[0],
                      ((PlayerObject*)ppSlots[3])->m_stats.m_aStats[0]);

    ExtTextOutA(hDC, g_nBackBufferWidth - nLength * 8 - 10, nY, 0, 0, g_szDebugText, nLength, 0);
}

// The orientation name is drawn ten characters long, which cuts it short.
// FUNCTION: HYPERX100 0x00475fe0
void __cdecl DebugOverlay::DrawPolyCounts(HDC hDC)
{
    if (g_bShowPolyCounts == 0)
    {
        return;
    }

    ExtTextOutA(hDC, 100, 0, 0, 0, g_pszDibOrientationName, 10, 0);

    UINT nLength = sprintf(g_szDebugText, "Polys: Total %u Drawn %u inView %u BackFace %u", g_nPolysTotal, g_nPolysDrawn, g_nPolysInView, g_nPolysBackFace);

    ExtTextOutA(hDC, 210, 0, 0, 0, g_szDebugText, nLength, 0);

    nLength = sprintf(g_szDebugText, "Polys: Clipped %u ClippedOut %u ClippedIn %u", g_nPolysClipped, g_nPolysClippedOut, g_nPolysClippedIn);

    ExtTextOutA(hDC, 210, 20, 0, 0, g_szDebugText, nLength, 0);
}

// FUNCTION: HYPERX100 0x00487270
void __cdecl DebugOverlay::DrawLogs(HDC hDC)
{
    short nLine;

    if (g_bShowLogA != 0)
    {
        for (nLine = 1; nLine <= g_nLogACount; nLine++)
        {
            ExtTextOutA(hDC, g_nLogAX, (nLine * 4 + 8) * 5, 0, 0, g_aszLogA[nLine - 1], g_anLogALength[nLine - 1], 0);
        }
    }

    if (g_bShowLogB != 0)
    {
        for (nLine = 1; nLine <= g_nLogBCount; nLine++)
        {
            ExtTextOutA(hDC, g_nLogBX, (nLine * 4 + 8) * 5, 0, 0, g_aszLogB[nLine - 1], g_anLogBLength[nLine - 1], 0);
        }
    }
}

// FUNCTION: HYPERX100 0x00487380
void __cdecl DebugOverlay::AddLogLine(const char* pszText, short nLength)
{
    g_nLogASequence++;

    if (g_nLogASequence > DEBUG_LOG_SEQ_MAX)
    {
        g_nLogASequence = 1;
    }

    // STRING: HYPERX100 0x00517b24
    sprintf(g_aszLogA[g_nLogAHead], "%4d: ", g_nLogASequence);

    const UINT nLineLength = nLength + DEBUG_LOG_PREFIX;

    g_anLogALength[g_nLogAHead] = nLineLength;

    memcpy(&g_aszLogA[g_nLogAHead][DEBUG_LOG_PREFIX], pszText, nLineLength);

    g_nLogAHead++;

    if (g_nLogAHead == DEBUG_LOG_LINES)
    {
        g_nLogAHead = 0;
    }

    if (g_nLogACount < DEBUG_LOG_LINES)
    {
        g_nLogACount++;
    }
}

// FUNCTION: HYPERX100 0x004874e0
TextOutRecord* __cdecl DebugOverlay::PopText(void)
{
    if (g_nTextStackDepth - 1 < 0)
    {
        return 0;
    }

    g_nTextStackDepth--;

    return g_apTextStack[g_nTextStackDepth];
}

// FUNCTION: HYPERX100 0x00487510
char __cdecl DebugOverlay::PushText(TextOutRecord* pRecord)
{
    if (g_nTextStackDepth == DEBUG_TEXT_STACK)
    {
        return 0;
    }

    g_apTextStack[g_nTextStackDepth++] = pRecord;

    return 1;
}

// FUNCTION: HYPERX100 0x00487550
char __cdecl DebugOverlay::PopTextBatch(TextOutRecord** ppRecords, short* pnCount)
{
    if (g_nBatchStackDepth - 1 < 0)
    {
        return 0;
    }

    g_nBatchStackDepth--;

    *ppRecords = g_apBatchStack[g_nBatchStackDepth];
    *pnCount   = g_anBatchCount[g_nBatchStackDepth];

    return 1;
}

// FUNCTION: HYPERX100 0x004875b0
char __cdecl DebugOverlay::PushTextBatch(TextOutRecord* pRecords, short nCount)
{
    if (g_nBatchStackDepth == DEBUG_BATCH_STACK)
    {
        return 0;
    }

    g_anBatchCount[g_nBatchStackDepth] = nCount;
    g_apBatchStack[g_nBatchStackDepth] = pRecords;

    g_nBatchStackDepth++;

    return 1;
}

// FUNCTION: HYPERX100 0x00487600
void __cdecl DebugOverlay::DrawQueuedText(HDC hDC)
{
    COLORREF       clrLast = 0;
    TextOutRecord* pRecord;

    while ((pRecord = DebugOverlay::PopText()) != 0)
    {
        if (pRecord->clrText != clrLast)
        {
            clrLast = pRecord->clrText;

            SetTextColor(hDC, GetNearestColor(hDC, clrLast));
        }

        ExtTextOutA(hDC, pRecord->nX, pRecord->nY, pRecord->nOptions, pRecord->pRect, pRecord->pszText, pRecord->nLength, pRecord->pnDx);
    }
}

// FUNCTION: HYPERX100 0x00487670
void __cdecl DebugOverlay::DrawQueuedBatches(HDC hDC)
{
    COLORREF       clrLast = DEBUG_NO_COLOUR;
    TextOutRecord* pRecord;
    short          nCount;

    while (DebugOverlay::PopTextBatch(&pRecord, &nCount) != 0)
    {
        for (int i = 0; i < nCount; i++)
        {
            if (pRecord->clrText != clrLast)
            {
                clrLast = pRecord->clrText;

                SetTextColor(hDC, GetNearestColor(hDC, clrLast));
            }

            ExtTextOutA(hDC, pRecord->nX, pRecord->nY, pRecord->nOptions, pRecord->pRect, pRecord->pszText, pRecord->nLength, pRecord->pnDx);

            pRecord++;
        }
    }
}
#endif
