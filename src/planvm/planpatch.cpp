// The plan fixes in res/plans are unified diffs against libadlib's decompiled source.

#include "planvm/planpatch.h"
#include <algorithm>
#include <charconv>

namespace
{

// The text's lines without their line ends; a final line end adds no empty line.
std::vector<std::string_view> Lines(std::string_view text)
{
    auto lines = std::vector<std::string_view>();

    while (!text.empty())
    {
        const auto nEnd = text.find('\n');
        auto       line = text.substr(0, nEnd);

        if (!line.empty() && line.back() == '\r')
        {
            line.remove_suffix(1);
        }

        lines.push_back(line);

        if (nEnd == std::string_view::npos)
        {
            break;
        }

        text.remove_prefix(nEnd + 1);
    }

    return lines;
}

// How many old lines come before a "@@ -a,b +c,d @@" hunk. Line a is its first old line, or the
// line it follows when b is nought.
std::optional<size_t> LinesBeforeHunk(std::string_view header)
{
    if (!header.starts_with("@@ -"))
    {
        return std::nullopt;
    }

    const auto* const pEnd = header.data() + header.size();

    auto       nStart = size_t{ 0 };
    const auto start  = std::from_chars(header.data() + 4, pEnd, nStart);

    if (start.ec != std::errc{})
    {
        return std::nullopt;
    }

    auto nCount = size_t{ 1 };

    if (start.ptr != pEnd && *start.ptr == ',' && std::from_chars(start.ptr + 1, pEnd, nCount).ec != std::errc{})
    {
        return std::nullopt;
    }

    if (nCount == 0 || nStart == 0)
    {
        return nStart;
    }

    return nStart - 1;
}

std::vector<uint32_t> Unsigned(std::span<const int> words)
{
    return std::vector<uint32_t>(words.begin(), words.end());
}

} // namespace

std::optional<std::string> PlanPatch::ApplyDiff(std::string_view text, std::string_view diff)
{
    const auto original = Lines(text);
    auto       patched  = std::string();
    auto       nCursor  = size_t{ 0 };
    auto       bInHunk  = false;

    const auto copyUpTo = [&](size_t nLine)
    {
        for (; nCursor < nLine; nCursor++)
        {
            patched.append(original[nCursor]);
            patched.push_back('\n');
        }
    };

    for (const auto line : Lines(diff))
    {
        if (line.starts_with("@@"))
        {
            const auto nBefore = LinesBeforeHunk(line);

            if (!nBefore.has_value() || *nBefore > original.size() || *nBefore < nCursor)
            {
                return std::nullopt;
            }

            copyUpTo(*nBefore);
            bInHunk = true;

            continue;
        }

        if (!bInHunk || line.starts_with("\\"))
        {
            continue;
        }

        // Some editors strip the space from an empty context line.
        const auto cKind = [&]
        {
            if (line.empty())
            {
                return ' ';
            }

            return line.front();
        }();

        const auto body = line.substr(std::min<size_t>(1, line.size()));

        if (cKind == '+')
        {
            patched.append(body);
            patched.push_back('\n');

            continue;
        }

        if ((cKind != ' ' && cKind != '-') || nCursor >= original.size() || original[nCursor] != body)
        {
            return std::nullopt;
        }

        if (cKind == ' ')
        {
            patched.append(body);
            patched.push_back('\n');
        }

        nCursor++;
    }

    copyUpTo(original.size());

    return patched;
}

std::optional<PlanPatch::Compiler> PlanPatch::Compiler::FromDataDir(const std::string& strDir)
{
    auto compiler = Compiler();
    auto strError = std::string();

    if (!adlib::tools::loadCompilerSymbolsDir(compiler.m_decompileSymbols, strDir, &strError))
    {
        return std::nullopt;
    }

    compiler.m_compileSymbols = adlib::CompilerSymbols::fromDataDir(strDir, &strError);

    if (!strError.empty())
    {
        return std::nullopt;
    }

    return compiler;
}

std::optional<std::string> PlanPatch::Compiler::Decompile(std::span<const int> words) const
{
    auto options   = adlib::tools::DecompileOptions{};
    options.header = false;

    auto source   = std::string();
    auto strError = std::string();

    if (!adlib::tools::decompile(Unsigned(words), m_decompileSymbols, options, source, &strError))
    {
        return std::nullopt;
    }

    return source;
}

std::optional<std::vector<int> > PlanPatch::Compiler::Compile(std::string_view source) const
{
    const auto result = adlib::compilePlan(source, m_compileSymbols);

    if (!result.ok)
    {
        return std::nullopt;
    }

    return std::vector<int>(result.words.begin(), result.words.end());
}

std::optional<std::vector<int> > PlanPatch::Compiler::Fixed(std::span<const int> words, std::string_view diff) const
{
    const auto source = Decompile(words);

    if (!source.has_value())
    {
        return std::nullopt;
    }

    const auto shipped = Compile(*source);

    if (!shipped.has_value() || !std::ranges::equal(*shipped, words))
    {
        return std::nullopt;
    }

    const auto patched = ApplyDiff(*source, diff);

    if (!patched.has_value())
    {
        return std::nullopt;
    }

    return Compile(*patched);
}
