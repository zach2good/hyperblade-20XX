// The own and foe panels at 0x00450270 and 0x00450600 are one function written twice.

#ifndef HYPERBLADE_INFOBARTEAMS_H
#define HYPERBLADE_INFOBARTEAMS_H

#include "graphics/cddtext.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"

constexpr auto ENGINE_SIDE_SWAPPED = 1;

constexpr auto TEAM_LABEL_BUFFER = 0x50;

constexpr auto TEAM_LABEL_COUNT    = 4;
constexpr auto TEAM_LABEL_ON_DROME = 2;

constexpr auto TEAM_COLOUR_FOLLOWED = 0xffff;
constexpr auto TEAM_COLOUR_RESERVE  = 0xff00;
constexpr auto TEAM_COLOUR_ON_DROME = 0x100ff;
constexpr auto TEAM_COLOUR_ABSENT   = 0;

#endif
