#include "dialogtemplates.h"
#include <cstddef>
#include <cstdint>
#include <utility>

namespace
{

constexpr auto kMenuPopup = 0x10u;
constexpr auto kMenuEnd   = 0x80u;

// DS_SETFONT: a point size and a face name follow the caption.
constexpr auto kDialogSetFont = 0x40u;

// The predefined window classes a template names by atom.
constexpr std::pair<unsigned int, const char*> s_aClassAtoms[] = {
    { 0x80, "Button" },
    { 0x81, "Edit" },
    { 0x82, "Static" },
    { 0x83, "ListBox" },
    { 0x84, "ScrollBar" },
    { 0x85, "ComboBox" },
};

// BS_TYPEMASK and the button types inside it.
constexpr auto kButtonTypeMask   = 0x0Fu;
constexpr auto kButtonCheckBox   = 0x02u;
constexpr auto kButtonAuto3State = 0x06u;
constexpr auto kButtonRadio      = 0x04u;
constexpr auto kButtonAutoRadio  = 0x09u;
constexpr auto kButtonGroupBox   = 0x07u;
constexpr auto kButton3State     = 0x05u;
constexpr auto kButtonAutoCheck  = 0x03u;

// SS_TYPEMASK and the static types inside it.
constexpr auto kStaticTypeMask    = 0x1Fu;
constexpr auto kStaticCentre      = 0x01u;
constexpr auto kStaticRight       = 0x02u;
constexpr auto kStaticIcon        = 0x03u;
constexpr auto kStaticBlackRect   = 0x04u;
constexpr auto kStaticWhiteFrame  = 0x09u;
constexpr auto kStaticBitmap      = 0x0Eu;
constexpr auto kStaticEtchedHorz  = 0x10u;
constexpr auto kStaticEtchedFrame = 0x12u;

// The longest menu nesting the reader follows. The game's goes three deep.
constexpr auto kMaxMenuDepth = 8u;

void AppendUTF8(std::string& out, unsigned int nCode)
{
    if (nCode < 0x80)
    {
        out.push_back(static_cast<char>(nCode));
    }
    else if (nCode < 0x800)
    {
        out.push_back(static_cast<char>(0xc0 | nCode >> 6));
        out.push_back(static_cast<char>(0x80 | (nCode & 0x3f)));
    }
    else if (nCode < 0x10000)
    {
        out.push_back(static_cast<char>(0xe0 | nCode >> 12));
        out.push_back(static_cast<char>(0x80 | (nCode >> 6 & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (nCode & 0x3f)));
    }
    else
    {
        out.push_back(static_cast<char>(0xf0 | nCode >> 18));
        out.push_back(static_cast<char>(0x80 | (nCode >> 12 & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (nCode >> 6 & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (nCode & 0x3f)));
    }
}

// Little-endian reads that mark the reader bad instead of running past the template.
struct Reader
{
    std::span<const unsigned char> data;
    size_t                         nAt    = 0;
    bool                           bValid = true;

    unsigned int U16()
    {
        if (nAt + 2 > data.size())
        {
            bValid = false;

            return 0;
        }

        const auto nValue = static_cast<unsigned int>(data[nAt] | data[nAt + 1] << 8);

        nAt += 2;

        return nValue;
    }

    unsigned int U32()
    {
        const auto nLow = U16();

        return nLow | U16() << 16;
    }

    short S16()
    {
        return static_cast<short>(static_cast<std::int16_t>(U16()));
    }

    // A zero-terminated UTF-16 string.
    std::string Text()
    {
        auto text = std::string();

        while (bValid)
        {
            auto nCode = U16();

            if (nCode == 0)
            {
                break;
            }

            if (nCode >= 0xd800 && nCode < 0xdc00)
            {
                nCode = 0x10000 + ((nCode - 0xd800) << 10) + (U16() - 0xdc00);
            }

            AppendUTF8(text, nCode);
        }

        return text;
    }

    // A string, or 0xffff and an ordinal. The ordinal comes back in nOrdinal and the text empty.
    std::string TextOrOrdinal(unsigned int& nOrdinal)
    {
        nOrdinal = 0;

        if (nAt + 2 <= data.size() && data[nAt] == 0xff && data[nAt + 1] == 0xff)
        {
            nAt += 2;
            nOrdinal = U16();

            return std::string();
        }

        return Text();
    }

    void Align()
    {
        nAt = (nAt + 3) & ~size_t(3);
    }
};

const char* ClassName(unsigned int nAtom)
{
    for (const auto& [nKnown, pszName] : s_aClassAtoms)
    {
        if (nKnown == nAtom)
        {
            return pszName;
        }
    }

    return "";
}

} // namespace

std::optional<std::vector<DialogTemplates::MenuEntry> > DialogTemplates::ParseMenu(std::span<const unsigned char> data)
{
    auto reader = Reader{ data };

    // MENUHEADER: a version, which a MENUEX sets to 1, and the offset of the first item.
    if (reader.U16() != 0)
    {
        return std::nullopt;
    }

    reader.U16();

    auto root = std::vector<MenuEntry>();

    // Each open level remembers whether its popup was the last item of its parent, so closing the
    // level closes the parent too.
    auto stack = std::vector<std::pair<std::vector<MenuEntry>*, bool> >{ { &root, false } };

    while (!stack.empty() && reader.bValid && reader.nAt < data.size())
    {
        const auto nFlags = reader.U16();

        auto entry = MenuEntry();

        if ((nFlags & kMenuPopup) == 0)
        {
            entry.nCommand = static_cast<unsigned short>(reader.U16());
        }

        entry.strText = reader.Text();
        entry.nFlags  = static_cast<unsigned short>(nFlags & ~(kMenuPopup | kMenuEnd));

        auto& level = *stack.back().first;

        level.push_back(std::move(entry));

        if ((nFlags & kMenuPopup) != 0)
        {
            if (stack.size() >= kMaxMenuDepth)
            {
                return std::nullopt;
            }

            stack.emplace_back(&level.back().children, (nFlags & kMenuEnd) != 0);
            continue;
        }

        if ((nFlags & kMenuEnd) == 0)
        {
            continue;
        }

        while (!stack.empty())
        {
            const auto bClosesParent = stack.back().second;

            stack.pop_back();

            if (!bClosesParent)
            {
                break;
            }
        }
    }

    if (!reader.bValid)
    {
        return std::nullopt;
    }

    return root;
}

std::optional<DialogTemplates::Dialog> DialogTemplates::ParseDialog(unsigned short nID, std::span<const unsigned char> data)
{
    auto reader = Reader{ data };
    auto dialog = Dialog();

    dialog.nID = nID;

    // A DIALOGEX opens with 1 and 0xffff. The game has none, so this reads DLGTEMPLATE only.
    if (data.size() >= 4 && data[2] == 0xff && data[3] == 0xff)
    {
        return std::nullopt;
    }

    dialog.dwStyle = reader.U32();
    reader.U32();

    const auto nCount = reader.U16();

    reader.S16();
    reader.S16();
    dialog.cx = reader.S16();
    dialog.cy = reader.S16();

    auto nOrdinal = 0u;

    reader.TextOrOrdinal(nOrdinal);
    reader.TextOrOrdinal(nOrdinal);
    dialog.strCaption = reader.Text();

    if ((dialog.dwStyle & kDialogSetFont) != 0)
    {
        reader.U16();
        reader.Text();
    }

    for (unsigned int n = 0; n < nCount && reader.bValid; n++)
    {
        reader.Align();

        auto control = Control();

        control.dwStyle   = reader.U32();
        control.dwExStyle = reader.U32();
        control.x         = reader.S16();
        control.y         = reader.S16();
        control.cx        = reader.S16();
        control.cy        = reader.S16();
        control.nID       = static_cast<unsigned short>(reader.U16());

        auto nAtom = 0u;

        control.strClass = reader.TextOrOrdinal(nAtom);

        if (nAtom != 0)
        {
            control.strClass = ClassName(nAtom);
        }

        // A numbered title names an icon or a bitmap, which draws as a placeholder.
        control.strText = reader.TextOrOrdinal(nOrdinal);

        const auto nExtra = reader.U16();

        reader.nAt += nExtra;

        dialog.controls.push_back(std::move(control));
    }

    if (!reader.bValid)
    {
        return std::nullopt;
    }

    return dialog;
}

std::vector<DialogTemplates::ComboItems> DialogTemplates::ParseDialogInit(unsigned short nDialog, std::span<const unsigned char> data)
{
    auto lists  = std::vector<ComboItems>();
    auto reader = Reader{ data };

    // Each record is a control, a message, a length and that many bytes, until a control of 0.
    while (reader.bValid && reader.nAt + 8 <= data.size())
    {
        const auto nControl = static_cast<unsigned short>(reader.U16());

        if (nControl == 0)
        {
            break;
        }

        reader.U16();

        const auto nLength = reader.U32();

        if (reader.nAt + nLength > data.size())
        {
            break;
        }

        // ANSI text, all ASCII in the game; the upper half is taken as Latin-1.
        auto item = std::string();

        for (unsigned int n = 0; n < nLength && data[reader.nAt + n] != 0; n++)
        {
            AppendUTF8(item, data[reader.nAt + n]);
        }

        reader.nAt += nLength;

        if (lists.empty() || lists.back().nControl != nControl)
        {
            lists.push_back(ComboItems{ nDialog, nControl, {} });
        }

        lists.back().items.push_back(std::move(item));
    }

    return lists;
}

DialogTemplates::ControlKind DialogTemplates::Classify(const Control& control)
{
    if (control.strClass == "Button")
    {
        switch (control.dwStyle & kButtonTypeMask)
        {
            case kButtonCheckBox:
            case kButtonAutoCheck:
            case kButton3State:
            case kButtonAuto3State:
                return ControlKind::CheckBox;

            case kButtonRadio:
            case kButtonAutoRadio:
                return ControlKind::RadioButton;

            case kButtonGroupBox:
                return ControlKind::GroupBox;

            default:
                return ControlKind::PushButton;
        }
    }

    if (control.strClass == "Static")
    {
        const auto nType = control.dwStyle & kStaticTypeMask;

        if ((nType >= kStaticBlackRect && nType <= kStaticWhiteFrame) || (nType >= kStaticEtchedHorz && nType <= kStaticEtchedFrame))
        {
            return ControlKind::Frame;
        }

        if (nType == kStaticIcon || nType == kStaticBitmap)
        {
            return ControlKind::Picture;
        }

        return ControlKind::Label;
    }

    if (control.strClass == "Edit")
    {
        return ControlKind::Edit;
    }

    if (control.strClass == "ComboBox")
    {
        return ControlKind::ComboBox;
    }

    if (control.strClass == "ListBox")
    {
        return ControlKind::ListBox;
    }

    if (control.strClass == "ScrollBar")
    {
        return ControlKind::ScrollBar;
    }

    return ControlKind::Other;
}

std::string DialogTemplates::StripMnemonic(std::string_view label)
{
    auto out = std::string();

    out.reserve(label.size());

    for (size_t n = 0; n < label.size(); n++)
    {
        if (label[n] != '&')
        {
            out.push_back(label[n]);
            continue;
        }

        if (n + 1 < label.size() && label[n + 1] == '&')
        {
            out.push_back('&');
            n++;
        }
    }

    return out;
}

float DialogTemplates::LabelAlignment(const Control& control)
{
    switch (control.dwStyle & kStaticTypeMask)
    {
        case kStaticCentre:
            return 0.5f;

        case kStaticRight:
            return 1.0f;

        default:
            return 0.0f;
    }
}

std::optional<unsigned short> DialogTemplates::PickIcon(std::span<const unsigned char> group)
{
    // GRPICONDIR's six bytes, then fourteen per image.
    constexpr auto kHeader = size_t{ 6 };
    constexpr auto kEntry  = size_t{ 14 };

    auto reader = Reader{ group, 4 };

    const auto nCount = reader.U16();

    auto best   = std::optional<unsigned short>();
    auto nScore = 0u;

    for (size_t n = 0; n < nCount && kHeader + (n + 1) * kEntry <= group.size(); n++)
    {
        const auto nAt = kHeader + n * kEntry;

        // A width of zero means 256.
        const auto nWidth = [&]
        {
            if (group[nAt] == 0)
            {
                return 256u;
            }

            return static_cast<unsigned int>(group[nAt]);
        }();

        reader.nAt = nAt + 6;

        const auto nScoreHere = nWidth * 64 + reader.U16();

        reader.nAt = nAt + 12;

        const auto nID = reader.U16();

        if (nScoreHere > nScore)
        {
            nScore = nScoreHere;
            best   = static_cast<unsigned short>(nID);
        }
    }

    return best;
}

std::optional<DialogTemplates::Image> DialogTemplates::DecodeIcon(std::span<const unsigned char> data)
{
    constexpr auto kInfoHeader = 40u;
    constexpr auto kMaxSide    = 256;

    auto reader = Reader{ data };

    if (reader.U32() != kInfoHeader)
    {
        return std::nullopt;
    }

    const auto nWidth = static_cast<int>(reader.U32());

    // The height covers the colour rows and the mask rows together.
    const auto nHeight = static_cast<int>(reader.U32()) / 2;

    reader.U16();

    const auto nBits        = reader.U16();
    const auto nCompression = reader.U32();

    reader.nAt = 32;

    const auto nUsed = reader.U32();

    if (!reader.bValid || nCompression != 0 || nWidth <= 0 || nHeight <= 0 || nWidth > kMaxSide || nHeight > kMaxSide)
    {
        return std::nullopt;
    }

    if (nBits != 1 && nBits != 4 && nBits != 8 && nBits != 24 && nBits != 32)
    {
        return std::nullopt;
    }

    const auto nColours = [&]
    {
        if (nBits > 8)
        {
            return 0u;
        }

        if (nUsed != 0)
        {
            return nUsed;
        }

        return 1u << nBits;
    }();

    const auto nPalette    = size_t{ kInfoHeader };
    const auto nPixels     = nPalette + size_t{ nColours } * 4;
    const auto nStride     = (static_cast<size_t>(nWidth) * nBits + 31) / 32 * 4;
    const auto nMask       = nPixels + nStride * nHeight;
    const auto nMaskStride = (static_cast<size_t>(nWidth) + 31) / 32 * 4;

    if (nMask + nMaskStride * nHeight > data.size())
    {
        return std::nullopt;
    }

    auto image = Image{ nWidth, nHeight, std::vector<unsigned char>(static_cast<size_t>(nWidth) * nHeight * 4) };

    for (int y = 0; y < nHeight; y++)
    {
        // Rows are stored bottom up.
        const auto nRow     = nPixels + nStride * (nHeight - 1 - y);
        const auto nMaskRow = nMask + nMaskStride * (nHeight - 1 - y);

        for (int x = 0; x < nWidth; x++)
        {
            auto* const pOut = &image.rgba[(static_cast<size_t>(y) * nWidth + x) * 4];

            if (nBits >= 24)
            {
                const auto nAt = nRow + static_cast<size_t>(x) * (nBits / 8);

                pOut[0] = data[nAt + 2];
                pOut[1] = data[nAt + 1];
                pOut[2] = data[nAt];

                if (nBits == 32)
                {
                    pOut[3] = data[nAt + 3];
                    continue;
                }
            }
            else
            {
                const auto nBit   = static_cast<size_t>(x) * nBits;
                const auto nIndex = data[nRow + nBit / 8] >> (8 - nBits - nBit % 8) & ((1u << nBits) - 1);

                if (nIndex >= nColours)
                {
                    return std::nullopt;
                }

                const auto nEntry = nPalette + size_t{ nIndex } * 4;

                pOut[0] = data[nEntry + 2];
                pOut[1] = data[nEntry + 1];
                pOut[2] = data[nEntry];
            }

            // The image starts transparent, and a clear mask bit makes a pixel opaque.
            if ((data[nMaskRow + x / 8] >> (7 - x % 8) & 1) == 0)
            {
                pOut[3] = 255;
            }
        }
    }

    return image;
}

// Every node starts on a four-byte boundary, so the scan tries each one for a String node by that key.
std::optional<std::string> DialogTemplates::VersionString(std::span<const unsigned char> data, std::string_view key)
{
    // wType 1 marks a node whose value is text.
    constexpr auto kTextNode = 1u;

    for (size_t nNode = 0; nNode + 6 < data.size(); nNode += 4)
    {
        auto reader = Reader{ data, nNode };

        reader.U16();

        const auto nValueLength = reader.U16();

        if (reader.U16() != kTextNode || reader.Text() != key || !reader.bValid)
        {
            continue;
        }

        reader.nAt = (reader.nAt + 3) & ~size_t{ 3 };

        auto text = std::string();

        for (auto n = 0u; n < nValueLength; n++)
        {
            const auto nCode = reader.U16();

            if (nCode == 0)
            {
                break;
            }

            AppendUTF8(text, nCode);
        }

        if (!reader.bValid || text.empty())
        {
            return std::nullopt;
        }

        return text;
    }

    return std::nullopt;
}

DialogTemplates::Scale DialogTemplates::ScaleFor(float flCharWidth, float flCharHeight)
{
    return Scale{ flCharWidth / 4.0f, flCharHeight / 8.0f };
}
