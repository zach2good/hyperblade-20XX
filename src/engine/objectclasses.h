// The engine's fixed object slots.

#ifndef HYPERBLADE_OBJECTCLASSES_H
#define HYPERBLADE_OBJECTCLASSES_H

#include "devicector.h"
#include "objectstats.h"
#include "engine/engineobject.h"

// id_MOD_Rok, the Rok the match is played with.
constexpr auto SLOT_ROK = 6;

// Slots zero to five hold the six PlayerObjects: four skaters, then the two goalies.
// The skaters' slots, named for id_MOD_Dromer0 to id_MOD_Dromer3.
constexpr auto SLOT_DROMER0        = 0;
constexpr auto SLOT_DROMER1        = 1;
constexpr auto SLOT_DROMER2        = 2;
constexpr auto SLOT_DROMER3        = 3;
constexpr auto PLAYER_OBJECT_COUNT = 6;
constexpr auto GOALIE_SLOT_A       = 4;
constexpr auto GOALIE_SLOT_B       = 5;

constexpr auto SLOT_NONE = (-1);

// A plan's wildcard slot: any slot to a test, the viewed slot to an action.
constexpr auto SLOT_WILDCARD = 999;

// The main camera, the one plan bodies reach when they have no context.
constexpr auto CAMERA_MAIN_SLOT = 9;

// The helmet camera, id_MOD_HeadCam.
constexpr auto CAMERA_HEAD_SLOT = 25;

// id_MOD_Drome, the slot src/graphics/renderer.cpp walks the scene from.
constexpr auto SLOT_DROME = 10;

// id_MOD_SoundMan: a plain EngineObject that receives the body-check event.
constexpr auto SLOT_SOUND_MAN = 24;

// id_MOD_God, the sender of the engine's own broadcasts.
constexpr auto SLOT_GOD = 23;

// id_MOD_Multiplier; GameEngine::ShowRokValue colours two of its materials by the Rok's value.
constexpr auto SLOT_MULTIPLIER = 27;

// The rest of the fixed slots, named for their models in DATA/ENUMIDS.H.
constexpr auto SLOT_NGOAL          = 7;
constexpr auto SLOT_SGOAL          = 8;
constexpr auto SLOT_RAMP           = 12;
constexpr auto SLOT_MINE           = 54;
constexpr auto SLOT_SWARF          = 55;
constexpr auto SLOT_FAST_LANE      = 56;
constexpr auto SLOT_SLOW_LANE      = 58;
constexpr auto SLOT_EXPLOSION      = 60;
constexpr auto SLOT_RED_RANDOMIZER = 80;

// The arena's repeated models, first and last of each run in DATA/ENUMIDS.H.
constexpr auto SLOT_MULTIPLIER_LAST     = 32;
constexpr auto SLOT_TURNSTILE           = 33;
constexpr auto SLOT_TURNSTILE_LAST      = 39;
constexpr auto SLOT_LIMBO_POLE          = 40;
constexpr auto SLOT_LIMBO_POLE_LAST     = 45;
constexpr auto SLOT_LASER_HURDLE        = 46;
constexpr auto SLOT_LASER_HURDLE_LAST   = 51;
constexpr auto SLOT_ARMORY              = 52;
constexpr auto SLOT_ARMORY_LAST         = 53;
constexpr auto SLOT_HURDLE              = 66;
constexpr auto SLOT_HURDLE_LAST         = 71;
constexpr auto SLOT_GREEN_RANDOMIZER    = 72;
constexpr auto SLOT_RED_RANDOMIZER_LAST = 85;
constexpr auto SLOT_KEYBOARD            = 86;
constexpr auto SLOT_GAMEPAD0            = 87;
constexpr auto SLOT_COACH0              = 92;
constexpr auto SLOT_COACH1              = 93;
constexpr auto SLOT_BODY_PART           = 94;
constexpr auto SLOT_BODY_PART_LAST      = 101;

constexpr auto PLAYER_SLOT_COUNT = 4;
constexpr auto PLAYER_SLOT_MASK  = 3;

// A skater's teammate is two slots on; a goalie folds onto skater slot 0 or 1.
inline int TeammateSlot(short nSlot)
{
    int nTeammate;

    if (nSlot < PLAYER_SLOT_COUNT)
    {
        nTeammate = nSlot + 2;
    }
    else
    {
        nTeammate = nSlot;
    }

    return nTeammate & PLAYER_SLOT_MASK;
}

#endif
