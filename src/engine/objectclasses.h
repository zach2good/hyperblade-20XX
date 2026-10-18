// The engine's fixed object slots.

#ifndef HYPERBLADE_OBJECTCLASSES_H
#define HYPERBLADE_OBJECTCLASSES_H

#include "devicector.h"
#include "objectstats.h"
#include "engine/engineobject.h"

// id_MOD_Rok, the Rok the match is played with.
constexpr auto kSlotRok = 6;

// Slots zero to five hold the six PlayerObjects: four skaters, then the two goalies.
// The skaters' slots, named for id_MOD_Dromer0 to id_MOD_Dromer3.
constexpr auto kSlotDromer0       = 0;
constexpr auto kSlotDromer1       = 1;
constexpr auto kSlotDromer2       = 2;
constexpr auto kSlotDromer3       = 3;
constexpr auto kPlayerObjectCount = 6;
constexpr auto kGoalieSlotA       = 4;
constexpr auto kGoalieSlotB       = 5;

constexpr auto kSlotNone = (-1);

// A plan's wildcard slot: any slot to a test, the viewed slot to an action.
constexpr auto kSlotWildcard = 999;

// The main camera, the one plan bodies reach when they have no context.
constexpr auto kCameraMainSlot = 9;

// The helmet camera, id_MOD_HeadCam.
constexpr auto kCameraHeadSlot = 25;

// id_MOD_Drome, the slot src/graphics/renderer.cpp walks the scene from.
constexpr auto kSlotDrome = 10;

// id_MOD_SoundMan: a plain EngineObject that receives the body-check event.
constexpr auto kSlotSoundMan = 24;

// id_MOD_God, the sender of the engine's own broadcasts.
constexpr auto kSlotGod = 23;

// id_MOD_Multiplier; GameEngine::ShowRokValue colours two of its materials by the Rok's value.
constexpr auto kSlotMultiplier = 27;

// The rest of the fixed slots, named for their models in DATA/ENUMIDS.H.
constexpr auto kSlotNgoal         = 7;
constexpr auto kSlotSgoal         = 8;
constexpr auto kSlotRamp          = 12;
constexpr auto kSlotMine          = 54;
constexpr auto kSlotSwarf         = 55;
constexpr auto kSlotFastLane      = 56;
constexpr auto kSlotSlowLane      = 58;
constexpr auto kSlotExplosion     = 60;
constexpr auto kSlotRedRandomizer = 80;

// The arena's repeated models, first and last of each run in DATA/ENUMIDS.H.
constexpr auto kSlotMultiplierLast    = 32;
constexpr auto kSlotTurnstile         = 33;
constexpr auto kSlotTurnstileLast     = 39;
constexpr auto kSlotLimboPole         = 40;
constexpr auto kSlotLimboPoleLast     = 45;
constexpr auto kSlotLaserHurdle       = 46;
constexpr auto kSlotLaserHurdleLast   = 51;
constexpr auto kSlotArmory            = 52;
constexpr auto kSlotArmoryLast        = 53;
constexpr auto kSlotHurdle            = 66;
constexpr auto kSlotHurdleLast        = 71;
constexpr auto kSlotGreenRandomizer   = 72;
constexpr auto kSlotRedRandomizerLast = 85;
constexpr auto kSlotKeyboard          = 86;
constexpr auto kSlotGamepad0          = 87;
constexpr auto kSlotCoach0            = 92;
constexpr auto kSlotCoach1            = 93;
constexpr auto kSlotBodyPart          = 94;
constexpr auto kSlotBodyPartLast      = 101;

constexpr auto kPlayerSlotCount = 4;
constexpr auto kPlayerSlotMask  = 3;

// A skater's teammate is two slots on; a goalie folds onto skater slot 0 or 1.
inline int TeammateSlot(short nSlot)
{
    int nTeammate;

    if (nSlot < kPlayerSlotCount)
    {
        nTeammate = nSlot + 2;
    }
    else
    {
        nTeammate = nSlot;
    }

    return nTeammate & kPlayerSlotMask;
}

#endif
