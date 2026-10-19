// Opening a compiled plan and reading it in, with its fixed version; both failures report and neither throws.

#include "common/compat.h"
#include "common/path.h"
#include "globals.h"
#include "planvmglobals.h"
#include "planfile.h"
#include "plannode.h"
#include "common/pathstring.h"
#include "planvm/planpatch.h"
#include <cstdio>
#include <vector>
#include "app/gameapp.h"

namespace
{

// The file holds 4-byte words; a PlanWord is pointer-sized, so each is widened.
std::unique_ptr<PlanWord[]> Widen(std::span<const int> words)
{
    auto pInstructions = std::make_unique<PlanWord[]>(words.size());

    for (size_t i = 0; i < words.size(); i++)
    {
        pInstructions[i].n = words[i];
    }

    return pInstructions;
}

} // namespace

// A truncated file yields a short plan rather than a refusal.
// FUNCTION: HYPERX 0x00436200
std::vector<int> PlanFile::ReadBinary(FILE* pFile)
{
    unsigned int nCount = 0;

    if (fread(&nCount, 4, 1, pFile) != 1)
    {
        // STRING: HYPERX 0x004bdf94
        g_gameApp.Error("Misread binary plan");
    }

    auto words = std::vector<int>(nCount);

    if (fread(words.data(), sizeof(int), nCount, pFile) != nCount)
    {
        // STRING: HYPERX 0x004bdf94
        g_gameApp.Error("Misread binary plan");
    }

    return words;
}

// FUNCTION: HYPERX 0x00436270
PlanFile::Blobs PlanFile::OpenBinary(const char* pszName)
{
    PathString strPath(kMaxPathLength, g_gameApp.m_strPlansPath.m_str.m_pchData, pszName, g_pszPbnExtension);

    // STRING: HYPERX 0x004bc938
    auto* const pFile = fopen(Path::Native(strPath.m_str.m_pchData).c_str(), "rb");

    if (pFile == nullptr)
    {
        // STRING: HYPERX 0x004bdfa8
        g_gameApp.Error("Binary Plan %s could not be opened!", pszName);

        return {};
    }

    const auto words = PlanFile::ReadBinary(pFile);

    fclose(pFile);

    auto blobs = Blobs{ Widen(words), nullptr };

    if (const auto fixed = PlanPatch::Fixed(pszName, words))
    {
        blobs.pFixed = Widen(*fixed);
    }

    return blobs;
}
