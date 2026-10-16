#include "common/compat.h"
#include "animset.h"
#include "animdb.h"
#include "animregistry.h"
struct GameApp;
#include "common/fileobj.h"
#include "globals.h"
#include "planvm/planvm.h"
#include "common/pathstring.h"
#include <string.h>
#include <new.h>
#include <stdio.h>
#include "app/gameapp.h"
#include "engine/messageids.h"
#include "animnode.h"
#include "app/config.h"
#include <SDL3/SDL.h>

// Each entry is released through its own scalar deleting destructor.

// FUNCTION: HYPERX 0x004181b0
AnimScriptTable::~AnimScriptTable()
{
    while (m_nCount-- > 0)
    {
        delete m_apEntries[m_nCount];

        m_apEntries[m_nCount] = 0;
    }
}

// A set is taken when its name holds the first; a clip starting with the second gets a kind.
#define SET_FILTER_RESOURCE  0x20
#define CLIP_PREFIX_RESOURCE 0x42

// The parse stops at the first line the line parser refuses, so a bad line truncates the file.
// FUNCTION: HYPERX 0x00418210
void AnimScriptTable::ParseScript(const char* pszPath, AnimRegistry* pAnimMgr, int bBind)
{
    OwnedFile file;

    PlanVM* const pVm = new PlanVM;

    m_pPlanVm = pVm;

    if (pVm != 0)
    {
        pVm->LoadSymbolTables();
    }

    if (file.m_file.Open(pszPath, FILE_MODE_READ) != 0)
    {
        memset(m_apEntries, 0, sizeof(m_apEntries));

        m_nCount = 0;

        AnimScriptState state((LPCSTR)SET_FILTER_RESOURCE, (LPCSTR)CLIP_PREFIX_RESOURCE);
        char*           pLine;

        while (file.m_file.GetNextLine(&pLine) > 0 && ParseScriptLine(pLine, &state) != 0)
        {
        }

        file.m_file.CloseAndTest();

        if (bBind != 0)
        {
            for (short nSet = 0; nSet < m_nCount; nSet++)
            {
                pAnimMgr->BindAnimSets(m_apEntries[nSet]);
            }
        }
    }
    else
    {
        // The original passes nothing for the %s.
        // STRING: HYPERX 0x004bb8b4
        g_gameApp.Fatal("Assembly file %s could not be opened!");
    }

    PlanVM* const pOwned = m_pPlanVm;

    if (pOwned != 0)
    {
        delete pOwned;

        m_pPlanVm = 0;
    }
}

#define REF_CHARS  0x14
#define NAME_CHARS 0x18

// Blank lines and comments are skipped; a clip's message defaults to MSG_NOMSG.
// FUNCTION: HYPERX 0x004183e0
unsigned int AnimScriptTable::ParseScriptLine(const char* pszLine, AnimScriptState* pState)
{
    short   nSecond;
    char    szRef[REF_CHARS];
    char    szName[NAME_CHARS];
    PlanVM* pVm;
    int     nMessage;

    if (*pszLine == '\0')
    {
        return 1;
    }

    if (Config::IsCommentLine(pszLine) != 0)
    {
        return 1;
    }

    const char* pszCursor = 0;

    if (ParseAndAddAnimDB(&pState->pDb, pszLine, &pszCursor, pState->strFilter) != 0)
    {
        pState->bStarted = 1;
    }

    if (pState->bStarted != 0)
    {
        short nFirst;

        if (ParseSegment(&pState->pSegment, pszLine, &pszCursor) != 0)
        {
            nFirst = pState->pDb->GetNodeCount();

            pState->pDb->m_wNodeCount = (short)(nFirst + 1);

            pState->pDb->m_apNodes[nFirst] = pState->pSegment;

            return 1;
        }
        else
        {
            short nSyncFrame   = -1;
            short nSyncSegment = -1;

            ParseTrigger(pszLine, &nFirst, &nSecond, szName, szRef, &nSyncSegment, &nSyncFrame);

            if (nSecond != -1)
            {
                if (strstr(szName, (const char*)pState->strPrefix) == szName && (pVm = m_pPlanVm) != 0)
                {
                    nMessage = (int)pVm->LookupSymbol(szName).nValue;
                }
                else
                {
                    nMessage = MSG_NOMSG;
                }

                pState->pDb->AddClipEntry((unsigned short)nFirst, (unsigned short)nSecond, szName, nMessage, szRef, nSyncSegment, nSyncFrame);
            }
        }
    }

    return 1;
}

// The tab the two field walkers split on; src/common/commonglobals.h names the same character at 0x004b3f00.
// GLOBAL: HYPERX 0x004b22d0
extern const char* const g_pszAnimTokenDelimiters;

// The scanf format the two field readers push.
// GLOBAL: HYPERX 0x004b22b8
extern const char* const g_pszAnimShortFormat;

// GLOBAL: HYPERX 0x004b22c8
extern const char* const g_pszAnimFloatFormat;

#define SEGMENT_TOKEN_CHARS 256
#define TRIGGER_TOKEN_CHARS 0x50

#define TRIGGER_FIELDS 3

// FUNCTION: HYPERX 0x00418540
int AnimScriptTable::ParseAndAddAnimDB(AnimDB** ppOut, const char* pszSource, const char** ppszCursor, const String& strFilter)
{
    char szName[20];
    char szExtra[32];

    int nResult = 0;

    szExtra[0] = 0;

    if (String::ExtractToken(pszSource, szName, ppszCursor) > 0)
    {
        if (strstr(szName, (const char*)strFilter) != 0)
        {
            String::ExtractToken(*ppszCursor, szExtra, ppszCursor);

            nResult = CreateAnimDB(ppOut, szName, szExtra);
        }
    }

    return nResult;
}

// FUNCTION: HYPERX 0x004185c0
int AnimScriptTable::CreateAnimDB(AnimDB** ppOut, const char* pszName, const char* pszExtra)
{
    const int nBefore = GameApp::GetInstallLevel();

    AnimDB* const pDb = new AnimDB(pszName, pszExtra);

    m_apEntries[m_nCount] = pDb;

    *ppOut = pDb;

    const int nAfter = GameApp::GetInstallLevel();

    int* const pLevel = &(*ppOut)->m_nInstallLevel;

    *pLevel += nAfter - nBefore;

    if (*ppOut != 0)
    {
        m_nCount++;
    }

    return *ppOut != 0;
}

inline int AnimScriptTable::ScanShort(const char** ppszCursor, char* pszToken, short* pnOut)
{
    return String::ExtractToken(*ppszCursor, pszToken, ppszCursor) > 0 && sscanf(pszToken, g_pszAnimShortFormat, pnOut) == 1;
}

inline int AnimScriptTable::ScanFloat(const char** ppszCursor, char* pszToken, float* pflOut)
{
    return String::ExtractToken(*ppszCursor, pszToken, ppszCursor) > 0 && sscanf(pszToken, g_pszAnimFloatFormat, pflOut) == 1;
}

// A segment's description is freed on every path out; nothing clamps the fast rate.
// FUNCTION: HYPERX 0x00418670
int AnimScriptTable::ParseSegment(AnimNode** ppSegment, const char* pszLine, const char** ppszCursor)
{
    int  nResult = 0;
    char szToken[SEGMENT_TOKEN_CHARS];

    const char* pszScan = (const char*)strpbrk(pszLine, g_pszAnimTokenDelimiters);
    pszScan             = pszScan + 1;
    pszScan             = (const char*)strpbrk(pszScan, g_pszAnimTokenDelimiters);
    pszScan             = pszScan + 1;
    pszScan             = (const char*)strpbrk(pszScan, g_pszAnimTokenDelimiters);

    *ppszCursor = pszScan;

    if (String::ExtractToken(pszScan, szToken, ppszCursor) <= 0)
    {
        *ppszCursor = pszLine;
    }
    else
    {
        AnimNode* const pNode = new AnimNode(1);

        *ppSegment = pNode;

        if (pNode != 0)
        {
            strcpy(pNode->m_szName, szToken);

            if (String::ExtractToken(*ppszCursor, szToken, ppszCursor) >= 0)
            {
                SDL_strlcpy((*ppSegment)->m_pszDescription, szToken, DESCRIPTION_CHARS);

                (*ppSegment)->m_pszDescription[DESCRIPTION_CHARS] = 0;

                if (ScanShort(ppszCursor, szToken, &(*ppSegment)->m_wStartFrame))
                {
                    if (ScanShort(ppszCursor, szToken, &(*ppSegment)->m_wEndFrame))
                    {
                        if (ScanFloat(ppszCursor, szToken, &(*ppSegment)->m_flSlowFps))
                        {
                            if (ScanFloat(ppszCursor, szToken, &(*ppSegment)->m_flStandardFps))
                            {
                                if (ScanFloat(ppszCursor, szToken, &(*ppSegment)->m_flFastFps))
                                {
                                    if (ScanShort(ppszCursor, szToken, &(*ppSegment)->m_wLoopCount))
                                    {
                                        if ((*ppSegment)->m_flSlowFps > (*ppSegment)->m_flStandardFps)
                                        {
                                            (*ppSegment)->m_flSlowFps = (*ppSegment)->m_flStandardFps;
                                        }

                                        nResult = 1;
                                    }
                                    else
                                    {
                                        // STRING: HYPERX 0x004bb9b4
                                        g_gameApp.Fatal("ERROR: in Loop Count field -- %s.", pszLine);
                                    }
                                }
                                else
                                {
                                    // STRING: HYPERX 0x004bb994
                                    g_gameApp.Fatal("ERROR: in Fast FPS field -- %s.", pszLine);
                                }
                            }
                            else
                            {
                                // STRING: HYPERX 0x004bb970
                                g_gameApp.Fatal("ERROR: in Standard FPS field -- %s.", pszLine);
                            }
                        }
                        else
                        {
                            // STRING: HYPERX 0x004bb950
                            g_gameApp.Fatal("ERROR: in Slow FPS field -- %s.", pszLine);
                        }
                    }
                    else
                    {
                        // STRING: HYPERX 0x004bb92c
                        g_gameApp.Fatal("ERROR: in End Frame field -- %s.", pszLine);
                    }
                }
                else
                {
                    // STRING: HYPERX 0x004bb908
                    g_gameApp.Fatal("ERROR: in Start Frame field -- %s.", pszLine);
                }
            }
            else
            {
                // STRING: HYPERX 0x004bb8dc
                g_gameApp.Fatal("ERROR in Segment Description field -- %s.", pszLine);
            }

            if ((*ppSegment)->m_pszDescription != 0)
            {
                delete[] (*ppSegment)->m_pszDescription;

                (*ppSegment)->m_pszDescription = 0;
            }
        }
    }

    return nResult;
}

// FUNCTION: HYPERX 0x004189c0
int AnimScriptTable::ParseTrigger(const char* pszLine, short* pnSegment, short* pnTriggerFrame, char* pszName, char* pszRef, short* pnSyncSegment, short* pnSyncFrame)
{
    char szToken[TRIGGER_TOKEN_CHARS];
    int  nResult = 1;

    *pnSegment      = 0;
    *pnTriggerFrame = -1;

    if (pszLine != 0)
    {
        const char* pszScan = pszLine;

        for (int nIndex = 0; nIndex < TRIGGER_FIELDS; nIndex++)
        {
            pszScan = (const char*)strpbrk(pszScan, g_pszAnimTokenDelimiters);

            if (pszScan == 0)
            {
                return 0;
            }

            pszScan = pszScan + 1;
        }

        if (String::ExtractToken(pszScan, szToken, &pszScan) <= 0 || sscanf(szToken, g_pszAnimShortFormat, pnSegment) != 1)
        {
            // STRING: HYPERX 0x004bb9d8
            g_gameApp.Fatal("ERROR: in Trigger Segment field.");

            nResult = 0;
        }
        else if (String::ExtractToken(pszScan, szToken, &pszScan) <= 0 || sscanf(szToken, g_pszAnimShortFormat, pnTriggerFrame) != 1)
        {
            // STRING: HYPERX 0x004bb9fc
            g_gameApp.Fatal("ERROR: in Trigger Frame field.");

            nResult = 0;
        }
        else if (String::ExtractToken(pszScan, pszName, &pszScan) > 0)
        {
            if (String::ExtractToken(pszScan, pszRef, &pszScan) > 0)
            {
                if (String::ExtractToken(pszScan, szToken, &pszScan) <= 0 || sscanf(szToken, g_pszAnimShortFormat, pnSyncSegment) != 1)
                {
                    // STRING: HYPERX 0x004bba40
                    g_gameApp.Fatal("ERROR: in Sync Seg field.");

                    nResult = 0;
                }
                else if (String::ExtractToken(pszScan, szToken, &pszScan) <= 0 || sscanf(szToken, g_pszAnimShortFormat, pnSyncFrame) != 1)
                {
                    // STRING: HYPERX 0x004bba5c
                    g_gameApp.Fatal("ERROR: in Sync frame field.");

                    nResult = 0;
                }
            }
        }
        else
        {
            // STRING: HYPERX 0x004bba1c
            g_gameApp.Fatal("ERROR: in Trigger Keyword field.");

            nResult = 0;
        }
    }

    return nResult;
}
