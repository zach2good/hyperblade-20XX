// The script table: 0x4c0 bytes holding three hundred script databases.

#ifndef HYPERBLADE_ANIMSCRIPTTABLE_H
#define HYPERBLADE_ANIMSCRIPTTABLE_H

#include "common/compat.h"
#include "anim/animset.h"

class AnimDB;
class String;

struct AnimScriptHead
{
    AnimScriptHead()
    : m_nHead(0)
    {
    }

    int m_nHead;
};

// Its table at 0x004b22d4 is one slot: the destructor and nothing else.
// VTABLE: HYPERX 0x004b22d4
class AnimScriptTable
{
public:
    virtual ~AnimScriptTable();

    AnimScriptTable()
    : m_pPlanVm(nullptr)
    {
        memset(m_apEntries, 0, sizeof(m_apEntries));

        m_nCount = 0;
    }

    int CreateAnimDB(AnimDB** ppOut, const char* pszName, const char* pszExtra);
    int ParseAndAddAnimDB(AnimDB** ppOut, const char* pszSource, const char** ppszCursor, const String& strFilter);

    // The registry is an argument, so the parse binds against an object it does not own.
    void ParseScript(const char* pszPath, class AnimRegistry* pAnimMgr, int bBind);

    unsigned int ParseScriptLine(const char* pszLine, struct AnimScriptState* pState);
    int          ParseSegment(class AnimNode** ppSegment, const char* pszLine, const char** ppszCursor);
    int          ParseTrigger(const char* pszLine, short* pnSegment, short* pnTriggerFrame, char* pszName, char* pszRef, short* pnSyncSegment, short* pnSyncFrame);

    // Each reads the next token and scans one value from it; nonzero when both succeed.
    static int ScanShort(const char** ppszCursor, char* pszToken, short* pnOut);
    static int ScanFloat(const char** ppszCursor, char* pszToken, float* pflOut);

    AnimDB* GetEntry(short iEntry) const
    {
        return m_apEntries[iEntry];
    }

    // The original reads index -1 as the zero word before the table, so any index outside it is null.
    AnimDB* Find(int nIndex) const
    {
        if (nIndex < 0 || nIndex >= m_nCount)
        {
            return nullptr;
        }

        return m_apEntries[nIndex];
    }

    AnimScriptHead m_headWord;                   // 0x004
    AnimDB*        m_apEntries[kAnimsetEntries]; // 0x008  three hundred script databases, 0x68 each
    short          m_nCount;                     // 0x4b8  how many are filled
    // The plan VM the parser builds the script into.
    class PlanVM* m_pPlanVm; // 0x4bc
};

// SYNTHETIC: HYPERX 0x00418bf0
// AnimScriptTable::`scalar deleting destructor'

#endif
