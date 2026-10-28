// The resources of a Windows program file, read without loading it. The game's own text, menu and
// dialogs come out of the player's HYPERX.EXE this way, so none of them ships with the port.

#ifndef HYPERBLADE_PERESOURCES_H
#define HYPERBLADE_PERESOURCES_H

#include <optional>
#include <string>
#include <vector>

namespace PEResources
{

constexpr auto kTypeMenu       = 4u;
constexpr auto kTypeDialog     = 5u;
constexpr auto kTypeString     = 6u;
constexpr auto kTypeIcon       = 3u;
constexpr auto kTypeGroupIcon  = 14u;
constexpr auto kTypeVersion    = 16u;
constexpr auto kTypeDialogInit = 240u;

// One resource, in the first language the file lists for it. A named resource has nID 0 and its
// name in strName; a numbered one has an empty strName.
struct Resource
{
    unsigned int               nType;
    unsigned int               nID;
    std::string                strName;
    std::vector<unsigned char> data;
};

// Every resource in the file. Nothing when the file is missing or is not a program file.
std::optional<std::vector<Resource> > Read(const std::string& strExe);

} // namespace PEResources

#endif
