// Resolves Windows-style game paths against a folder whose names differ in case.

#include "common/path.h"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

static int s_nFailed = 0;

static void Expect(bool bOK, const char* pszWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", pszWhat);
        s_nFailed++;
    }
}

int main()
{
    const auto directory = std::filesystem::temp_directory_path() / "hyperx_path_check";

    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory / "Data" / "Music");
    std::ofstream(directory / "Data" / "DftKeybd.CFG") << "x";

    const auto base = directory.string() + "\\";

    const auto found = Path::Native((base + "data\\DFTKEYBD.cfg").c_str());

    Expect(found.find('\\') == std::string::npos, "backslashes become slashes");
    Expect(std::filesystem::exists(found), "a file named in another case is found");

    const auto created = Path::Native((base + "DATA\\music\\Track02.flac").c_str());

    Expect(std::filesystem::exists(std::filesystem::path(created).parent_path()), "a missing file keeps its found folder");
    Expect(created.ends_with("/Track02.flac"), "a missing file keeps its own name");

    std::filesystem::remove_all(directory);

    return s_nFailed;
}
