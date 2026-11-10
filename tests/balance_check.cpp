// The balance proposals against a small made-up Machine: medians, aliases, dromer pairing and
// rounding, with no game data.

#include "app/gameapp.h"
#include "engine/balance.h"
#include "engine/gameoptions.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <print>
#include <string>

// Stand-ins for the game's own, which bring the whole game with them.
std::unique_ptr<GameOptions> g_pGameOptions;

GameOptions::~GameOptions()
{
}

unsigned int GameOptions::ReadInt(const char*, const char*, int nDefault)
{
    return static_cast<unsigned int>(nDefault);
}

void GameOptions::WriteInt(const char*, const char*, int)
{
}

char* GameApp::ResolveCDRomPath(char* pszPath)
{
    return pszPath;
}

static int s_nFailed = 0;

static void Expect(int nGot, int nWanted, const char* pszWhat)
{
    if (nGot != nWanted)
    {
        std::println("failed: {}, got {}, wanted {}", pszWhat, nGot, nWanted);
        s_nFailed++;
    }
}

int main()
{
    const auto folder = std::filesystem::temp_directory_path() / "hyperblade_balance_check";

    std::filesystem::create_directories(folder);

    {
        auto file = std::ofstream(folder / "CHICO.HTM", std::ios::binary | std::ios::trunc);

        file << "DefineObject type=Dromer name=First\r\n"
                "SetRatings1\tid_RAT_Speed = 3\r\n"
                "SetRatings2\t id_RAT_Checking    = 4\t\tid_RAT_Stamina = 3\r\n"
                "SetRatings2\t id_RAT_Stealing = 6\r\n"
                "DefineObject type=Dromer name=Second\r\n"
                "SetRatings2\t id_RAT_Checking    = 7\t\tid_RAT_Stamina = 8\r\n"
                "SetRatings2\t id_RAT_RokHandling = 9\r\n";
    }

    const auto strNemesis = (folder / "NEM.HTM").string();
    const auto strBottom  = (folder / "MUNIC.HTM").string();
    const auto strOther   = (folder / "LEEDS.HTM").string();

    const auto nFirst  = 0;
    const auto nSecond = 0;

    Balance::Set(false);
    Balance::BeginTeam(strNemesis.c_str());
    Expect(Balance::Adjust(&nFirst, "id_RAT_Checking", 1, 12), 12, "switch off");
    Balance::EndTeam();

    Balance::Set(true);

    Balance::BeginTeam(strOther.c_str());
    Expect(Balance::Adjust(&nFirst, "id_RAT_Checking", 1, 12), 12, "a team with no proposal");
    Balance::EndTeam();

    // 5.5 + 0.6875 * (12 - 5.5) = 9.97
    Balance::BeginTeam(strNemesis.c_str());
    Expect(Balance::Adjust(&nFirst, "id_RAT_Checking", 1, 12), 10, "Nemesis toward the median");
    // Stealing counts as RokHandling: 7.5 + 0.6875 * (11 - 7.5) = 9.91
    Expect(Balance::Adjust(&nFirst, "id_RAT_RokHandling", 1, 11), 10, "an alias joins its rating's median");
    Expect(Balance::Adjust(&nFirst, "id_RAT_Checking", 0, 12), 12, "a rating The Machine lacks at that level");
    Balance::EndTeam();

    // 4 + 0.375 * (8 - 4) = 5.5, and 7 + 0.375 * (11 - 7) = 8.5: halves go to the even neighbour.
    Balance::BeginTeam(strBottom.c_str());
    Expect(Balance::Adjust(&nFirst, "id_RAT_Checking", 1, 8), 6, "The Sturmnacht's first dromer");
    Expect(Balance::Adjust(&nSecond, "id_RAT_Checking", 1, 11), 8, "The Sturmnacht's second dromer");
    Balance::EndTeam();

    std::filesystem::remove_all(folder);

    if (s_nFailed != 0)
    {
        std::println("{} failed", s_nFailed);
        return 1;
    }

    std::println("balance: all passed");

    return 0;
}
