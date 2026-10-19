// Fails on any raw new, delete, malloc family call or shared_ptr in the given folders. Ownership goes
// through std::unique_ptr, and SDL's own allocations through the deleter in common/sdlmemory.
//     rawalloc_check <folder>...

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>

namespace
{

// Blanks out comments and string and character literals, keeping the line breaks.
std::string StripNonCode(const std::string& source)
{
    enum class State
    {
        kCode,
        kLineComment,
        kBlockComment,
        kString,
        kChar
    };

    auto out   = source;
    auto state = State::kCode;

    for (size_t i = 0; i < out.size(); i++)
    {
        const auto ch   = source[i];
        const auto next = [&]
        {
            if (i + 1 < source.size())
            {
                return source[i + 1];
            }

            return '\0';
        }();

        switch (state)
        {
            case State::kCode:
                if (ch == '/' && next == '/')
                {
                    state = State::kLineComment;
                }
                else if (ch == '/' && next == '*')
                {
                    state = State::kBlockComment;
                }
                else if (ch == '"')
                {
                    state = State::kString;
                }
                else if (ch == '\'')
                {
                    state = State::kChar;
                }

                continue;

            case State::kLineComment:
                if (ch == '\n')
                {
                    state = State::kCode;

                    continue;
                }

                break;

            case State::kBlockComment:
                if (ch == '*' && next == '/')
                {
                    out[i + 1] = ' ';
                    i++;
                    state = State::kCode;
                }

                break;

            case State::kString:
            case State::kChar:
                if (ch == '\\')
                {
                    out[i] = ' ';

                    if (i + 1 < out.size() && out[i + 1] != '\n')
                    {
                        out[i + 1] = ' ';
                    }

                    i++;

                    continue;
                }

                if ((state == State::kString && ch == '"') || (state == State::kChar && ch == '\''))
                {
                    state = State::kCode;

                    continue;
                }

                break;
        }

        if (ch != '\n')
        {
            out[i] = ' ';
        }
    }

    return out;
}

} // namespace

int main(int argc, char** argv)
{
    const auto pattern = std::regex(
        R"((^|[^\w.>])(new|delete|malloc|calloc|realloc|free|strdup|_strdup|SDL_malloc|SDL_calloc|SDL_realloc|SDL_free|SDL_strdup|shared_ptr|make_shared)\b)");
    const auto deletedFunction = std::regex(R"(=\s*delete\b)");
    const auto preprocessor    = std::regex(R"(^\s*#)");

    auto nFound = 0;

    for (int nArg = 1; nArg < argc; nArg++)
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(argv[nArg]))
        {
            const auto& path = entry.path();

            if (!entry.is_regular_file() || (path.extension() != ".cpp" && path.extension() != ".h"))
            {
                continue;
            }

            if (path.stem() == "sdlmemory" || path.filename() == "rawalloc_check.cpp")
            {
                continue;
            }

            auto stream = std::ifstream(path, std::ios::binary);
            auto buffer = std::stringstream();
            buffer << stream.rdbuf();

            auto code     = std::istringstream(StripNonCode(buffer.str()));
            auto original = std::istringstream(buffer.str());
            auto line     = std::string();
            auto text     = std::string();
            auto nLine    = 0;

            while (std::getline(code, line) && std::getline(original, text))
            {
                nLine++;

                if (std::regex_search(line, preprocessor))
                {
                    continue;
                }

                const auto stripped = std::regex_replace(line, deletedFunction, "");

                if (std::regex_search(stripped, pattern))
                {
                    std::printf("%s:%d: %s\n", path.generic_string().c_str(), nLine, text.c_str());
                    nFound++;
                }
            }
        }
    }

    std::printf("%d raw allocation sites\n", nFound);

    return static_cast<int>(nFound != 0);
}
