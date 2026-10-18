// The own and foe panels at 0x00450270 and 0x00450600 are one function written twice.

#ifndef HYPERBLADE_INFOBARTEAMS_H
#define HYPERBLADE_INFOBARTEAMS_H

#include "graphics/cddtext.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"

constexpr auto kEngineSideSwapped = 1;

constexpr auto kTeamLabelBuffer = 0x50;

constexpr auto kTeamLabelCount   = 4;
constexpr auto kTeamLabelOnDrome = 2;

constexpr auto kTeamColourFollowed = 0xffff;
constexpr auto kTeamColourReserve  = 0xff00;
constexpr auto kTeamColourOnDrome  = 0x100ff;
constexpr auto kTeamColourAbsent   = 0;

#endif
