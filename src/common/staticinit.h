// The globals the static initialisers read and write.

#ifndef HYPERBLADE_STATICINIT_H
#define HYPERBLADE_STATICINIT_H

// GLOBAL: HYPERX 0x004b39e8
extern float g_flHalfRokObject;
// GLOBAL: HYPERX 0x004b3704
extern float g_flPlanRealDefault;
// GLOBAL: HYPERX 0x004b2a20
extern const float g_flZeroFaceNormal;
// GLOBAL: HYPERX 0x004b23e0
extern const float g_flZeroBehavior;
// GLOBAL: HYPERX 0x004b3c74
extern float g_flStatWeightScale;
// GLOBAL: HYPERX 0x004b1524
extern float g_flOneCollisionPlane;
// GLOBAL: HYPERX 0x004b1484
extern const float g_flZeroPlanLaunch;
// GLOBAL: HYPERX 0x004b15e8
extern const float g_flZeroPhysicsBody;
// GLOBAL: HYPERX 0x004b1698
extern const float g_flZeroTracking;
// GLOBAL: HYPERX 0x004b2848
extern const float g_flZeroCamera;
// GLOBAL: HYPERX 0x004b2a08
extern const double g_dMsPerSecond;
// GLOBAL: HYPERX 0x004b2a4c
extern float g_flOneWidget;
// GLOBAL: HYPERX 0x004b2a58
extern float g_flDotsPerInchWidget;
// GLOBAL: HYPERX 0x004b2a5c
extern float g_flDotPitchWidget;
// GLOBAL: HYPERX 0x004b2a60
extern float g_flCmPerInchWidget;
// GLOBAL: HYPERX 0x004b2ca4
extern float g_flDotsPerInchProjection;
// GLOBAL: HYPERX 0x004b2ca8
extern float g_flDotPitchProjection;
// GLOBAL: HYPERX 0x004b2cac
extern float g_flCmPerInchProjection;
// GLOBAL: HYPERX 0x004b2cb8
extern const float g_flZeroLightLevel;
// GLOBAL: HYPERX 0x004b2d38
extern float g_flOneParseUtil;
// GLOBAL: HYPERX 0x004b2d50
extern float g_flDotsPerInchParseUtil;
// GLOBAL: HYPERX 0x004b2d54
extern float g_flDotPitchParseUtil;
// GLOBAL: HYPERX 0x004b2d58
extern float g_flCmPerInchParseUtil;
// GLOBAL: HYPERX 0x004b30cc
extern const float g_flZeroRadar;
// GLOBAL: HYPERX 0x004b3220
extern const float g_flZeroSWRend;
// GLOBAL: HYPERX 0x004b36e8
extern const float g_flZeroPlan;

// The squared distance a player may stray from its spawn square before it counts as out.
// GLOBAL: HYPERX 0x004b3748
extern float g_flSpawnEscapeDistance;
// GLOBAL: HYPERX 0x004b3738
extern float g_flPlanStatScale;
// GLOBAL: HYPERX 0x004b385c
extern const float g_flZeroSteering;
// GLOBAL: HYPERX 0x004b3864
extern float g_flTwoSteering;
// GLOBAL: HYPERX 0x004b38d8
extern const float g_flZeroVertexColour;
// GLOBAL: HYPERX 0x004b38e8
extern const float g_flZeroEngineObject;
// GLOBAL: HYPERX 0x004b39dc
extern const float g_flZeroRokObject;
// GLOBAL: HYPERX 0x004b3e34
extern const float g_flZeroPlanArg;
// GLOBAL: HYPERX 0x004b3f24
extern const float g_flZeroGameEngine;
// GLOBAL: HYPERX 0x004b3f28
extern float g_flOneGameEngine;
// GLOBAL: HYPERX 0x004b40dc
extern float g_flOneModelFile;
// GLOBAL: HYPERX 0x004b40e8
extern float g_flDotsPerInchModelFile;
// GLOBAL: HYPERX 0x004b40ec
extern float g_flDotPitchModelFile;
// GLOBAL: HYPERX 0x004b40f0
extern float g_flCmPerInchModelFile;
// GLOBAL: HYPERX 0x004c41a0
extern float g_flTrackingEllipsoidScale;
// GLOBAL: HYPERX 0x004c67b0
extern float g_flLightIntensityScale;
// GLOBAL: HYPERX 0x004cb6f8
extern double g_dMsPerMinute;
// GLOBAL: HYPERX 0x004cb770
extern float g_flFifthWidget;
// GLOBAL: HYPERX 0x004cb778
extern float g_flDotPitchCmWidget;
// GLOBAL: HYPERX 0x004cb77c
extern float g_flDotPitchInchWidget;
// GLOBAL: HYPERX 0x004cb808
extern float g_flDotPitchCmProjection;
// GLOBAL: HYPERX 0x004cb80c
extern float g_flDotPitchInchProjection;
// GLOBAL: HYPERX 0x004cd4a4
extern float g_flFifthParseUtil;
// GLOBAL: HYPERX 0x004cd4ac
extern float g_flDotPitchCmParseUtil;
// GLOBAL: HYPERX 0x004cd4b0
extern float g_flDotPitchInchParseUtil;
// GLOBAL: HYPERX 0x004cf048
extern float g_flArenaEllipsoidZ;
// GLOBAL: HYPERX 0x004cf04c
extern float g_flArenaEllipsoidY;
// GLOBAL: HYPERX 0x004cf050
extern float g_flArenaEllipsoidX;
// GLOBAL: HYPERX 0x004d09f4
extern float g_flScaledLightIntensity;
// GLOBAL: HYPERX 0x004d09f8
extern float g_flLightAttenuation;
// GLOBAL: HYPERX 0x004d2174
extern float g_flFifthGameEngine;
// GLOBAL: HYPERX 0x004d2bbc
extern float g_flFifthModelFile;
// GLOBAL: HYPERX 0x004d2bc4
extern float g_flDotPitchCmModelFile;
// GLOBAL: HYPERX 0x004d2bc8
extern float g_flDotPitchInchModelFile;
// GLOBAL: HYPERX 0x004cb774
extern float g_flScreenDotsPerCmWidget;
// GLOBAL: HYPERX 0x004cd4a8
extern float g_flScreenDotsPerCmParseUtil;
// GLOBAL: HYPERX 0x004d2bc0
extern float g_flScreenDotsPerCmModelFile;

// More units' zeros, and the floats each unit sets to its zero and never reads.
// GLOBAL: HYPERX 0x004b3454
extern const float g_flZeroGame;
// GLOBAL: HYPERX 0x004b142c
extern const float g_flZeroPlanTracking;
// GLOBAL: HYPERX 0x004b22b0
extern const float g_flZeroConfigDialog;
// GLOBAL: HYPERX 0x004b2a00
extern const float g_flZeroDistance;
// GLOBAL: HYPERX 0x004b2f78
extern const float g_flZeroDisplayMode;
// GLOBAL: HYPERX 0x004b322c
extern const float g_flZeroDisplay;
// GLOBAL: HYPERX 0x004b3234
extern const float g_flZeroWinMain;
// GLOBAL: HYPERX 0x004b32ac
extern const float g_flZeroSurface2;
// GLOBAL: HYPERX 0x004b3e54
extern const float g_flZeroFace;
// GLOBAL: HYPERX 0x004b4140
extern const float g_flZeroMDM1;
// GLOBAL: HYPERX 0x004cb370
extern float g_flUnusedPlanTrackingA;
// GLOBAL: HYPERX 0x004cb36c
extern float g_flUnusedPlanTrackingB;
// GLOBAL: HYPERX 0x004cb368
extern float g_flUnusedPlanTrackingC;
// SYNTHETIC: HYPERX 0x004017c0
// $E2
// SYNTHETIC: HYPERX 0x004017e0
// $E5
// SYNTHETIC: HYPERX 0x00401800
// $E8
// SYNTHETIC: HYPERX 0x00401810
// $E7

// GLOBAL: HYPERX 0x004cb380
extern float g_flUnusedPlanLaunchA;
// GLOBAL: HYPERX 0x004cb37c
extern float g_flUnusedPlanLaunchB;
// GLOBAL: HYPERX 0x004cb378
extern float g_flUnusedPlanLaunchC;
// SYNTHETIC: HYPERX 0x00403180
// $E2
// SYNTHETIC: HYPERX 0x004031a0
// $E5
// SYNTHETIC: HYPERX 0x004031c0
// $E8
// SYNTHETIC: HYPERX 0x004031d0
// $E7

// GLOBAL: HYPERX 0x004cb3a4
extern float g_flUnusedPhysicsBodyA;
// GLOBAL: HYPERX 0x004cb3a0
extern float g_flUnusedPhysicsBodyB;
// GLOBAL: HYPERX 0x004cb39c
extern float g_flUnusedPhysicsBodyC;
// GLOBAL: HYPERX 0x004cb428
extern float g_flUnusedTrackingA;
// GLOBAL: HYPERX 0x004cb424
extern float g_flUnusedTrackingB;
// GLOBAL: HYPERX 0x004cb420
extern float g_flUnusedTrackingC;
// SYNTHETIC: HYPERX 0x00408db0
// $E2
// SYNTHETIC: HYPERX 0x00408dd0
// $E5
// SYNTHETIC: HYPERX 0x00408df0
// $E8
// SYNTHETIC: HYPERX 0x00408e00
// $E7

// GLOBAL: HYPERX 0x004cb4d4
extern float g_flUnusedAgendaItemA;
// GLOBAL: HYPERX 0x004cb4d0
extern float g_flUnusedAgendaItemB;
// GLOBAL: HYPERX 0x004cb4cc
extern float g_flUnusedAgendaItemC;
// GLOBAL: HYPERX 0x004cb4ec
extern float g_flUnusedConfigDialogA;
// GLOBAL: HYPERX 0x004cb4e8
extern float g_flUnusedConfigDialogB;
// GLOBAL: HYPERX 0x004cb4e4
extern float g_flUnusedConfigDialogC;
// GLOBAL: HYPERX 0x004cb5d8
extern float g_flUnusedCameraA;
// GLOBAL: HYPERX 0x004cb5d4
extern float g_flUnusedCameraB;
// GLOBAL: HYPERX 0x004cb5d0
extern float g_flUnusedCameraC;
// SYNTHETIC: HYPERX 0x00421920
// $E2
// SYNTHETIC: HYPERX 0x00421940
// $E5
// SYNTHETIC: HYPERX 0x00421960
// $E8
// SYNTHETIC: HYPERX 0x00421970
// $E7

// GLOBAL: HYPERX 0x004cb708
extern float g_flUnusedDistanceA;
// GLOBAL: HYPERX 0x004cb704
extern float g_flUnusedDistanceB;
// GLOBAL: HYPERX 0x004cb700
extern float g_flUnusedDistanceC;
// GLOBAL: HYPERX 0x004cd494
extern float g_flUnusedLightLevelA;
// GLOBAL: HYPERX 0x004cd490
extern float g_flUnusedLightLevelB;
// GLOBAL: HYPERX 0x004cd48c
extern float g_flUnusedLightLevelC;
// GLOBAL: HYPERX 0x004cdaa0
extern float g_flUnusedDisplayModeA;
// GLOBAL: HYPERX 0x004cda9c
extern float g_flUnusedDisplayModeB;
// GLOBAL: HYPERX 0x004cda98
extern float g_flUnusedDisplayModeC;
// SYNTHETIC: HYPERX 0x004394c0
// $E2
// SYNTHETIC: HYPERX 0x00439500
// $E8
// SYNTHETIC: HYPERX 0x00439510
// $E7

// GLOBAL: HYPERX 0x004ce6d0
extern float g_flUnusedRadarA;
// GLOBAL: HYPERX 0x004ce6cc
extern float g_flUnusedRadarB;
// GLOBAL: HYPERX 0x004ce6c8
extern float g_flUnusedRadarC;
// GLOBAL: HYPERX 0x004ced54
extern float g_flUnusedSWRendA;
// GLOBAL: HYPERX 0x004ced50
extern float g_flUnusedSWRendB;
// GLOBAL: HYPERX 0x004ced4c
extern float g_flUnusedSWRendC;
// GLOBAL: HYPERX 0x004cee64
extern float g_flUnusedDisplayA;
// GLOBAL: HYPERX 0x004cee60
extern float g_flUnusedDisplayB;
// GLOBAL: HYPERX 0x004cee5c
extern float g_flUnusedDisplayC;
// GLOBAL: HYPERX 0x004cef44
extern float g_flUnusedWinMainA;
// GLOBAL: HYPERX 0x004cef40
extern float g_flUnusedWinMainB;
// GLOBAL: HYPERX 0x004cef3c
extern float g_flUnusedWinMainC;
// GLOBAL: HYPERX 0x004cefdc
extern float g_flUnusedSurface2A;
// GLOBAL: HYPERX 0x004cefd8
extern float g_flUnusedSurface2B;
// GLOBAL: HYPERX 0x004cefd4
extern float g_flUnusedSurface2C;
// GLOBAL: HYPERX 0x004cfa4c
extern float g_flUnusedPlanA;
// GLOBAL: HYPERX 0x004cfa48
extern float g_flUnusedPlanB;
// GLOBAL: HYPERX 0x004cfa44
extern float g_flUnusedPlanC;
// GLOBAL: HYPERX 0x004d01e8
extern float g_flUnusedSteeringA;
// GLOBAL: HYPERX 0x004d01e4
extern float g_flUnusedSteeringB;
// GLOBAL: HYPERX 0x004d01e0
extern float g_flUnusedSteeringC;
// GLOBAL: HYPERX 0x004d05a8
extern float g_flUnusedVertexColourA;
// GLOBAL: HYPERX 0x004d05a4
extern float g_flUnusedVertexColourB;
// GLOBAL: HYPERX 0x004d05a0
extern float g_flUnusedVertexColourC;
// SYNTHETIC: HYPERX 0x0046eeb0
// $E2
// SYNTHETIC: HYPERX 0x0046eef0
// $E8
// SYNTHETIC: HYPERX 0x0046ef00
// $E7

// GLOBAL: HYPERX 0x004d0648
extern float g_flUnusedEngineObjectA;
// GLOBAL: HYPERX 0x004d0644
extern float g_flUnusedEngineObjectB;
// GLOBAL: HYPERX 0x004d0640
extern float g_flUnusedEngineObjectC;
// GLOBAL: HYPERX 0x004d0770
extern float g_flUnusedRokObjectA;
// GLOBAL: HYPERX 0x004d076c
extern float g_flUnusedRokObjectB;
// GLOBAL: HYPERX 0x004d0768
extern float g_flUnusedRokObjectC;
// SYNTHETIC: HYPERX 0x00473a20
// $E2
// SYNTHETIC: HYPERX 0x00473a60
// $E8
// SYNTHETIC: HYPERX 0x00473a70
// $E7

// GLOBAL: HYPERX 0x004d19e4
extern float g_flUnusedPlanArgA;
// GLOBAL: HYPERX 0x004d19e0
extern float g_flUnusedPlanArgB;
// GLOBAL: HYPERX 0x004d19dc
extern float g_flUnusedPlanArgC;
// GLOBAL: HYPERX 0x004d1a64
extern float g_flUnusedFaceA;
// GLOBAL: HYPERX 0x004d1a60
extern float g_flUnusedFaceB;
// GLOBAL: HYPERX 0x004d1a5c
extern float g_flUnusedFaceC;
// GLOBAL: HYPERX 0x004d2194
extern float g_flUnusedGameEngineA;
// GLOBAL: HYPERX 0x004d2190
extern float g_flUnusedGameEngineB;
// GLOBAL: HYPERX 0x004d218c
extern float g_flUnusedGameEngineC;
// GLOBAL: HYPERX 0x004ceff0
extern float g_flUnusedGame;

#endif
