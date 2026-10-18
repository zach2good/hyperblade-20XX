#include "common/compat.h"
#include "common/animnamedref.h"
#include <cstring>
#include <new>

// FUNCTION: HYPERX 0x00417880
AnimNamedRef::AnimNamedRef(const char* pszName, short nSegment, short nFrame)
{
    m_nFrame = nFrame;

    m_pszName = strcpy(new char[strlen(pszName) + 1], pszName);

    const int nAdjusted = nSegment + ANIMNAMEDREF_BIAS;

    m_nSegment = (short)nAdjusted;
}

// Only a reference still carrying the bias owns the pointer; a second call is harmless.

// FUNCTION: HYPERX 0x004178e0
AnimNamedRef::~AnimNamedRef()
{
    if (m_nSegment >= ANIMNAMEDREF_BIAS)
    {
        if (m_pszName != nullptr)
        {
            delete[] m_pszName;

            m_pszName = nullptr;
        }
    }
}
