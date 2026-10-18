#include "common/compat.h"
#include "animset.h"
#include "animdb.h"
#include "animregistry.h"
struct GameApp;
#include "common/fileobj.h"
#include "globals.h"
#include "planvm/planvm.h"
#include "common/pathstring.h"
#include <cstdint>
#include <cstring>
#include <new>
#include <cstdio>
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

        m_apEntries[m_nCount] = nullptr;
    }
}

// A set is taken when its name holds the first; a clip starting with the second gets a kind.
constexpr auto kSetFilterResource  = std::uintptr_t{ 0x20 };
constexpr auto kClipPrefixResource = std::uintptr_t{ 0x42 };

// The parse stops at the first line the line parser refuses, so a bad line truncates the file.
// FUNCTION: HYPERX 0x00418210
void AnimScriptTable::ParseScript(const char* pszPath, AnimRegistry* pAnimMgr, int bBind)
{
    OwnedFile file;

    auto* const pVm = new PlanVM;

    m_pPlanVm = pVm;

    if (pVm != nullptr)
    {
        pVm->LoadSymbolTables();
    }

    if (file.m_file.Open(pszPath, FILE_MODE_READ) != 0)
    {
        memset(m_apEntries, 0, sizeof(m_apEntries));

        m_nCount = 0;

        AnimScriptState state(reinterpret_cast<const char*>(kSetFilterResource), reinterpret_cast<const char*>(kClipPrefixResource));
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

    auto* const pOwned = m_pPlanVm;

    if (pOwned != nullptr)
    {
        delete pOwned;

        m_pPlanVm = nullptr;
    }
}

constexpr auto kRefChars  = 0x14;
constexpr auto kNameChars = 0x18;

// Blank lines and comments are skipped; a clip's message defaults to kMsgNomsg.
// FUNCTION: HYPERX 0x004183e0
unsigned int AnimScriptTable::ParseScriptLine(const char* pszLine, AnimScriptState* pState)
{
    short   nSecond;
    char    szRef[kRefChars];
    char    szName[kNameChars];
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

    const char* pszCursor = nullptr;

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

            pState->pDb->m_wNodeCount = static_cast<short>(nFirst + 1);

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
                if (strstr(szName, (const char*)pState->strPrefix) == szName && (pVm = m_pPlanVm) != nullptr)
                {
                    nMessage = static_cast<int>(pVm->LookupSymbol(szName).nValue);
                }
                else
                {
                    nMessage = kMsgNomsg;
                }

                pState->pDb->AddClipEntry(static_cast<unsigned short>(nFirst), static_cast<unsigned short>(nSecond), szName, nMessage, szRef, nSyncSegment, nSyncFrame);
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

constexpr auto kSegmentTokenChars = 256;
constexpr auto kTriggerTokenChars = 0x50;

constexpr auto kTriggerFields = 3;

// FUNCTION: HYPERX 0x00418540
int AnimScriptTable::ParseAndAddAnimDB(AnimDB** ppOut, const char* pszSource, const char** ppszCursor, const String& strFilter)
{
    char szName[20];
    char szExtra[32];

    auto nResult = 0;

    szExtra[0] = 0;

    if (String::ExtractToken(pszSource, szName, ppszCursor) > 0)
    {
        if (strstr(szName, (const char*)strFilter) != nullptr)
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
    const auto nBefore = GameApp::GetInstallLevel();

    auto* const pDb = new AnimDB(pszName, pszExtra);

    m_apEntries[m_nCount] = pDb;

    *ppOut = pDb;

    const auto nAfter = GameApp::GetInstallLevel();

    auto* const pLevel = &(*ppOut)->m_nInstallLevel;

    *pLevel += nAfter - nBefore;

    if (*ppOut != nullptr)
    {
        m_nCount++;
    }

    return *ppOut != nullptr;
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
    auto nResult = 0;
    char szToken[kSegmentTokenChars];

    const auto* pszScan = strpbrk(pszLine, g_pszAnimTokenDelimiters);
    pszScan             = pszScan + 1;
    pszScan             = strpbrk(pszScan, g_pszAnimTokenDelimiters);
    pszScan             = pszScan + 1;
    pszScan             = strpbrk(pszScan, g_pszAnimTokenDelimiters);

    *ppszCursor = pszScan;

    if (String::ExtractToken(pszScan, szToken, ppszCursor) <= 0)
    {
        *ppszCursor = pszLine;
    }
    else
    {
        auto* const pNode = new AnimNode(1);

        *ppSegment = pNode;

        if (pNode != nullptr)
        {
            strcpy(pNode->m_szName, szToken);

            if (String::ExtractToken(*ppszCursor, szToken, ppszCursor) >= 0)
            {
                SDL_strlcpy((*ppSegment)->m_pszDescription, szToken, kDescriptionChars);

                (*ppSegment)->m_pszDescription[kDescriptionChars] = 0;

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

            if ((*ppSegment)->m_pszDescription != nullptr)
            {
                delete[] (*ppSegment)->m_pszDescription;

                (*ppSegment)->m_pszDescription = nullptr;
            }
        }
    }

    return nResult;
}

// FUNCTION: HYPERX 0x004189c0
int AnimScriptTable::ParseTrigger(const char* pszLine, short* pnSegment, short* pnTriggerFrame, char* pszName, char* pszRef, short* pnSyncSegment, short* pnSyncFrame)
{
    char szToken[kTriggerTokenChars];
    auto nResult = 1;

    *pnSegment      = 0;
    *pnTriggerFrame = -1;

    if (pszLine != nullptr)
    {
        const auto* pszScan = pszLine;

        for (int nIndex = 0; nIndex < kTriggerFields; nIndex++)
        {
            pszScan = strpbrk(pszScan, g_pszAnimTokenDelimiters);

            if (pszScan == nullptr)
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
