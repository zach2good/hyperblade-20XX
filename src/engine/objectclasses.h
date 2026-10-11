// The engine's fixed object slots.

#ifndef HYPERBLADE_OBJECTCLASSES_H
#define HYPERBLADE_OBJECTCLASSES_H

#include "devicector.h"
#include "objectstats.h"
#include "engine/engineobject.h"

// id_MOD_Rok, the Rok the match is played with.
#define SLOT_ROK 6

// Slots zero to five hold the six PlayerObjects: four skaters, then the two goalies.
// The skaters' slots, named for id_MOD_Dromer0 to id_MOD_Dromer3.
#define SLOT_DROMER0        0
#define SLOT_DROMER1        1
#define SLOT_DROMER2        2
#define SLOT_DROMER3        3
#define PLAYER_OBJECT_COUNT 6
#define GOALIE_SLOT_A       4
#define GOALIE_SLOT_B       5

#define SLOT_NONE (-1)

// A plan's wildcard slot: any slot to a test, the viewed slot to an action.
#define SLOT_WILDCARD 999

// The main camera, the one plan bodies reach when they have no context.
#define CAMERA_MAIN_SLOT 9

// The helmet camera, id_MOD_HeadCam.
#define CAMERA_HEAD_SLOT 25

// id_MOD_Drome, the slot src/graphics/renderer.cpp walks the scene from.
#define SLOT_DROME 10

// id_MOD_SoundMan: a plain EngineObject that receives the body-check event.
#define SLOT_SOUND_MAN 24

// id_MOD_God, the sender of the engine's own broadcasts.
#define SLOT_GOD 23

// id_MOD_Multiplier; GameEngine::ShowRokValue colours two of its materials by the Rok's value.
#define SLOT_MULTIPLIER 27

// The rest of the fixed slots, named for their models in DATA/ENUMIDS.H.
#define SLOT_NGOAL          7
#define SLOT_SGOAL          8
#define SLOT_RAMP           12
#define SLOT_MINE           54
#define SLOT_SWARF          55
#define SLOT_FAST_LANE      56
#define SLOT_SLOW_LANE      58
#define SLOT_EXPLOSION      60
#define SLOT_RED_RANDOMIZER 80

// The arena's repeated models, first and last of each run in DATA/ENUMIDS.H.
#define SLOT_MULTIPLIER_LAST     32
#define SLOT_TURNSTILE           33
#define SLOT_TURNSTILE_LAST      39
#define SLOT_LIMBO_POLE          40
#define SLOT_LIMBO_POLE_LAST     45
#define SLOT_LASER_HURDLE        46
#define SLOT_LASER_HURDLE_LAST   51
#define SLOT_ARMORY              52
#define SLOT_ARMORY_LAST         53
#define SLOT_HURDLE              66
#define SLOT_HURDLE_LAST         71
#define SLOT_GREEN_RANDOMIZER    72
#define SLOT_RED_RANDOMIZER_LAST 85
#define SLOT_KEYBOARD            86
#define SLOT_GAMEPAD0            87
#define SLOT_COACH0              92
#define SLOT_COACH1              93
#define SLOT_BODY_PART           94
#define SLOT_BODY_PART_LAST      101

#define PLAYER_SLOT_COUNT 4
#define PLAYER_SLOT_MASK  3

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
