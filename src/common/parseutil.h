#ifndef HYPERBLADE_PARSEUTIL_H
#define HYPERBLADE_PARSEUTIL_H

#include "common/compat.h"
#include <stdio.h>

constexpr auto PARSE_TOKEN_MAX = 0x78;

namespace ParseUtil
{

int   ReadLine(FILE* pStream, char* pszBuffer);
char* SkipToChar(char* pszText, const char* pszStop);
char* SkipPastChar(char* pszText, const char* pszStop);
char  ParseToken(char* pszInput, char* pszOutToken, const char* pszDelimitersA, const char* pszDelimitersB, short nMaxLength, char** ppszNext);
short FindKeyword(const char* pszWord, const char** ppszTable, short nCount);

} // namespace ParseUtil

#endif
