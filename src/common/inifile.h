// booie.ini, read and written the way GetPrivateProfileInt and WritePrivateProfileString did:
// section and key names ignore case, and a write leaves every other line as it was.

#ifndef HYPERBLADE_INIFILE_H
#define HYPERBLADE_INIFILE_H

namespace IniFile
{

// The default when the file, section or key is missing; a negative value reads as zero.
unsigned int ReadInt(const char* pszPath, const char* pszSection, const char* pszKey, int nDefault);

// False when the file cannot be written.
bool WriteString(const char* pszPath, const char* pszSection, const char* pszKey, const char* pszValue);

} // namespace IniFile

#endif
