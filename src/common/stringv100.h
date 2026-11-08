// String helpers only v1.0.0.0 has. They live outside String: a change to string.h's line
// numbers moves VC6's code for the functions that inline from it, and v1.0.0.1 must not move.
#ifndef HYPERBLADE_STRINGV100_H
#define HYPERBLADE_STRINGV100_H

#ifdef HYPERX_V100
#include "common/string.h"

namespace StringV100
{

String __stdcall Concat(const String& first, const String& second);
String __stdcall Concat(const char* pszFirst, const String& second);
String __stdcall Concat(const String& first, char chSecond);
String __stdcall Concat(char chFirst, const String& second);
// The format is a string resource.
void __cdecl FormatResource(String* pString, UINT nFormatID, ...);
// Shrinks the buffer to the length.
void FreeExtra(String* pString);

} // namespace StringV100
#endif

#endif
