#ifndef HYPERBLADE_BEHAVIORIDS_H
#define HYPERBLADE_BEHAVIORIDS_H

// A behaviour object's kind, numbered and named as DATA/ENUMIDS.H's Behavior_IDs.
#define BEH_GLIDE_TO_OBJECT       0
#define BEH_GLIDE                 1
#define BEH_RAIL_SLIDE            2
#define BEH_FLY                   3
#define BEH_MONITOR_KEYBOARD      4
#define BEH_UPDATE_WHILE_ATTACHED 5
#define BEH_SET_IN_PLACE          6
#define BEH_OPEN_AND_CLOSE        7
#define BEH_TRACKING              8
#define BEH_WATCHING              9
#define BEH_TRACK_TRAJECTORY      10
#define BEH_GLIDE_TO_POSITION     11
#define BEH_FLY_TOWARD            12
#define BEH_TRACK_PASS_PREP       13
#define BEH_AVOIDANCE             14
#define BEH_UPDATE_RELATIVE_POS   15
#define BEH_SET_FLAT_IN_PLACE     16
#define BEH_DO_NOTHING            17
#define BEH_SET_MOVING_IN_PLACE   18
#define BEH_GLIDE_BETWEEN_FACING  19
#define BEH_HEAD_TRACKING         20
#define BEH_COACH_TEAM            21
#define BEH_CANDY_TRACKING        22
#define BEH_GLIDE_LCL             23
#define BEH_FLY_LCL               24
#define BEH_COUNT                 25

// SET_BEHAVIOR_PARAM's handler slots, DATA/ENUMIDS.H's BehaviorSetParamFnIDs.
#define SET_MODS_TWEEN_N_FACING  0
#define SET_KEY_MONITOR_MODE     1
#define SET_INTERACTOR_LATENCY   2
#define SET_TARGET               3
#define SET_TARGET_POSITION      4
#define SET_RELATIVE_POSITION    5
#define SET_MODEL_IM_RELATIVE_TO 6
#define SET_OBJECT_TO_AVOID      7
#define SET_MODEL_IM_FACING      8
#define SET_PERSISTENT_ANIMATION 9
#define SET_TARGET_GOAL          10
#define SET_ANIMATION            11
#define SET_ANIMATION_EVENT      12
#define SET_OBJECT_TO_FOLLOW     13
#define SET_DOUBLE_SCORE         14
#define SET_DIGRESSIVE           15
#define SET_PURGE_FLAG           16
#define SET_BEHAVIOR_PARAM_COUNT 17

#endif
