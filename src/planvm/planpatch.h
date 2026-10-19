#ifndef HYPERBLADE_PLANPATCH_H
#define HYPERBLADE_PLANPATCH_H

#include <adlib/compiler.h>
#include <adlib/tools.h>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PlanPatch
{

// Applies a unified diff to text. Nothing when a hunk's old lines are not where it says.
std::optional<std::string> ApplyDiff(std::string_view text, std::string_view diff);

// libadlib's decompiler and compiler, with the game's own symbol files.
class Compiler
{
public:
    // Reads enumIDs.h, AnimData/Animassm.txt, Sndfiles.lst and Infobtxt.lst from the Data folder.
    static std::optional<Compiler> FromDataDir(const std::string& strDir);

    std::optional<std::string>       Decompile(std::span<const int> words) const;
    std::optional<std::vector<int> > Compile(std::string_view source) const;

    // The plan decompiled, patched and compiled again. Nothing when the plan does not recompile
    // to its own words, the diff does not apply or the result does not compile.
    std::optional<std::vector<int> > Fixed(std::span<const int> words, std::string_view diff) const;

private:
    adlib::tools::SymbolTable m_decompileSymbols;
    adlib::CompilerSymbols    m_compileSymbols;
};

} // namespace PlanPatch

#endif
