#include "common/compat.h"
#include "teamblock.h"
#include <new>
#include <string.h>
#include "objectdef.h"

// FUNCTION: HYPERX 0x00484730
TeamBlock::TeamBlock()
{
    szTsAnim[0]      = 0;
    szTsTitle[0]     = 0;
    szTsText[0]      = 0;
    szTsLogo[0]      = 0;
    szIdLogo[0]      = 0;
    szDromeExt[0]    = 0;
    szEnvironment[0] = 0;
    szName[0]        = 0;
    szPath[0]        = 0;

    memset(apDefs, 0, sizeof(apDefs));
    memset(apOnIce, 0, sizeof(apOnIce));
    memset(aStatRows, 0, sizeof(aStatRows));
    memset(aRows, 0, sizeof(aRows));
}

// The slot keeps the freed pointer: this runs at shutdown and nothing reads the table again.

// FUNCTION: HYPERX 0x004847a0
TeamBlock::~TeamBlock()
{
    for (int nIndex = 0; nIndex < TEAM_BLOCK_SLOTS; nIndex++)
    {
        delete apDefs[nIndex];
    }
}
