#include "graphics/prompticons.h"
#include "common/pak.h"
#include <SDL3/SDL.h>
#include <string>
#include <utility>

namespace
{

constexpr std::pair<const char*, const char*> ICONS[] = {
    { "A", "xbox_button_color_a.png" },
    { "B", "xbox_button_color_b.png" },
    { "X", "xbox_button_color_x.png" },
    { "Y", "xbox_button_color_y.png" },
    { "LeftShoulder", "xbox_lb.png" },
    { "RightShoulder", "xbox_rb.png" },
    { "LeftTrigger", "xbox_lt.png" },
    { "RightTrigger", "xbox_rt.png" },
    { "DPadUp", "xbox_dpad_up.png" },
    { "DPadDown", "xbox_dpad_down.png" },
    { "DPadLeft", "xbox_dpad_left.png" },
    { "DPadRight", "xbox_dpad_right.png" },
    { "Back", "xbox_button_back.png" },
    { "Start", "xbox_button_start.png" },
    { "StickUp", "xbox_stick_l_up.png" },
    { "StickDown", "xbox_stick_l_down.png" },
    { "StickLeft", "xbox_stick_l_left.png" },
    { "StickRight", "xbox_stick_l_right.png" },
};

} // namespace

std::vector<unsigned char> PromptIcons::Find(const char* pszName)
{
    for (const auto& [pszIcon, pszFile] : ICONS)
    {
        if (SDL_strcasecmp(pszIcon, pszName) == 0)
        {
            return Pak::Read(std::string("prompts/") + pszFile);
        }
    }

    return {};
}
