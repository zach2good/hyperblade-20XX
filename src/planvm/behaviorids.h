#ifndef HYPERBLADE_BEHAVIORIDS_H
#define HYPERBLADE_BEHAVIORIDS_H

// A behaviour object's kind, numbered and named as DATA/ENUMIDS.H's Behavior_IDs.
constexpr auto kBehGlideToObject       = 0;
constexpr auto kBehGlide               = 1;
constexpr auto kBehRailSlide           = 2;
constexpr auto kBehFly                 = 3;
constexpr auto kBehMonitorKeyboard     = 4;
constexpr auto kBehUpdateWhileAttached = 5;
constexpr auto kBehSetInPlace          = 6;
constexpr auto kBehOpenAndClose        = 7;
constexpr auto kBehTracking            = 8;
constexpr auto kBehWatching            = 9;
constexpr auto kBehTrackTrajectory     = 10;
constexpr auto kBehGlideToPosition     = 11;
constexpr auto kBehFlyToward           = 12;
constexpr auto kBehTrackPassPrep       = 13;
constexpr auto kBehAvoidance           = 14;
constexpr auto kBehUpdateRelativePos   = 15;
constexpr auto kBehSetFlatInPlace      = 16;
constexpr auto kBehDoNothing           = 17;
constexpr auto kBehSetMovingInPlace    = 18;
constexpr auto kBehGlideBetweenFacing  = 19;
constexpr auto kBehHeadTracking        = 20;
constexpr auto kBehCoachTeam           = 21;
constexpr auto kBehCandyTracking       = 22;
constexpr auto kBehGlideLcl            = 23;
constexpr auto kBehFlyLcl              = 24;
constexpr auto kBehCount               = 25;

// SET_BEHAVIOR_PARAM's handler slots, DATA/ENUMIDS.H's BehaviorSetParamFnIDs.
constexpr auto kSetModsTweenNFacing    = 0;
constexpr auto kSetKeyMonitorMode      = 1;
constexpr auto kSetInteractorLatency   = 2;
constexpr auto kSetTarget              = 3;
constexpr auto kSetTargetPosition      = 4;
constexpr auto kSetRelativePosition    = 5;
constexpr auto kSetModelImRelativeTo   = 6;
constexpr auto kSetObjectToAvoid       = 7;
constexpr auto kSetModelImFacing       = 8;
constexpr auto kSetPersistentAnimation = 9;
constexpr auto kSetTargetGoal          = 10;
constexpr auto kSetAnimation           = 11;
constexpr auto kSetAnimationEvent      = 12;
constexpr auto kSetObjectToFollow      = 13;
constexpr auto kSetDoubleScore         = 14;
constexpr auto kSetDigressive          = 15;
constexpr auto kSetPurgeFlag           = 16;
constexpr auto kSetBehaviorParamCount  = 17;

#endif
