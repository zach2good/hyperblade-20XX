// The original's reference-counted string, laid out as MFC's CString.

#ifndef HYPERBLADE_STRING_H
#define HYPERBLADE_STRING_H

#include "common/compat.h"
#include <stdarg.h>
#include <cstring>
#include <memory>

struct StringHeader
{
    int nRefs;
    int nDataLength;
    int nAllocLength;

    // The characters follow the header.
    char* Data()
    {
        return reinterpret_cast<char*>(this + 1);
    }
};

// String finds a header 12 bytes before its data, so the empty string's two parts are one record.
struct EmptyString
{
    StringHeader header;
    char         szData[4];
};

// Reference count minus one, so nothing ever frees it.
// GLOBAL: HYPERX 0x004bcf2c
extern EmptyString g_emptyString;

class String
{
public:
    // Free functions in the original, made static members named by the class.
    static int    ExtractToken(const char* pszSource, char* pszDest, const char** ppszNext);
    static String Concat(const String& source, const char* pszSuffix);
    static void   Format(String* pString, const char* pszFormat, ...);
    static char** GetGlobalStringPtr();

public:
    String()
    {
        m_pchData = *GetGlobalStringPtr();
    }

    explicit String(int nReserve)
    {
        m_pchData = *GetGlobalStringPtr();
        Reserve(nReserve);
    }

    // 0x0042f3e0; it initialises m_pchData first, so it is the copy constructor.
    String(const String& src);

    String(String&& src) noexcept;
    String& operator=(const String& src);
    String& operator=(String&& src) noexcept;

    // 0x0042f420; not explicit, because callers convert from a C string.
    String(const char* psz);

    // FUNCTION: HYPERX 0x004197b0
    ~String() = default;

    operator const char*() const
    {
        return m_pchData;
    }

    // The header sits just before the characters.
    static StringHeader* HeaderOf(char* pch)
    {
        return reinterpret_cast<StringHeader*>(pch) - 1;
    }

    StringHeader* GetHeader() const
    {
        return HeaderOf(m_pchData);
    }

    int IsEmpty() const
    {
        return GetHeader()->nDataLength == 0;
    }

    void    FreeBuffer();
    String* AssignFromString(const String& src);
    char*   Resize(int nLength);
    void    SetLength(int nLength);

    void ClearAndFree();
    void AllocBuffer(int nLength);
    void PrepareForWrite(int nLength);
    void Assign(unsigned int nLength, const char* pchData);
    void MakeUnique();

    char* Reserve(int nLength);
    void  Empty();
    void  ConcatAlloc(int nLength1, const char* pchData1, int nLength2, const char* pchData2);
    bool  AssignResourceId(unsigned int nID);
    void  AppendData(int nLength, const char* pchData);

    // Answers the string it was called on.
    String* FormatResourcePath(const char* pszDirectory, const char* pszName, const char* pszExtension);

    char* m_pchData;

    // MFC's SafeStrlen.
    static int SafeStrlen(const char* psz)
    {
        if (psz == nullptr)
        {
            return 0;
        }

        return static_cast<int>(strlen(psz));
    }

    static void FormatAlloc(String* pString, const char* pszFormat, va_list argList);

private:
    // The header and the characters, or null while m_pchData is the shared empty string.
    std::unique_ptr<char[]> m_pBlock;
};

// Each caller now frees the old block by moving it into a local.
// FUNCTION: HYPERX 0x0042f700
// String::FreeHeader

#endif
