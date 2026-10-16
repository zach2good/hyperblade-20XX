#ifndef HYPERBLADE_PLANVM_H
#define HYPERBLADE_PLANVM_H

#include "planstate.h"
#include "interactor.h"

class PlanState;

#include "planvm/planutil.h"
#include "planvm/plannode.h"

struct PlanNode;

#define PLANVM_BEHAVIOR_LIMIT        60
#define PLANVM_INTERACTOR_NAME_LIMIT 150
#define PLANVM_MESSAGES_PER_BEHAVIOR 20

// A keyword table entry: the name, a sentinel word, and the value the name stands for.
struct PlanOpcodeRef
{
    char         szName[0x20];
    unsigned int nMagic;
    unsigned int nValue;
};

// The plan machine the editor dialog is handed and stores in its own user data word.
class PlanVM
{
public:
    // Free functions in the original, made static members named by the class.
    static void __cdecl InitRuntimeOpcodeTable(void);
    static void         LoadAnimationLists(void);

public:
    PlanVM();
    short         SkipLine();
    PlanOpcodeRef LookupSymbol(char* pszName);
    short         FindLocalRealIndex(char* pszName);
    short         FindInteractorNameIndex(char* pszName);
    PlanOpcodeRef LookupKeyword(char* pszName);

    short FindBehaviorIndexOrThrow(char* pszName);
    short FindInteractorNameIndexOrThrow(char* pszName);

    int           IsSeparatorChar();
    int           IsCommentOrGroup();
    int           IsAAPrefix(const char* pszToken);
    unsigned char IsIdentifierPrefix(const char* pszToken);
    unsigned char IsNumericChar(const char* pszToken);
    unsigned char IsPRN(const char* pOpcode);
    char          ParseNumber(char* pszText, union PlanWord* pOut);
    char          EmitStringLiteral(const char* pszToken, union PlanWord* paStream, int* pnPos);
    short         SkipSeparators();

    bool AtEnd() const
    {
        return m_nCursor > m_nLength || m_pszText[m_nCursor] == 0;
    }

    bool AtLineEnd() const
    {
        const char cAt = m_pszText[m_nCursor];

        return cAt == '\r' || cAt == '\n';
    }

    void ReadNextToken(char* pToken);

    char          LoadText(short nCamera, char* pszName);
    char          ReadAll(class FileObj* pFile);
    unsigned char Compile();
    char          SaveBinary(char* pszName);

    char EmitArgToken();

    void LoadSymbolTables();

    short           m_nCamera; // 0x00
    int             m_nLength; // 0x04
    int             m_nCursor; // 0x08
    int             m_nPos;    // 0x0c
    char*           m_pszText; // 0x10
    union PlanWord* m_pStream; // 0x14

    // The plan's declared names: behaviours, local reals and interactors; messages are below.
    char m_aBehaviorNames[PLANVM_BEHAVIOR_LIMIT][0x20];          // 0x00018
    char m_aLocalRealNames[PLANVM_BEHAVIOR_LIMIT][0x20];         // 0x00798
    char m_aInteractorNames[PLANVM_INTERACTOR_NAME_LIMIT][0x20]; // 0x00f18

private:
    int m_nUnused; // 0x021d8

public:
    // The instruction keywords, ending in the empty-name entry; the constructor fills 28.
    PlanOpcodeRef m_aOpcodes[180]; // 0x021dc

    // Six named tokens, then _0_ to _50_, then the terminator.
    PlanOpcodeRef m_aTokenRefs[80]; // 0x03dfc

private:
    // LoadSymbolTables never checks the slot against the capacity.
    PlanOpcodeRef m_aSymbols[2100]; // 0x04a7c

public:
    // Each behaviour's SET_MESSAGE_INTERACTOR ids, PLANVM_MESSAGES_PER_BEHAVIOR to a row.
    char m_aMessageNames[PLANVM_BEHAVIOR_LIMIT * PLANVM_MESSAGES_PER_BEHAVIOR][0x20]; // 0x1929c

    ~PlanVM();

    char ParseHeaderDeclarations();

    static char* UnquoteString(char* pszText);
};

// The stat handlers, indexed by the packed argument's low word; each answers a slot index.
typedef short(__cdecl* PlanStatFn)(class EngineObject* pActor, PlanNode* pArgs);
// Two callers read the whole return register, so they call through this view of the table.
typedef int(__cdecl* PlanStatFnWide)(class EngineObject* pActor, PlanNode* pArgs);

// GLOBAL: HYPERX 0x004d1798
extern PlanStatFn g_apPlanStatFns[PRN_COUNT];

#endif
