#include "common/compat.h"
#include "common/inifile.h"
#include "gameoptions.h"
#include "globals.h"
#include "common/pathstring.h"
#include "gameengine.h"
#include "app/gameapp.h"
#include <cstdio>

// The ini section name, seven setting key names, and the install level.

// FUNCTION: HYPERX 0x00479880
GameOptions::GameOptions()
{
    // STRING: HYPERX 0x004c64e4
    m_pszSection = "Options";

    // STRING: HYPERX 0x004c64d8
    m_pszDifficulty = "Difficulty";

    // STRING: HYPERX 0x004c64cc
    m_pszBrutality = "Brutality";

    // STRING: HYPERX 0x004c64c0
    m_pszSfxVolume = "SfxVolume";

    // STRING: HYPERX 0x004c64b4
    m_pszMusicVolume = "MusicVolume";

    // STRING: HYPERX 0x004c64a8
    m_pszScreenSize = "ScreenSize";

    // STRING: HYPERX 0x004c6498
    m_pszPeriodLength = "PeriodLength";

    // STRING: HYPERX 0x004c1ea8
    const char* pszSection = "Misc";

    // STRING: HYPERX 0x004c64ec
    const char* pszKey = "InstallLevel";

    m_nInstallLevel = ReadInt(pszSection, pszKey, 0);
}

GameOptions::~GameOptions()
{
}

// FUNCTION: HYPERX 0x004798d0
int GameOptions::WriteString(const char* pszSection, const char* pszKey, const char* pszValue)
{
    // STRING: HYPERX 0x004c64fc
    const int nResult = IniFile::WriteString(String::Concat(g_gameApp.m_strExeDir.m_str, "booie.ini"), pszSection, pszKey, pszValue);

    return nResult;
}

// FUNCTION: HYPERX 0x00479910
void GameOptions::WriteInt(const char* pszSection, const char* pszKey, int nValue)
{
    char szValue[MAX_PATH_LENGTH];

    // STRING: HYPERX 0x004c6508
    sprintf(szValue, "%d", nValue);

    WriteString(pszSection, pszKey, szValue);
}

// FUNCTION: HYPERX 0x00479960
unsigned int GameOptions::ReadInt(const char* pszSection, const char* pszKey, int nDefault)
{
    // STRING: HYPERX 0x004c64fc
    return IniFile::ReadInt(String::Concat(g_gameApp.m_strExeDir.m_str, "booie.ini"), pszSection, pszKey, nDefault);
}

std::string GameOptions::ReadString(const char* pszSection, const char* pszKey, const char* pszDefault)
{
    return IniFile::ReadString(String::Concat(g_gameApp.m_strExeDir.m_str, "booie.ini"), pszSection, pszKey, pszDefault);
}

// FUNCTION: HYPERX 0x004799a0
unsigned int GameOptions::ReadDifficulty()
{
    return ReadInt(m_pszSection, m_pszDifficulty, 1);
}

// FUNCTION: HYPERX 0x004799b0
void GameOptions::WriteDifficulty(int nValue)
{
    WriteInt(m_pszSection, m_pszDifficulty, nValue);
}

// FUNCTION: HYPERX 0x004799d0
unsigned int GameOptions::ReadBrutality()
{
    return ReadInt(m_pszSection, m_pszBrutality, 1);
}

// FUNCTION: HYPERX 0x004799e0
void GameOptions::WriteBrutality(int nValue)
{
    WriteInt(m_pszSection, m_pszBrutality, nValue);
}

// FUNCTION: HYPERX 0x00479a00
unsigned int GameOptions::ReadSfxVolume()
{
    return ReadInt(m_pszSection, m_pszSfxVolume, 7);
}

// FUNCTION: HYPERX 0x00479a10
void GameOptions::WriteSfxVolume(int nValue)
{
    WriteInt(m_pszSection, m_pszSfxVolume, nValue);
}

// FUNCTION: HYPERX 0x00479a30
unsigned int GameOptions::ReadMusicVolume()
{
    return ReadInt(m_pszSection, m_pszMusicVolume, 4);
}

// FUNCTION: HYPERX 0x00479a40
void GameOptions::WriteMusicVolume(int nValue)
{
    WriteInt(m_pszSection, m_pszMusicVolume, nValue);
}

// FUNCTION: HYPERX 0x00479a60
unsigned int GameOptions::ReadScreenSize()
{
    return ReadInt(m_pszSection, m_pszScreenSize, 6);
}

// FUNCTION: HYPERX 0x00479a70
void GameOptions::WriteScreenSize(int nValue)
{
    WriteInt(m_pszSection, m_pszScreenSize, nValue);
}

// FUNCTION: HYPERX 0x00479a90
unsigned int GameOptions::ReadPeriodLength()
{
    return ReadInt(m_pszSection, m_pszPeriodLength, 0x493e0);
}

// FUNCTION: HYPERX 0x00479ab0
void GameOptions::WritePeriodLength(int nValue)
{
    WriteInt(m_pszSection, m_pszPeriodLength, nValue);
}
