
#ifndef HYPERBLADE_TEAMSBLOCK_H
#define HYPERBLADE_TEAMSBLOCK_H

struct TeamBlock;

// The team catalogue, 0x8c bytes: the team blocks and their count.
struct TeamsBlock
{
    const short& GetTeamCount() const
    {
        return m_wTeamCount;
    }

    // Nought past the last team; teamblock.h defines both.
    const char* GetTeamPath(int nAt) const;
    const char* GetTeamEnvironment(int nAt) const;

    // Folds every team's name into a checksum and stores it in m_nChecksum.
    int ComputeTeamNameChecksum();

    struct TeamBlock** m_ppTeams;    // 0x00
    short              m_wTeamCount; // 0x04
    short              m_wUnused;    // 0x06

    union
    {
        int           m_nChecksum; // 0x08
        unsigned char m_abChecksum[4];
    };

    int m_anUnused[(0x8c - 0x0c) / 4]; // 0x0c

    TeamsBlock();
};

#endif
