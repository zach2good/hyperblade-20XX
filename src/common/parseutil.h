#ifndef HYPERBLADE_PARSEUTIL_H
#define HYPERBLADE_PARSEUTIL_H

#include "common/compat.h"
#include <stdio.h>

#define PARSE_TOKEN_MAX 0x78

namespace ParseUtil
{

int __cdecl   ReadLine(FILE* pStream, char* pszBuffer);
char* __cdecl SkipToChar(char* pszText, const char* pszStop);
char* __cdecl SkipPastChar(char* pszText, const char* pszStop);
char __cdecl  ParseToken(char* pszInput, char* pszOutToken, const char* pszDelimitersA, const char* pszDelimitersB, short nMaxLength, char** ppszNext);
short __cdecl FindKeyword(const char* pszWord, const char** ppszTable, short nCount);

} // namespace ParseUtil

#endif
