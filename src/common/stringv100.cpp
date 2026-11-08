// The string helpers only v1.0.0.0 has.

#ifdef HYPERX_V100
#include "common/stringv100.h"
#include <string.h>

// FUNCTION: HYPERX100 0x00414370
String __stdcall StringV100::Concat(const String& first, const String& second)
{
    String temp;

    temp.ConcatAlloc(first.GetHeader()->nDataLength, first.m_pchData, second.GetHeader()->nDataLength, second.m_pchData);

    return temp;
}

// FUNCTION: HYPERX100 0x00414520
String __stdcall StringV100::Concat(const char* pszFirst, const String& second)
{
    String temp;

    temp.ConcatAlloc(String::SafeStrlen(pszFirst), pszFirst, second.GetHeader()->nDataLength, second.m_pchData);

    return temp;
}

// FUNCTION: HYPERX100 0x00414600
String __stdcall StringV100::Concat(const String& first, char chSecond)
{
    String temp;

    temp.ConcatAlloc(first.GetHeader()->nDataLength, first.m_pchData, 1, &chSecond);

    return temp;
}

// FUNCTION: HYPERX100 0x004146d0
String __stdcall StringV100::Concat(char chFirst, const String& second)
{
    String temp;

    temp.ConcatAlloc(1, &chFirst, second.GetHeader()->nDataLength, second.m_pchData);

    return temp;
}

// TODO: The original passes a frame slot below the locals as the va_list, not the arguments.
// FUNCTION: HYPERX100 0x00414870
void __cdecl StringV100::FormatResource(String* pString, UINT nFormatID, ...)
{
    String  strFormat;
    va_list argList;

    strFormat.AssignResourceId(nFormatID);

    va_start(argList, nFormatID);

    String::FormatAlloc(pString, strFormat.m_pchData, argList);

    va_end(argList);
}

// FUNCTION: HYPERX100 0x00414a10
void StringV100::FreeExtra(String* pString)
{
    StringHeader* const pOldHeader = pString->GetHeader();

    if (pOldHeader->nDataLength != pOldHeader->nAllocLength)
    {
        pString->AllocBuffer(pOldHeader->nDataLength);

        memcpy(pString->m_pchData, pOldHeader->Data(), pOldHeader->nDataLength);

        String::FreeHeader(pOldHeader);
    }
}
#endif
