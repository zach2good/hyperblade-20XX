// The line and token readers, keyword lookup and character skips the configuration parsers share.

#include "compat.h"
#include <cstring>
#include <cstdio>
#include "common/parseutil.h"
#include "common/staticinit.h"
#include "common/commonglobals.h"
#include <SDL3/SDL.h>

float g_flOneParseUtil               = 1.0f;
float g_flDotsPerInchParseUtil       = 72.0f;
float g_flDotPitchParseUtil          = 0.75f;
float g_flCmPerInchParseUtil         = 2.54f;
float g_flFifthParseUtil             = 0.2f * g_flOneParseUtil;
float g_flDotPitchInchParseUtil      = g_flDotsPerInchParseUtil * g_flDotPitchParseUtil;
float g_flScreenDotsPerInchParseUtil = g_flDotsPerInchParseUtil;
float g_flDotPitchCmParseUtil        = g_flDotPitchInchParseUtil / g_flCmPerInchParseUtil;
float g_flScreenDotsPerCmParseUtil   = g_flScreenDotsPerInchParseUtil / g_flCmPerInchParseUtil;

#define STREAM_EOF ((char)-1)

// The original calls the multibyte _mbsicmp at 0x0049f2d2, not _stricmp.
// FUNCTION: HYPERX 0x00435f90
short ParseUtil::FindKeyword(const char* pszWord, const char** ppszTable, short nCount)
{
    for (short iEntry = 0; iEntry < nCount; iEntry++)
    {
        // Inline, VC6 loads the entry into ECX; the client uses EAX (0x00435fac).
        const auto* const pszEntry = ppszTable[iEntry];

        if (SDL_strcasecmp(pszEntry, pszWord) == 0)
        {
            return iEntry;
        }
    }

    return -1;
}

// An empty token is a failure, so a caller can loop on this until it stops.
// FUNCTION: HYPERX 0x00487370
char ParseUtil::ParseToken(char* pszInput, char* pszOutToken, const char* pszDelimitersA, const char* pszDelimitersB, short nMaxLength, char** ppszNext)
{
    char* pszStart;
    int   nLength;

    // STRING: HYPERX 0x004c732c
    // The first character past the blanks, or null when there is none.
    char* const pszScan = [&]() -> char*
    {
        auto* const pszFirst = pszInput + strspn(pszInput, " 	");

        if (*pszFirst == 0)
        {
            return nullptr;
        }

        return pszFirst;
    }();

    if (pszScan == nullptr)
    {
        return 0;
    }

    if (pszDelimitersA != nullptr)
    {
        pszStart = strpbrk(pszScan, pszDelimitersA);

        if (pszStart == nullptr)
        {
            return 0;
        }
    }
    else
    {
        pszStart = pszScan;
    }

    auto* const pszEnd = strpbrk(pszStart, pszDelimitersB);

    if (pszEnd == nullptr)
    {
        nLength = static_cast<int>(strlen(pszStart));

        *ppszNext = nullptr;
    }
    else
    {
        nLength = static_cast<int>(pszEnd - pszStart);

        if (nLength > nMaxLength - 1)
        {
            return 0;
        }

        *ppszNext = pszEnd;
    }

    if (nLength == 0)
    {
        return 0;
    }

    SDL_strlcpy(pszOutToken, pszStart, nLength + 1);

    pszOutToken[nLength] = '\0';

    return 1;
}

// FUNCTION: HYPERX 0x00487420
char* ParseUtil::SkipToChar(char* pszText, const char* pszStop)
{
    if (pszText != nullptr)
    {
        auto nLeft = static_cast<int>(strlen(pszText));

        if (nLeft != 0)
        {
            while (nLeft != 0)
            {
                const auto cAt = *pszText;

                if (cAt == *pszStop)
                {
                    return pszText;
                }

                if (cAt != ' ' && cAt != '\t')
                {
                    break;
                }

                pszText++;

                nLeft--;
            }
        }
    }

    return nullptr;
}

// Steps past the character, and answers null when there is none.
// FUNCTION: HYPERX 0x00487460
char* ParseUtil::SkipPastChar(char* pszText, const char* pszStop)
{
    auto* const pAt = ParseUtil::SkipToChar(pszText, pszStop);

    if (pAt == nullptr)
    {
        return nullptr;
    }

    return pAt + 1;
}

// End of file is the byte 0xff, stored and then tested, so only an empty last line answers -1.
// FUNCTION: HYPERX 0x00493ec0
int ParseUtil::ReadLine(FILE* pStream, char* pszBuffer)
{
    auto* const pStart = pszBuffer;

    *pszBuffer = static_cast<char>(fgetc(pStream));

    while (*pszBuffer != STREAM_EOF)
    {
        if (*pszBuffer == '\n' || *pszBuffer == '\r')
        {
            break;
        }

        // A nul in the file is overwritten, as CharNextA never stepped past one.
        if (*pszBuffer != 0)
        {
            pszBuffer++;
        }

        *pszBuffer = static_cast<char>(fgetc(pStream));
    }

    if (pStart == pszBuffer && *pszBuffer == STREAM_EOF)
    {
        return -1;
    }

    *pszBuffer = '\0';

    return static_cast<int>(pszBuffer - pStart);
}
