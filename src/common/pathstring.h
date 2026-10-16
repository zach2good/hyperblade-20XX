// Strings built on String: the ones that reserve or fill inside their own constructor.

#ifndef HYPERBLADE_PATHSTRING_H
#define HYPERBLADE_PATHSTRING_H
#include "common/compat.h"

#include "common/string.h"
#include <cstring>

class PathString
{
public:
    explicit PathString(int nReserve)
    {
        m_str.Reserve(nReserve);
    }

    PathString(int nReserve, const char* pszDirectory, const char* pszName, const char* pszExtension)
    {
        m_str.Reserve(nReserve);
        m_str.FormatResourcePath(pszDirectory, pszName, pszExtension);
    }

    PathString(LPCSTR psz, int nReserve)
    {
        int nLength;

        m_str.Reserve(nReserve);

        if (psz == 0)
        {
            nLength = 0;
        }
        else
        {
            nLength = (int)strlen(psz);
        }

        m_str.Assign(nLength, psz);
    }

    String m_str;
};

#endif
