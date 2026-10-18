#ifndef HYPERBLADE_BEHAVIORIDS_H
#define HYPERBLADE_BEHAVIORIDS_H

// A behaviour object's kind, numbered and named as DATA/ENUMIDS.H's Behavior_IDs.
constexpr auto BEH_GLIDE_TO_OBJECT       = 0;
constexpr auto BEH_GLIDE                 = 1;
constexpr auto BEH_RAIL_SLIDE            = 2;
constexpr auto BEH_FLY                   = 3;
constexpr auto BEH_MONITOR_KEYBOARD      = 4;
constexpr auto BEH_UPDATE_WHILE_ATTACHED = 5;
constexpr auto BEH_SET_IN_PLACE          = 6;
constexpr auto BEH_OPEN_AND_CLOSE        = 7;
constexpr auto BEH_TRACKING              = 8;
constexpr auto BEH_WATCHING              = 9;
constexpr auto BEH_TRACK_TRAJECTORY      = 10;
constexpr auto BEH_GLIDE_TO_POSITION     = 11;
constexpr auto BEH_FLY_TOWARD            = 12;
constexpr auto BEH_TRACK_PASS_PREP       = 13;
constexpr auto BEH_AVOIDANCE             = 14;
constexpr auto BEH_UPDATE_RELATIVE_POS   = 15;
constexpr auto BEH_SET_FLAT_IN_PLACE     = 16;
constexpr auto BEH_DO_NOTHING            = 17;
constexpr auto BEH_SET_MOVING_IN_PLACE   = 18;
constexpr auto BEH_GLIDE_BETWEEN_FACING  = 19;
constexpr auto BEH_HEAD_TRACKING         = 20;
constexpr auto BEH_COACH_TEAM            = 21;
constexpr auto BEH_CANDY_TRACKING        = 22;
constexpr auto BEH_GLIDE_LCL             = 23;
constexpr auto BEH_FLY_LCL               = 24;
constexpr auto BEH_COUNT                 = 25;

// SET_BEHAVIOR_PARAM's handler slots, DATA/ENUMIDS.H's BehaviorSetParamFnIDs.
constexpr auto SET_MODS_TWEEN_N_FACING  = 0;
constexpr auto SET_KEY_MONITOR_MODE     = 1;
constexpr auto SET_INTERACTOR_LATENCY   = 2;
constexpr auto SET_TARGET               = 3;
constexpr auto SET_TARGET_POSITION      = 4;
constexpr auto SET_RELATIVE_POSITION    = 5;
constexpr auto SET_MODEL_IM_RELATIVE_TO = 6;
constexpr auto SET_OBJECT_TO_AVOID      = 7;
constexpr auto SET_MODEL_IM_FACING      = 8;
constexpr auto SET_PERSISTENT_ANIMATION = 9;
constexpr auto SET_TARGET_GOAL          = 10;
constexpr auto SET_ANIMATION            = 11;
constexpr auto SET_ANIMATION_EVENT      = 12;
constexpr auto SET_OBJECT_TO_FOLLOW     = 13;
constexpr auto SET_DOUBLE_SCORE         = 14;
constexpr auto SET_DIGRESSIVE           = 15;
constexpr auto SET_PURGE_FLAG           = 16;
constexpr auto SET_BEHAVIOR_PARAM_COUNT = 17;

#endif
