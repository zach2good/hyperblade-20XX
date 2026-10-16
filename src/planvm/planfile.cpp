// Opening a compiled plan and reading it in; both failures report and neither throws.

#include "common/compat.h"
#include "globals.h"
#include "planvmglobals.h"
#include "planfile.h"
#include "plannode.h"
#include "common/pathstring.h"
#include <stdio.h>
#include "app/gameapp.h"

// A truncated file yields a short plan rather than a refusal.
// FUNCTION: HYPERX 0x00436200
PlanWord* PlanFile::ReadBinary(FILE* pFile, unsigned int* pnCount)
{
    if (fread(pnCount, 4, 1, pFile) != 1)
    {
        // STRING: HYPERX 0x004bdf94
        g_gameApp.Error("Misread binary plan");
    }

    // The file holds 4-byte words; a PlanWord is pointer-sized, so each is widened as it is read.
    PlanWord* const pInstructions = new PlanWord[*pnCount]();
    auto* const     pWords        = new int[*pnCount]();

    if (fread(pWords, sizeof(int), *pnCount, pFile) != *pnCount)
    {
        // STRING: HYPERX 0x004bdf94
        g_gameApp.Error("Misread binary plan");
    }

    for (unsigned int i = 0; i < *pnCount; i++)
    {
        pInstructions[i].n = pWords[i];
    }

    delete[] pWords;

    return pInstructions;
}

// FUNCTION: HYPERX 0x00436270
PlanWord* PlanFile::OpenBinary(const char* pszName, void**)
{
    PathString   strPath(MAX_PATH, g_gameApp.m_strPlansPath.m_str.m_pchData, pszName, g_pszPbnExtension);
    unsigned int nCount;

    // STRING: HYPERX 0x004bc938
    FILE* const pFile = fopen(strPath.m_str.m_pchData, "rb");

    if (pFile != 0)
    {
        PlanWord* const pInstructions = PlanFile::ReadBinary(pFile, &nCount);

        fclose(pFile);

        if (pInstructions == 0)
        {
            // STRING: HYPERX 0x004bdfcc
            g_gameApp.Error("Inability to read plan %s", pszName);
        }

        return pInstructions;
    }

    // STRING: HYPERX 0x004bdfa8
    g_gameApp.Error("Binary Plan %s could not be opened!", pszName);

    return 0;
}
