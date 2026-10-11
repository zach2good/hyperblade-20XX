// The bring-up build's _purecall, which replaces LIBCMT's.

#include "common/compat.h"
#include "app/bringup.h"

#ifdef HYPERX_BRINGUP

#define PURE_STACK_WORDS 0x100
#define PURE_STACK_HITS  8

// Traces the stack first: the R6025 box waits for a click, and a later exception loses the caller.
extern "C" int __cdecl _purecall(void)
{
    unsigned int  nAnchor;
    unsigned int* pStack;
    int           nStep;
    int           nFound;

    nAnchor = 0;
    pStack  = &nAnchor;
    nFound  = 0;

    for (nStep = 1; nStep < PURE_STACK_WORDS && nFound < PURE_STACK_HITS; nStep++)
    {
        unsigned int nWord = pStack[nStep];

        if (nWord > 0x00401000 && nWord < 0x004b0000)
        {
            BringUp::Trace("pureCallStack", (int)nWord);

            nFound++;
        }
    }

    RaiseException(0xe0000025, EXCEPTION_NONCONTINUABLE, 0, 0);

    return 0;
}

#endif
