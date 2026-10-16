// The game options object: ini names per setting, and their readers and writers.

#ifndef HYPERBLADE_GAMEOPTIONS_H
#define HYPERBLADE_GAMEOPTIONS_H

class GameOptions
{
public:
    // Never reads the object.
    unsigned int ReadInt(const char* pszSection, const char* pszKey, int nDefault);

    int  WriteString(const char* pszSection, const char* pszKey, const char* pszValue);
    void WriteInt(const char* pszSection, const char* pszKey, int nValue);

    // The engine's constructor copies difficulty, brutality and period length into itself.
    unsigned int ReadDifficulty();
    unsigned int ReadBrutality();
    unsigned int ReadSfxVolume();
    unsigned int ReadMusicVolume();
    unsigned int ReadPeriodLength();
    unsigned int ReadScreenSize();

    void WriteDifficulty(int nValue);
    void WriteBrutality(int nValue);
    void WritePeriodLength(int nValue);
    void WriteSfxVolume(int nValue);
    void WriteMusicVolume(int nValue);
    void WriteScreenSize(int nValue);

    GameOptions();
    // Empty; the original linker folded it onto Stub::Assert.
    ~GameOptions();

    // A table of ini names; the section is at 0x00.
    const char* m_pszSection;      // 0x00
    const char* m_pszDifficulty;   // 0x04
    const char* m_pszBrutality;    // 0x08
    const char* m_pszSfxVolume;    // 0x0c
    const char* m_pszMusicVolume;  // 0x10
    const char* m_pszScreenSize;   // 0x14
    const char* m_pszPeriodLength; // 0x18
    // Not a name: the install level, read from the Misc section.
    int m_nInstallLevel; // 0x1c
};

#endif
