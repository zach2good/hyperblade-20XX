#include "compat.h"
#include "poolcounts.h"

// Each int count is a dword its pool loads whole (0x00412cb3, against MOV AX at 0x0041304d for a short),
// so its high half carries the short two bytes after it.

short g_wPoolCountCTrack                    = 2;
short g_wPoolCountCHeadTrack                = 2;
short g_wPoolCountCTrackTraj                = 4;
short g_wPoolCountCWatch                    = 1;
short g_wPoolCountCTrackPasser_Receiver     = 5;
short g_wPoolCountCViewIt                   = 2;
short g_wPoolCountCCircleTrack              = 2;
int   g_nPoolCountCGlideOnSurface           = 20;
int   g_nPoolCountCSlideOnRail              = 0x000a0003;
int   g_nPoolCountCBeAttached               = 0x000a0002;
short g_wPoolCountCSteerToTarget            = 10;
int   g_nPoolCountCSetPositionRelative      = 0x00080004;
short g_wPoolCountCSteerToPosition          = 8;
int   g_nPoolCountCSteerBetweenFacing       = 0x00030005;
short g_wPoolCountCSteerToward              = 3;
int   g_nPoolCountCAdjustAcceleration       = 0x00060014;
short g_wPoolCountCAvoidObjects             = 6;
int   g_nPoolCountCBungeeToward             = 0x000a000a;
short g_wPoolCountCSetAttitude              = 10;
int   g_nPoolCountCRelaxYaw                 = 0x000a0007;
short g_wPoolCountCPaletteEffect            = 10;
int   g_nPoolCountCStepAnimation            = 0x0002000a;
short g_wPoolCountCMonitorHeldKeys          = 2;
int   g_nPoolCountCMonitorKeys              = 4;
short g_wPoolCountCTimerActivity            = 10;
int   g_nPoolCountCRelaxToPosition          = 3;
int   g_nPoolCountCPanSound                 = 0x00030003;
short g_wPoolCountCDisplayShotPower         = 3;
int   g_nPoolCountCUpdateShotPower          = 0x00030003;
short g_wPoolCountCDisplayPassBar           = 3;
int   g_nPoolCountCDopplerSnd               = 0x00050003;
short g_wPoolCountCExplosionAnimation       = 5;
int   g_nPoolCountCCheckAI                  = 5;
int   g_nPoolCountBehObjGlideToObject       = 0x000a000a;
short g_wPoolCountBehObjGlideToPosition     = 10;
int   g_nPoolCountBehObjGlideBetweenFacing  = 0x00050005;
short g_wPoolCountBehObjAvoidance           = 5;
int   g_nPoolCountBehObjUpdateRelativePos   = 0x000a0007;
short g_wPoolCountBehObjGlide               = 10;
int   g_nPoolCountBehObjGlideLCL            = 0x0004000a;
short g_wPoolCountBehObjRailSlide           = 4;
int   g_nPoolCountBehObjFly                 = 0x00050005;
short g_wPoolCountBehObjFlyLCL              = 5;
int   g_nPoolCountBehObjFlyToward           = 0x00020002;
short g_wPoolCountBehObjMonitorKeyboard     = 2;
int   g_nPoolCountBehObjUpdateWhileAttached = 0x000a0002;
short g_wPoolCountBehObjSetInPlace          = 10;
int   g_nPoolCountBehObjCoachTeam           = 0x0002000a;
short g_wPoolCountBehObjSetFlatInPlace      = 2;
int   g_nPoolCountBehObjSetMovingInPlace    = 0x00050004;
short g_wPoolCountBehObjDoNothing           = 5;
int   g_nPoolCountBehObjOpenAndClose        = 0x00020002;
short g_wPoolCountBehObjTracking            = 2;
int   g_nPoolCountBehObjHeadTracking        = 0x00020002;
short g_wPoolCountBehObjWatching            = 2;
int   g_nPoolCountBehObjTrackTrajectory     = 0x00050005;
short g_wPoolCountBehObjTrackPassPrep       = 5;
int   g_nPoolCountBehObjCandyTracking       = 2;
short g_wPoolCountCProjectAndBounce         = 10;
int   g_nPoolCountCSpeedBurst               = 0x000a0005;
