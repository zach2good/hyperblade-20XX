// booie.ini, read and written the way GetPrivateProfileInt and WritePrivateProfileString did:
// section and key names ignore case, and a write leaves every other line as it was.

#ifndef HYPERBLADE_INIFILE_H
#define HYPERBLADE_INIFILE_H

#include <string>

namespace IniFile
{

// The default when the file, section or key is missing; a negative value reads as zero.
unsigned int ReadInt(const char* pszPath, const char* pszSection, const char* pszKey, int nDefault);

// The value with the spaces around it trimmed, or the default when the key is missing.
std::string ReadString(const char* pszPath, const char* pszSection, const char* pszKey, const char* pszDefault);

// False when the file cannot be written.
bool WriteString(const char* pszPath, const char* pszSection, const char* pszKey, const char* pszValue);

} // namespace IniFile

#endif
