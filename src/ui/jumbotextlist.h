#ifndef HYPERBLADE_JUMBOTEXTLIST_H
#define HYPERBLADE_JUMBOTEXTLIST_H

#include <string>
#include <vector>

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

    std::vector<std::string> m_rows;
};

#endif
