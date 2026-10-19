// Opening a compiled plan and reading it in, with our fixed version from the pak; both failures
// report and neither throws.

#include "common/compat.h"
#include "common/path.h"
#include "globals.h"
#include "planvmglobals.h"
#include "planfile.h"
#include "plannode.h"
#include "common/pathstring.h"
#include "common/pak.h"
#include "planvm/planpatch.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
#include <string>
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

// The pak's fixes for the plan, compiled once a run. Nothing when the pak has none for it.
const std::optional<std::vector<int> >& FixedPlan(const char* pszName, std::span<const int> words)
{
    static auto s_fixed = std::map<std::string, std::optional<std::vector<int> > >();

    auto strName = std::string(pszName);

    std::ranges::transform(
        strName,
        strName.begin(),
        [](unsigned char c)
        {
            return static_cast<char>(std::toupper(c));
        });

    if (const auto found = s_fixed.find(strName); found != s_fixed.end())
    {
        return found->second;
    }

    auto& fixed = s_fixed[strName];

    const auto diff = Pak::Read("plans/" + strName + ".diff");

    if (diff.empty())
    {
        return fixed;
    }

    static const auto s_compiler = PlanPatch::Compiler::FromDataDir(Path::Native(g_gameApp.m_strExeDir.m_str.m_pchData));

    if (s_compiler.has_value())
    {
        fixed = s_compiler->Fixed(words, std::string_view(reinterpret_cast<const char*>(diff.data()), diff.size()));
    }

    if (!fixed.has_value())
    {
        SDL_Log("The fixes for plan %s did not apply, so it runs as shipped.", strName.c_str());
    }

    return fixed;
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

    if (const auto& fixed = FixedPlan(pszName, words); fixed.has_value())
    {
        blobs.pFixed = Widen(*fixed);
    }

    return blobs;
}
