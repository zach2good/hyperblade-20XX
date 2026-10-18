#ifndef HYPERBLADE_SAVEOBJECT_H
#define HYPERBLADE_SAVEOBJECT_H

// Twenty rounds of the gauntlet bracket, twenty team slots each.
constexpr auto kGauntletRounds     = 0x14;
constexpr auto kGauntletRoundSlots = 0x14;

// The object the engine keeps in m_pSaveObject.
class SaveObject
{
public:
    SaveObject(short nHiddenTeamA, short nHiddenTeamB);

    unsigned int ParseGauntletConfig(const char* pszFileName);
    void         SetAllTeamsAvailable(int nAvailable);

    // The team in each round's slot, minus one where the config leaves it empty.
    short m_aBracket[kGauntletRounds][kGauntletRoundSlots]; // 0x000

    short m_wBracketPos; // 0x320
    // One means every team is available.
    int   m_nAllTeamsAvailable; // 0x324
    short m_wHiddenTeamA;       // 0x328
    short m_wHiddenTeamB;       // 0x32a

    short GetHomeTeam() const;
    short GetBracketPos() const;
    short BracketIterPeek();
    short BracketIterAdvance();
    void  BracketIterReset();
    void  SetBracketPos(short nPos);
    void  ValidateSetHomeTeam(short nTeam, int nFlag);
    short WrapToNextValidTeam(short nTeam, int nDirection);
};

#endif
