// The original's menu and dialog templates, parsed out of its resources, and the rules that turn a
// template control into something to draw. No ImGui here, so the rules can be checked on their own.

#ifndef HYPERBLADE_DIALOGTEMPLATES_H
#define HYPERBLADE_DIALOGTEMPLATES_H

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace DialogTemplates
{

// MF_GRAYED and MF_CHECKED, the two menu states a menu template records.
constexpr auto kMenuGrayed  = 0x01u;
constexpr auto kMenuChecked = 0x08u;

constexpr auto kStyleVisible  = 0x10000000u;
constexpr auto kStyleDisabled = 0x08000000u;
constexpr auto kStyleGroup    = 0x00020000u;

// A menu item. A popup has children and no command; a separator has neither text nor command.
struct MenuEntry
{
    std::string            strText;
    unsigned short         nCommand = 0;
    unsigned short         nFlags   = 0;
    std::vector<MenuEntry> children;
};

// A DLGITEMTEMPLATE. Coordinates are dialog units, as the template stores them.
struct Control
{
    unsigned short nID = 0;
    std::string    strClass;
    std::string    strText;
    unsigned int   dwStyle   = 0;
    unsigned int   dwExStyle = 0;
    short          x         = 0;
    short          y         = 0;
    short          cx        = 0;
    short          cy        = 0;
};

struct Dialog
{
    unsigned short       nID = 0;
    std::string          strCaption;
    unsigned int         dwStyle = 0;
    short                cx      = 0;
    short                cy      = 0;
    std::vector<Control> controls;
};

// The strings an RT_DLGINIT resource adds to one combo box as its dialog opens.
struct ComboItems
{
    unsigned short           nDialog  = 0;
    unsigned short           nControl = 0;
    std::vector<std::string> items;
};

enum class ControlKind : unsigned char
{
    PushButton,
    CheckBox,
    RadioButton,
    GroupBox,
    Label,
    Frame,
    Edit,
    ComboBox,
    ListBox,
    ScrollBar,
    Picture,
    Other,
};

// Pixels per dialog unit. Windows derives them from the dialog font's average character, x from
// its width / 4 and y from its height / 8, so a layout keeps its proportions in any font.
struct Scale
{
    float x;
    float y;
};

// Each parse returns nothing when the template is cut short. Text is converted to UTF-8.
std::optional<std::vector<MenuEntry> > ParseMenu(std::span<const unsigned char> data);
std::optional<Dialog>                  ParseDialog(unsigned short nID, std::span<const unsigned char> data);
std::vector<ComboItems>                ParseDialogInit(unsigned short nDialog, std::span<const unsigned char> data);

ControlKind Classify(const Control& control);

// "&Game Options" reads "Game Options", and "&&" is a literal ampersand.
std::string StripMnemonic(std::string_view label);

// Where a Static's text sits across its width: 0 left, 0.5 centre, 1 right.
float LabelAlignment(const Control& control);

// The pixels per dialog unit for a font whose average character is flCharWidth by flCharHeight.
Scale ScaleFor(float flCharWidth, float flCharHeight);

// A decoded picture, four bytes a pixel in RGBA order, top row first.
struct Image
{
    int                        nWidth  = 0;
    int                        nHeight = 0;
    std::vector<unsigned char> rgba;
};

// The RT_ICON id of the largest, deepest image a group icon lists.
std::optional<unsigned short> PickIcon(std::span<const unsigned char> group);

// An RT_ICON image: a DIB of 1, 4, 8, 24 or 32 bits a pixel and its transparency mask.
std::optional<Image> DecodeIcon(std::span<const unsigned char> data);

// A value from a version resource's string table, such as "CompanyName". Nothing for a missing
// or empty value.
std::optional<std::string> VersionString(std::span<const unsigned char> data, std::string_view key);

} // namespace DialogTemplates

#endif
