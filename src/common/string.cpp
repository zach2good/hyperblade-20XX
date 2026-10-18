// The reference-counted string's members.

#include "compat.h"
#include <cstdint>
#include "stringtable.h"
#include "globals.h"
#include "app/gameapp.h"
#include "commonglobals.h"
#include "pathstring.h"
#include <string.h>
#include "common/string.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <cstring>

#define STEP(p) ((const char*)(p) + 1)

#define FORCE_ANSI 0x10000

#define FORCE_UNICODE 0x20000

// MFC's CString::FormatV with the names changed.
// FUNCTION: HYPERX 0x00448980
void String::FormatAlloc(String* pString, const char* pszFormat, va_list argList)
{
    va_list argListSave;
    va_copy(argListSave, argList);

    int nMaxLen = 0;

    for (const char* lpsz = pszFormat; *lpsz != 0; lpsz = STEP(lpsz))
    {
        if (*lpsz != '%' || *(lpsz = STEP(lpsz)) == '%')
        {
            nMaxLen += 1;
            continue;
        }

        int nItemLen = 0;
        int nWidth   = 0;

        for (; *lpsz != 0; lpsz = STEP(lpsz))
        {
            if (*lpsz == '#')
            {
                nMaxLen += 2;
            }
            else if (*lpsz == '*')
            {
                nWidth = va_arg(argList, int);
            }
            else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' || *lpsz == ' ')
            {
                ;
            }
            else
            {
                break;
            }
        }

        if (nWidth == 0)
        {
            nWidth = atoi(lpsz);

            for (; *lpsz != 0 && SDL_isdigit((unsigned char)*lpsz); lpsz = STEP(lpsz))
            {
                ;
            }
        }

        int nPrecision = 0;

        if (*lpsz == '.')
        {
            lpsz = STEP(lpsz);

            if (*lpsz == '*')
            {
                nPrecision = va_arg(argList, int);
                lpsz       = STEP(lpsz);
            }
            else
            {
                nPrecision = atoi(lpsz);

                for (; *lpsz != 0 && SDL_isdigit((unsigned char)*lpsz); lpsz = STEP(lpsz))
                {
                    ;
                }
            }
        }

        int nModifier = 0;

        switch (*lpsz)
        {
            case 0x68:
                nModifier = FORCE_ANSI;
                lpsz      = STEP(lpsz);
                break;

            case 0x6c:
                nModifier = FORCE_UNICODE;
                lpsz      = STEP(lpsz);
                break;

            case 0x46:
            case 0x4e:
            case 0x4c:
                lpsz = STEP(lpsz);
                break;
        }

        switch (*lpsz | nModifier)
        {
            case 0x63:
            case 0x43:
                nItemLen = 2;
                va_arg(argList, int);
                break;

            case 0x63 | FORCE_ANSI:
            case 0x43 | FORCE_ANSI:
                nItemLen = 2;
                va_arg(argList, int);
                break;

            case 0x63 | FORCE_UNICODE:
            case 0x43 | FORCE_UNICODE:
                nItemLen = 2;
                va_arg(argList, int);
                break;

            case 0x73:
                // lstrlenA read a null argument as empty.
                nItemLen = SafeStrlen(va_arg(argList, const char*));

                if (nItemLen < 1)
                {
                    nItemLen = 1;
                }
                break;

            case 0x53:
                nItemLen = (int)wcslen((const wchar_t*)va_arg(argList, const unsigned short*));

                if (nItemLen < 1)
                {
                    nItemLen = 1;
                }
                break;

            case 0x73 | FORCE_ANSI:
            case 0x53 | FORCE_ANSI:
                // lstrlenA read a null argument as empty.
                nItemLen = SafeStrlen(va_arg(argList, const char*));

                if (nItemLen < 1)
                {
                    nItemLen = 1;
                }
                break;

            case 0x73 | FORCE_UNICODE:
            case 0x53 | FORCE_UNICODE:
                nItemLen = (int)wcslen((const wchar_t*)va_arg(argList, const unsigned short*));

                if (nItemLen < 1)
                {
                    nItemLen = 1;
                }
                break;
        }

        if (nItemLen != 0)
        {
            if (nItemLen <= nWidth)
            {
                nItemLen = nWidth;
            }

            if (nPrecision != 0)
            {
                if (nItemLen >= nPrecision)
                {
                    nItemLen = nPrecision;
                }
            }
        }
        else
        {
            switch (*lpsz)
            {
                case 0x64:
                case 0x69:
                case 0x75:
                case 0x78:
                case 0x58:
                case 0x6f:
                    va_arg(argList, int);
                    nItemLen = 32;

                    if (nItemLen <= nWidth + nPrecision)
                    {
                        nItemLen = nWidth + nPrecision;
                    }
                    break;

                case 0x65:
                case 0x66:
                case 0x67:
                case 0x47:
                    va_arg(argList, double);
                    nItemLen = 128;

                    if (nItemLen <= nWidth + nPrecision)
                    {
                        nItemLen = nWidth + nPrecision;
                    }
                    break;

                case 0x70:
                    va_arg(argList, void*);
                    nItemLen = 32;

                    if (nItemLen <= nWidth + nPrecision)
                    {
                        nItemLen = nWidth + nPrecision;
                    }
                    break;

                case 0x6e:
                    va_arg(argList, int*);
                    break;
            }
        }

        nMaxLen += nItemLen;
    }

    vsprintf(pString->Reserve(nMaxLen), pszFormat, argListSave);
    va_end(argListSave);

    pString->SetLength(-1);
}

// FUNCTION: HYPERX 0x00423860
String* String::FormatResourcePath(const char* pszDirectory, const char* pszName, const char* pszExtension)
{
    if (pszDirectory != nullptr && *pszDirectory != 0)
    {
        // STRING: HYPERX 0x004bc468
        String::Format(this, "%s\\%s.%s", pszDirectory, pszName, pszExtension);
    }
    else
    {
        // STRING: HYPERX 0x004bc460
        String::Format(this, "%s.%s", pszName, pszExtension);
    }

    return this;
}

// A default-constructed string points at the one shared empty buffer.
// FUNCTION: HYPERX 0x0042f3d0
char** String::GetGlobalStringPtr()
{
    return &g_pEmptyStringData;
}

// FUNCTION: HYPERX 0x0042f3e0
String::String(const String& src)
{
    m_pchData = *GetGlobalStringPtr();

    const char* pchSource = src.m_pchData;

    Assign(SafeStrlen(pchSource), pchSource);
}

// FUNCTION: HYPERX 0x0042f420
String::String(const char* psz)
{
    m_pchData = *GetGlobalStringPtr();

    if (psz != nullptr)
    {
        if ((((std::uintptr_t)(psz) >> 16) == 0))
        {
            AssignResourceId((unsigned int)(std::uintptr_t)psz & 0xffff);

            return;
        }

        const int nLength = (int)strlen(psz);

        if (nLength != 0)
        {
            AllocBuffer(nLength);

            memcpy(m_pchData, psz, nLength);
        }
    }
}

// FUNCTION: HYPERX 0x0042f490
void String::FreeBuffer()
{
    StringHeader* const pHeader = GetHeader();

    if (pHeader != g_pEmptyStringHeader)
    {
        operator delete(GetHeader());
    }
}

// FUNCTION: HYPERX 0x0042f4b0
bool String::AssignResourceId(unsigned int nID)
{
    const char* const pszText = StringTable::Find(nID);

    Assign((unsigned int)strlen(pszText), pszText);

    return pszText[0] != 0;
}

// FUNCTION: HYPERX 0x0042f530
String* String::AssignFromString(const String& src)
{
    Assign(src.GetHeader()->nDataLength, src.m_pchData);

    return this;
}

// FUNCTION: HYPERX 0x0042f550
String String::Concat(const String& source, const char* pszSuffix)
{
    String temp;
    int    nSuffixLength;

    if (pszSuffix == nullptr)
    {
        nSuffixLength = 0;
    }
    else
    {
        nSuffixLength = (int)strlen(pszSuffix);
    }

    temp.ConcatAlloc(source.GetHeader()->nDataLength,
                     source.m_pchData,
                     nSuffixLength,
                     pszSuffix);

    return temp;
}

// FUNCTION: HYPERX 0x0042f5f0
void String::Format(String* pString, const char* pszFormat, ...)
{
    va_list argList;

    va_start(argList, pszFormat);

    String::FormatAlloc(pString, pszFormat, argList);

    va_end(argList);
}

// FUNCTION: HYPERX 0x0042f610
char* String::Reserve(int nLength)
{
    StringHeader* const pOldHeader = GetHeader();

    if (GetHeader()->nRefs > 1 || nLength > GetHeader()->nAllocLength)
    {
        const int nOldLength = GetHeader()->nDataLength;

        if (nLength < nOldLength)
        {
            nLength = nOldLength;
        }

        AllocBuffer(nLength);

        memcpy(m_pchData, pOldHeader->Data(), nOldLength + 1);

        GetHeader()->nDataLength = nOldLength;

        String::FreeHeader(pOldHeader);
    }

    return m_pchData;
}

// FUNCTION: HYPERX 0x0042f680
void String::SetLength(int nLength)
{
    MakeUnique();

    if (nLength == -1)
    {
        nLength = (int)strlen(m_pchData);
    }

    GetHeader()->nDataLength = nLength;
    m_pchData[nLength]       = 0;
}

// FUNCTION: HYPERX 0x0042f6b0
char* String::Resize(int nLength)
{
    Reserve(nLength);

    GetHeader()->nDataLength = nLength;
    m_pchData[nLength]       = 0;

    return m_pchData;
}

// FUNCTION: HYPERX 0x0042f6d0
void String::ClearAndFree()
{
    if (GetHeader() != g_pEmptyStringHeader)
    {
        operator delete(GetHeader());

        m_pchData = *GetGlobalStringPtr();
    }
}

// FUNCTION: HYPERX 0x0042f700
void String::FreeHeader(StringHeader* pHeader)
{
    if (pHeader != g_pEmptyStringHeader)
    {
        operator delete(pHeader);
    }
}

// MFC's CString::Empty; the null test on the fixed buffer never fires, as in the original.
// FUNCTION: HYPERX 0x0042f720
void String::Empty()
{
    if (GetHeader()->nRefs >= 0)
    {
        ClearAndFree();
    }
    else
    {
        Assign(SafeStrlen(g_szEmptyString), g_szEmptyString);
    }
}

// FUNCTION: HYPERX 0x0042f770
void String::AllocBuffer(int nLength)
{
    if (nLength == 0)
    {
        m_pchData = *GetGlobalStringPtr();
        return;
    }

    StringHeader* const pHeader = (StringHeader*)operator new(sizeof(StringHeader) + nLength + 1);

    pHeader->nRefs           = 1;
    pHeader->Data()[nLength] = 0;
    pHeader->nAllocLength    = nLength;
    pHeader->nDataLength     = nLength;

    m_pchData = pHeader->Data();
}

// FUNCTION: HYPERX 0x0042f7c0
void String::PrepareForWrite(int nLength)
{
    if (GetHeader()->nRefs > 1 || nLength > GetHeader()->nAllocLength)
    {
        ClearAndFree();
        AllocBuffer(nLength);
    }
}

// FUNCTION: HYPERX 0x0042f7f0
void String::Assign(unsigned int nLength, const char* pchData)
{
    PrepareForWrite(nLength);

    memcpy(m_pchData, pchData, nLength);

    GetHeader()->nDataLength = nLength;
    m_pchData[nLength]       = 0;
}

// FUNCTION: HYPERX 0x0042f830
void String::ConcatAlloc(int nLength1, const char* pchData1, int nLength2, const char* pchData2)
{
    if (nLength2 + nLength1 == 0)
    {
        return;
    }

    AllocBuffer(nLength2 + nLength1);

    memcpy(m_pchData, pchData1, nLength1);
    memcpy(m_pchData + nLength1, pchData2, nLength2);
}

// FUNCTION: HYPERX 0x0042f890
void String::AppendData(int nLength, const char* pchData)
{
    char* const pchOld = m_pchData;

    if (HeaderOf(pchOld)->nRefs <= 1 &&
        HeaderOf(pchOld)->nDataLength + nLength <=
            HeaderOf(pchOld)->nAllocLength)
    {
        memcpy(m_pchData + HeaderOf(pchOld)->nDataLength, pchData, nLength);

        GetHeader()->nDataLength += nLength;

        m_pchData[GetHeader()->nDataLength] = 0;

        return;
    }

    ConcatAlloc(HeaderOf(pchOld)->nDataLength, pchOld, nLength, pchData);

    String::FreeHeader(HeaderOf(pchOld));
}

// FUNCTION: HYPERX 0x0042f910
void String::MakeUnique()
{
    char* const pchOld = m_pchData;

    if (HeaderOf(pchOld)->nRefs > 1)
    {
        ClearAndFree();

        AllocBuffer(HeaderOf(pchOld)->nDataLength);

        memcpy(m_pchData, pchOld, HeaderOf(pchOld)->nDataLength + 1);
    }
}

// FUNCTION: HYPERX 0x00480bd0
int String::ExtractToken(const char* pszSource, char* pszDest, const char** ppszNext)
{
    const char* pszEnd;
    int         nLength;
    int         nResult = -1;

    pszDest[0] = 0;

    if (pszSource != nullptr)
    {
        pszEnd = (char*)strpbrk(pszSource,
                                g_pszTokenDelimiters);

        if (pszEnd != nullptr)
        {
            const char* pszStart = pszEnd + 1;

            pszEnd = (char*)strpbrk(pszStart,
                                    g_pszTokenDelimiters);

            *ppszNext = pszEnd;

            if (pszEnd != nullptr)
            {
                nLength = (int)(pszEnd - pszStart);

                nResult = 0;

                if (nLength != 0)
                {
                    SDL_strlcpy(pszDest, pszStart, nLength + 1);

                    nResult = nLength;
                }
            }
            else
            {
                nLength = (int)strlen(pszStart);
                nResult = 0;

                if (nLength != 0)
                {
                    SDL_strlcpy(pszDest, pszStart, nLength + 1);

                    nResult = nLength;
                }
            }
        }
    }

    return nResult;
}

EmptyString   g_emptyString        = { { -1, 0, 0 }, { 0, 0, 0, 0 } };
StringHeader* g_pEmptyStringHeader = &g_emptyString.header;
char*         g_pEmptyStringData   = g_emptyString.szData;
