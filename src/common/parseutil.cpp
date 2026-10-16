// The line and token readers, keyword lookup and character skips the configuration parsers share.

#include "compat.h"
#include <string.h>
#include <stdio.h>
#include <mbstring.h>
#include "common/parseutil.h"
#include "common/staticinit.h"
#include "common/commonglobals.h"

float g_flOneParseUtil               = 1.0f;
float g_flDotsPerInchParseUtil       = 72.0f;
float g_flDotPitchParseUtil          = 0.75f;
float g_flCmPerInchParseUtil         = 2.54f;
float g_flFifthParseUtil             = 0.2f * g_flOneParseUtil;
float g_flDotPitchInchParseUtil      = g_flDotsPerInchParseUtil * g_flDotPitchParseUtil;
float g_flScreenDotsPerInchParseUtil = g_flDotsPerInchParseUtil;
float g_flDotPitchCmParseUtil        = g_flDotPitchInchParseUtil / g_flCmPerInchParseUtil;
float g_flScreenDotsPerCmParseUtil   = g_flScreenDotsPerInchParseUtil / g_flCmPerInchParseUtil;

#define STREAM_EOF ((char)0xff)

// The original calls the multibyte _mbsicmp at 0x0049f2d2, not _stricmp.
// FUNCTION: HYPERX 0x00435f90
short __cdecl ParseUtil::FindKeyword(const char* pszWord, const char** ppszTable, short nCount)
{
    for (short iEntry = 0; iEntry < nCount; iEntry++)
    {
        // Inline, VC6 loads the entry into ECX; the client uses EAX (0x00435fac).
        const char* const pszEntry = ppszTable[iEntry];

        if (_mbsicmp((const unsigned char*)pszEntry, (const unsigned char*)pszWord) == 0)
        {
            return iEntry;
        }
    }

    return -1;
}

// An empty token is a failure, so a caller can loop on this until it stops.
// FUNCTION: HYPERX 0x00487370
char __cdecl ParseUtil::ParseToken(char* pszInput, char* pszOutToken, const char* pszDelimitersA, const char* pszDelimitersB, short nMaxLength, char** ppszNext)
{
    char* pszStart;
    int   nLength;

    // STRING: HYPERX 0x004c732c
    char* const pszScan = (char*)_mbsspnp((const unsigned char*)pszInput, (const unsigned char*)" 	");

    if (pszScan == 0)
    {
        return 0;
    }

    if (pszDelimitersA != 0)
    {
        pszStart = (char*)_mbspbrk((const unsigned char*)pszScan, (const unsigned char*)pszDelimitersA);

        if (pszStart == 0)
        {
            return 0;
        }
    }
    else
    {
        pszStart = pszScan;
    }

    char* const pszEnd = (char*)_mbspbrk((const unsigned char*)pszStart, (const unsigned char*)pszDelimitersB);

    if (pszEnd == 0)
    {
        nLength = (int)strlen(pszStart);

        *ppszNext = 0;
    }
    else
    {
        nLength = (int)(pszEnd - pszStart);

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

    lstrcpynA(pszOutToken, pszStart, nLength + 1);

    pszOutToken[nLength] = '\0';

    return 1;
}

// FUNCTION: HYPERX 0x00487420
char* __cdecl ParseUtil::SkipToChar(char* pszText, const char* pszStop)
{
    if (pszText != 0)
    {
        int nLeft = (int)strlen(pszText);

        if (nLeft != 0)
        {
            while (nLeft != 0)
            {
                char cAt = *pszText;

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

    return 0;
}

// Steps past the character, and answers null when there is none.
// FUNCTION: HYPERX 0x00487460
char* __cdecl ParseUtil::SkipPastChar(char* pszText, const char* pszStop)
{
    char* const pAt = ParseUtil::SkipToChar(pszText, pszStop);

    if (pAt == 0)
    {
        return 0;
    }

    return pAt + 1;
}

// End of file is the byte 0xff, stored and then tested, so only an empty last line answers -1.
// FUNCTION: HYPERX 0x00493ec0
int __cdecl ParseUtil::ReadLine(FILE* pStream, char* pszBuffer)
{
    char* const pStart = pszBuffer;

    *pszBuffer = (char)fgetc(pStream);

    while (*pszBuffer != STREAM_EOF)
    {
        if (*pszBuffer == '\n' || *pszBuffer == '\r')
        {
            break;
        }

        pszBuffer = CharNextA(pszBuffer);

        *pszBuffer = (char)fgetc(pStream);
    }

    if (pStart == pszBuffer && *pszBuffer == STREAM_EOF)
    {
        return -1;
    }

    *pszBuffer = '\0';

    return (int)(pszBuffer - pStart);
}
