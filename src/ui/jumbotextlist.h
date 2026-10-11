#ifndef HYPERBLADE_JUMBOTEXTLIST_H
#define HYPERBLADE_JUMBOTEXTLIST_H

class JumboTextList
{
public:
    JumboTextList()
    {
        Init();
    }

    ~JumboTextList()
    {
        FreeAll();
    }

    JumboTextList* Init();

    void         FreeAll();
    unsigned int Load(const char* pszFileName);
    int          GetRandom(char* pszOut, int nMax);

    char** m_ppszText; // 0x00
    short  m_nCount;   // 0x04
};

#endif
