// Parses menu, dialog and combo box templates built here byte by byte, as HYPERX.EXE stores them,
// and checks the rules that decide how each control draws.

#include "ui/dialogtemplates.h"
#include <cstdio>
#include <span>
#include <string>
#include <vector>

static int s_nFailed = 0;

static void Expect(bool bOK, const char* pszWhat)
{
    if (!bOK)
    {
        std::printf("failed: %s\n", pszWhat);
        s_nFailed++;
    }
}

namespace
{

struct Bytes
{
    std::vector<unsigned char> data;

    void U16(unsigned int nValue)
    {
        data.push_back(static_cast<unsigned char>(nValue));
        data.push_back(static_cast<unsigned char>(nValue >> 8));
    }

    void U32(unsigned int nValue)
    {
        U16(nValue & 0xffff);
        U16(nValue >> 16);
    }

    void Text(const char* pszText)
    {
        for (; *pszText != '\0'; pszText++)
        {
            U16(static_cast<unsigned char>(*pszText));
        }

        U16(0);
    }

    void Align()
    {
        while (data.size() % 4 != 0)
        {
            data.push_back(0);
        }
    }
};

constexpr auto kPopup   = 0x10u;
constexpr auto kEnd     = 0x80u;
constexpr auto kChecked = 0x08u;

} // namespace

int main()
{
    // File with two items, View whose last item is a popup, and Help, the last popup of all. An END
    // item inside View's last popup closes View as well.
    {
        auto menu = Bytes();

        menu.U16(0);
        menu.U16(0);
        menu.U16(kPopup);
        menu.Text("&File");
        menu.U16(0);
        menu.U16(124);
        menu.Text("&New Game");
        menu.U16(kEnd);
        menu.U16(2);
        menu.Text("E&xit");
        menu.U16(kPopup);
        menu.Text("&View");
        menu.U16(kPopup | kEnd);
        menu.Text("&Resolution");
        menu.U16(kChecked | kEnd);
        menu.U16(4);
        menu.Text("&640x480");
        menu.U16(kPopup | kEnd);
        menu.Text("&Help");
        menu.U16(kEnd);
        menu.U16(1);
        menu.Text("&About");

        const auto parsed = DialogTemplates::ParseMenu(menu.data);

        Expect(parsed.has_value(), "menu parses");
        Expect(parsed && parsed->size() == 3, "three popups at the top");
        Expect(parsed && (*parsed)[0].children.size() == 2, "File holds two items");
        Expect(parsed && (*parsed)[0].children[1].nCommand == 2, "Exit keeps its command");
        Expect(parsed && (*parsed)[1].children.size() == 1, "View holds the Resolution popup");
        Expect(parsed && (*parsed)[1].children[0].children[0].nFlags == kChecked, "the check mark survives, without END");
        Expect(parsed && (*parsed)[2].strText == "&Help", "Help follows the closed View");

        menu.data.resize(menu.data.size() - 3);
        Expect(!DialogTemplates::ParseMenu(menu.data).has_value(), "a cut-off menu fails");
    }

    // A dialog with a default push button and a centred label.
    {
        auto dialog = Bytes();

        dialog.U32(0x900000c0);
        dialog.U32(0);
        dialog.U16(2);
        dialog.U16(0);
        dialog.U16(0);
        dialog.U16(185);
        dialog.U16(43);
        dialog.U16(0);
        dialog.U16(0);
        dialog.Text("Test");
        dialog.U16(8);
        dialog.Text("MS Sans Serif");

        dialog.Align();
        dialog.U32(0x50010001);
        dialog.U32(0);
        dialog.U16(7);
        dialog.U16(18);
        dialog.U16(78);
        dialog.U16(15);
        dialog.U16(1032);
        dialog.U16(0xffff);
        dialog.U16(0x80);
        dialog.Text("&Game Options");
        dialog.U16(0);

        dialog.Align();
        dialog.U32(0x50020001);
        dialog.U32(0);
        dialog.U16(75);
        dialog.U16(195);
        dialog.U16(121);
        dialog.U16(19);
        dialog.U16(0xffff);
        dialog.U16(0xffff);
        dialog.U16(0x82);
        dialog.Text("PLEASE WAIT");
        dialog.U16(0);

        const auto parsed = DialogTemplates::ParseDialog(122, dialog.data);

        Expect(parsed.has_value(), "dialog parses");
        Expect(parsed && parsed->strCaption == "Test" && parsed->cx == 185 && parsed->cy == 43, "caption and size");
        Expect(parsed && parsed->controls.size() == 2, "two controls");

        if (parsed && parsed->controls.size() == 2)
        {
            const auto& button = parsed->controls[0];
            const auto& label  = parsed->controls[1];

            Expect(button.strClass == "Button" && button.nID == 1032 && button.cx == 78, "button fields");
            Expect(DialogTemplates::Classify(button) == DialogTemplates::ControlKind::PushButton, "a default push button is a push button");
            Expect(DialogTemplates::Classify(label) == DialogTemplates::ControlKind::Label, "a static with text is a label");
            Expect(DialogTemplates::LabelAlignment(label) == 0.5f, "SS_CENTER centres");
        }

        dialog.data.resize(dialog.data.size() - 6);
        Expect(!DialogTemplates::ParseDialog(122, dialog.data).has_value(), "a cut-off dialog fails");
    }

    // Two strings for one combo box and one for the next, then the terminating zero.
    {
        auto init = Bytes();

        const auto add = [&](unsigned int nControl, const char* pszText)
        {
            const auto strText = std::string(pszText);

            init.U16(nControl);
            init.U16(0x403);
            init.U32(static_cast<unsigned int>(strText.size() + 1));

            for (const auto ch : strText)
            {
                init.data.push_back(static_cast<unsigned char>(ch));
            }

            init.data.push_back(0);
        };

        add(1015, "640 x 480");
        add(1015, "320 x 240");
        add(1013, "Stadium Cam");
        init.U16(0);

        const auto lists = DialogTemplates::ParseDialogInit(137, init.data);

        Expect(lists.size() == 2, "two combo boxes");
        Expect(lists.size() == 2 && lists[0].nControl == 1015 && lists[0].items.size() == 2, "the first gets both of its strings");
        Expect(lists.size() == 2 && lists[1].items[0] == "Stadium Cam", "the second gets its string");
    }

    Expect(DialogTemplates::StripMnemonic("&Game Options") == "Game Options", "a mnemonic goes");
    Expect(DialogTemplates::StripMnemonic("Tom && Jerry") == "Tom & Jerry", "a doubled ampersand stays one");

    const auto scale = DialogTemplates::ScaleFor(6.0f, 13.0f);

    Expect(scale.x == 1.5f && scale.y == 1.625f, "MS Sans Serif's base units at 13 pixels");

    // A String node: length, value length in characters, type 1, key, padding, value.
    auto version = Bytes();

    version.U16(0);
    version.U16(0);
    version.U16(0);
    version.Text("VS_VERSION_INFO");
    version.Align();
    version.U16(0);
    version.U16(8);
    version.U16(1);
    version.Text("CompanyName");
    version.Align();
    version.Text("Wizbang");
    version.Align();
    version.U16(0);
    version.U16(0);
    version.U16(1);
    version.Text("LegalTrademarks");
    version.Align();

    Expect(DialogTemplates::VersionString(version.data, "CompanyName") == "Wizbang", "a version string by key");
    Expect(!DialogTemplates::VersionString(version.data, "LegalTrademarks"), "an empty version string reads as nothing");
    Expect(!DialogTemplates::VersionString(version.data, "FileVersion"), "a missing version string reads as nothing");
    Expect(!DialogTemplates::VersionString(std::span(version.data).first(80), "CompanyName"), "a cut-short version resource");

    // A group listing a 16 pixel and a 32 pixel image picks the 32 pixel one's id.
    auto group = Bytes();

    group.U16(0);
    group.U16(1);
    group.U16(2);

    for (const auto nSide : { 16u, 32u })
    {
        group.data.push_back(static_cast<unsigned char>(nSide));
        group.data.push_back(static_cast<unsigned char>(nSide));
        group.data.push_back(16);
        group.data.push_back(0);
        group.U16(1);
        group.U16(4);
        group.U32(0);
        group.U16(nSide);
    }

    Expect(DialogTemplates::PickIcon(group.data) == 32, "the largest image in a group icon");

    // A two by two, one bit icon: black and white over white and black, its top right corner masked.
    auto icon = Bytes();

    icon.U32(40);
    icon.U32(2);
    icon.U32(4);
    icon.U16(1);
    icon.U16(1);
    icon.U32(0);
    icon.U32(0);
    icon.U32(0);
    icon.U32(0);
    icon.U32(2);
    icon.U32(0);
    icon.U32(0x000000);
    icon.U32(0xffffff);
    icon.U32(0x80);
    icon.U32(0x40);
    icon.U32(0x00);
    icon.U32(0x40);

    const auto image = DialogTemplates::DecodeIcon(icon.data);

    Expect(image && image->nWidth == 2 && image->nHeight == 2, "an icon's size is half its stored height");
    Expect(image && image->rgba[0] == 0 && image->rgba[4] == 255 && image->rgba[8] == 255 && image->rgba[12] == 0, "an icon's rows run bottom up");
    Expect(image && image->rgba[3] == 255 && image->rgba[7] == 0 && image->rgba[11] == 255, "an icon's mask makes pixels transparent");
    Expect(!DialogTemplates::DecodeIcon(std::span(icon.data).first(60)), "a cut-short icon");

    if (s_nFailed != 0)
    {
        return 1;
    }

    std::printf("dialog templates check passed\n");

    return 0;
}
