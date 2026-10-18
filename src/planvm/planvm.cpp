#include "common/compat.h"
#include "common/path.h"
#include "planvm/planfunctionids.h"
#include "globals.h"
#include "planvmglobals.h"
#include "planvm.h"
#include "graphics/meshbone.h"
#include "planvm/planfocusarg.h"
struct GameApp;
#include "ui/screen.h"
#include "ui/widget.h"
#include "common/fileobj.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "planopcodetable.h"
#include "physics/collinteractorpool.h"
#include "physics/msginteractorpool.h"
#include "app/gameapp.h"
#include "anim/animevent.h"
#include "physics/collisioncheck.h"
#include "common/parseutil.h"
#include "planvm/planaction.h"
#include "planvm/planarg.h"
#include "planvm/plancond.h"
#include "common/util.h"
#include "planvm/plannode.h"
#include <SDL3/SDL.h>

// The reaction table at 0x004cf9fc, defined in src/engine/object.cpp.
extern short g_awReactionByDamageAndSector[35];

// The names the hit list's entries were found under; nothing reads them.
// GLOBAL: HYPERX 0x004cf4f4
extern char g_aAnimListNames[35][32];

constexpr auto kAnimListLines = 5;

// Seven names to a line, five lines to a file.
constexpr auto kAnimNamesPerLine = 7;

constexpr auto kAnimNameMax = 32;

constexpr auto kAnimLineMax = 100;

// GLOBAL: HYPERX 0x004cf4f0
extern int g_nAnimListNamesLead;

// The condition and opcode half of the dispatch array, and the action half: one storage, two bases.
// GLOBAL: HYPERX 0x004cf084
extern PlanOpcodeSlot g_pfnPlanOpcodeHandlers[141];

// GLOBAL: HYPERX 0x004cf2b8
extern PlanHandlerFn g_pfnPlanActionHandlers[140];

constexpr auto kTokenBuffer = 0x28;

constexpr auto kPlanSourceChars = 80000;
constexpr auto kPlanStreamWords = 30000;

// The five counters and a number token's scratch; marked in src/globals.cpp.
extern short g_wArgsSinceDeclareBehavior;

extern short g_wArgsSinceChangeToBehavior;

extern short g_wArgsSinceChangeOfPlan;

extern short g_wArgsSinceSetInteractorParam;

extern short g_wArgsSinceDeclareLocalReals;

constexpr auto kPlanOpcodeNone = 0xffffffff;

// How far into the token and into each message the appended text is cut off.
constexpr auto kTokenMessageLength = 20;

extern char g_szBadPlanIdValue[20];

constexpr auto kBadPlanIDMessageLength = 19;

extern PlanWord g_planParsedLocalReal;

extern char g_szExpectedArgEnumeration[40];

extern char g_szBadNumberInPlan[24];

constexpr auto kBadNumberMessageLength = 20;

// The four messages, three of which the offending token is appended to.
extern char g_szInabilityToReadString[28];

constexpr auto kUnreadableStringMessageLength = 26;

constexpr auto kPlanStringLimit = 0x28;

// Five action slots are never written, and only the hit list's names are kept.
// FUNCTION: HYPERX 0x0046ac10
void PlanVM::LoadAnimationLists()
{
    char aHitName[kAnimNamesPerLine][kAnimNameMax];
    char aChkName[kAnimNamesPerLine][kAnimNameMax];
    char aThrName[kAnimNamesPerLine][kAnimNameMax];
    char aJmpName[kAnimNamesPerLine][kAnimNameMax];
    char szHitLine[kAnimLineMax];
    char szThrLine[kAnimLineMax];
    char szJmpLine[kAnimLineMax];
    char szChkLine[kAnimLineMax];

    g_pCollInteractorPool = new CollInteractorPool();
    g_pMsgInteractorPool  = new MsgInteractorPool();

    g_nAnimListNamesLead                                                                  = 0;
    g_pfnPlanOpcodeHandlers[kSetInteractorName].pfnSetParam                               = PlanCond::SetInteractorName;
    g_pfnPlanOpcodeHandlers[kSetActiveState].pfnSetParam                                  = PlanCond::SetActiveState;
    g_pfnPlanActionHandlers[kACFSendMessage]                                              = PlanAction::SendMsg;
    g_pfnPlanActionHandlers[kACFReallySendMsg]                                            = PlanAction::ReallySendMsg;
    g_pfnPlanActionHandlers[kACFSetAttachPak]                                             = PlanAction::Set_AttachPak;
    g_pfnPlanActionHandlers[kACFUnsetAttachPak]                                           = PlanAction::Unset_AttachPak;
    g_pfnPlanActionHandlers[kACFRecoilOffWall]                                            = PlanAction::RecoilOffWall;
    g_pfnPlanActionHandlers[kACFReflectOffWall]                                           = PlanAction::ReflectOffWall;
    g_pfnPlanActionHandlers[kACFReflectOffModel]                                          = PlanAction::ReflectOffModel;
    g_pfnPlanActionHandlers[kACFClothesLineRecoil]                                        = PlanAction::ClothesLineRecoil;
    g_pfnPlanActionHandlers[kACFManOnManRecoil]                                           = PlanAction::ManOnManRecoil;
    g_pfnPlanActionHandlers[kACFBounceOffDromer]                                          = PlanAction::BounceOffDromer;
    g_pfnPlanActionHandlers[kACFLaunch]                                                   = PlanAction::Launch;
    g_pfnPlanActionHandlers[kACFLaunchToward]                                             = PlanAction::LaunchToward;
    g_pfnPlanActionHandlers[kACFLaunchAt]                                                 = PlanAction::LaunchAt;
    g_pfnPlanActionHandlers[kACFThrow]                                                    = PlanAction::Throw;
    g_pfnPlanActionHandlers[kACFExplodeFlat]                                              = PlanAction::ExplodeFlat;
    g_pfnPlanActionHandlers[kACFExplodeAgainstVelocity]                                   = PlanAction::ExplodeAgainstVelocity;
    g_pfnPlanActionHandlers[kACFExplodeWithVelocity]                                      = PlanAction::ExplodeWithVelocity;
    g_pfnPlanActionHandlers[kACFExplodeUniform]                                           = PlanAction::ExplodeUniform;
    g_pfnPlanActionHandlers[kACFLoseHead]                                                 = PlanAction::LoseHead;
    g_pfnPlanActionHandlers[kACFRegainHead]                                               = PlanAction::RegainHead;
    g_pfnPlanActionHandlers[kACFBodySplit]                                                = PlanAction::BodySplit;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFBodyPartAnimation].pfnDecide          = PlanCond::GetTeamParity;
    g_pfnPlanActionHandlers[kACFSetSpeedBurst]                                            = PlanAction::Set_SpeedBurst;
    g_pfnPlanActionHandlers[kACFEngageOpponent]                                           = PlanAction::Engage_Opponent;
    g_pfnPlanActionHandlers[kACFPlaySound]                                                = AnimEvent::PlaySnd;
    g_pfnPlanActionHandlers[kACFPlaySndType]                                              = AnimEvent::PlaySndType;
    g_pfnPlanActionHandlers[kACFPanSndType]                                               = AnimEvent::PanSndType;
    g_pfnPlanActionHandlers[kACFDopplerSound]                                             = AnimEvent::DopplerSound;
    g_pfnPlanActionHandlers[kACFPlaceSound]                                               = AnimEvent::PlaySoundByDistance;
    g_pfnPlanActionHandlers[kACFNoteStats4Possessors]                                     = PlanAction::NoteStats4Possessors;
    g_pfnPlanActionHandlers[kACFIncrementStat]                                            = PlanAction::IncrementStat;
    g_pfnPlanActionHandlers[kACFStartStatTimer]                                           = PlanAction::StartStatTimer;
    g_pfnPlanActionHandlers[kACFEndStatTimer]                                             = PlanAction::EndStatTimer;
    g_pfnPlanActionHandlers[kACFSetLocation]                                              = PlanAction::SetLocation;
    g_pfnPlanActionHandlers[kACFSetVelocity]                                              = PlanAction::SetVelocity;
    g_pfnPlanActionHandlers[kACFSetObjectToTrack]                                         = PlanAction::SetObjectToTrack;
    g_pfnPlanActionHandlers[kACFSetTrackToViewOwner]                                      = PlanAction::SetTrackToViewOwner;
    g_pfnPlanActionHandlers[kACFSetTrackingOffset]                                        = PlanAction::SetTrackingOffset;
    g_pfnPlanActionHandlers[kACFIncrementTrackingOffset]                                  = PlanAction::IncrementTrackingOffset;
    g_pfnPlanActionHandlers[kACFIncrementCameraDir]                                       = PlanAction::IncrementCameraDir;
    g_pfnPlanActionHandlers[kACFIncrementBungeeLength]                                    = PlanAction::IncrementBungeeLength;
    g_pfnPlanActionHandlers[kACFSetBungeeLength]                                          = PlanAction::SetBungeeLength;
    g_pfnPlanActionHandlers[kACFSetTrackHeight]                                           = PlanAction::SetTrackHeight;
    g_pfnPlanActionHandlers[kACFSetMinHeight]                                             = PlanAction::SetMinHeight;
    g_pfnPlanActionHandlers[kACFSetLookHeight]                                            = PlanAction::SetLookHeight;
    g_pfnPlanActionHandlers[kACFSetCamBufferFactor]                                       = PlanAction::SetCamBufferFactor;
    g_pfnPlanActionHandlers[kACFSetScanModes]                                             = PlanAction::SetScanModes;
    g_pfnPlanActionHandlers[kACFSetSolarWindFactor]                                       = PlanAction::SetSolarWindFactor;
    g_pfnPlanActionHandlers[kACFSetOffsetValues]                                          = PlanAction::SetOffsetValues;
    g_pfnPlanActionHandlers[kACFSetObjectToWatch]                                         = PlanAction::SetObjectToWatch;
    g_pfnPlanActionHandlers[kACFIncrementObjectToWatch]                                   = PlanAction::IncrementObjectToWatch;
    g_pfnPlanActionHandlers[kACFIncrementObjectToTrack]                                   = PlanAction::IncrementObjectToTrack;
    g_pfnPlanActionHandlers[kACFResetCameraBufferSize]                                    = PlanAction::ResetCameraBufferSize;
    g_pfnPlanActionHandlers[kACFResetReticle]                                             = PlanAction::ResetReticle;
    g_pfnPlanActionHandlers[kACFSetHeadCamText]                                           = PlanAction::SetHeadCamText;
    g_pfnPlanActionHandlers[kACFInitRearViewPos]                                          = PlanAction::InitRearViewPos;
    g_pfnPlanActionHandlers[kACFSetCamOffsetToLocation]                                   = PlanAction::SetCamOffsetToLocation;
    g_pfnPlanActionHandlers[kACFRelaxYawToFace]                                           = PlanAction::RelaxYawToFace;
    g_pfnPlanActionHandlers[kACFSetLocalReal]                                             = PlanAction::SetLocalReal;
    g_pfnPlanActionHandlers[kACFSetRenderFlag]                                            = PlanAction::SetRenderFlag;
    g_pfnPlanActionHandlers[kACFIncRokValue]                                              = PlanAction::IncRokValue;
    g_pfnPlanActionHandlers[kACFSetRokKillState]                                          = PlanAction::SetRokKillState;
    g_pfnPlanActionHandlers[kACFSetMyGravity]                                             = PlanAction::SetMyGravity;
    g_pfnPlanActionHandlers[kACFModifyDamage]                                             = PlanAction::ModifyDamage;
    g_pfnPlanActionHandlers[kACFModifyPitch]                                              = PlanAction::ModifyPitch;
    g_pfnPlanActionHandlers[kACFHoverAboveArmory]                                         = PlanAction::HoverAboveArmory;
    g_pfnPlanActionHandlers[kACFDeployWeapon]                                             = PlanAction::DeployWeapon;
    g_pfnPlanActionHandlers[kACFSetInDrag]                                                = PlanAction::SetInDrag;
    g_pfnPlanActionHandlers[kACFSetOnRim]                                                 = PlanAction::SetOnRim;
    g_pfnPlanActionHandlers[kACFSetFriction]                                              = PlanAction::SetFriction;
    g_pfnPlanActionHandlers[kACFModifyRating]                                             = PlanAction::ModifyRating;
    g_pfnPlanActionHandlers[kACFModifyAvailableReds]                                      = PlanAction::ModifyAvailableReds;
    g_pfnPlanActionHandlers[kACFSetAnimation]                                             = PlanAction::SetAnimation;
    g_pfnPlanActionHandlers[kACFRestorePersistentAnimation]                               = PlanAction::RestorePersistentAnimation;
    g_pfnPlanActionHandlers[kACFSetAnimationEvent]                                        = PlanAction::SetAnimationEvent;
    g_pfnPlanActionHandlers[kACFSetAnimationState]                                        = PlanAction::SetAnimationState;
    g_pfnPlanActionHandlers[kACFSetAnimationSpeed]                                        = PlanAction::SetAnimationSpeed;
    g_pfnPlanActionHandlers[kACFIncrAnimationSpeed]                                       = PlanAction::IncrAnimationSpeed;
    g_pfnPlanActionHandlers[kACFSetAnimationDoneMessage]                                  = PlanAction::SetAnimationDoneMessage;
    g_pfnPlanActionHandlers[kACFAnimationPeek]                                            = GameApp::AnimationPeek;
    g_pfnPlanActionHandlers[kACFHookDragBungeesOn]                                        = PlanAction::HookDragBungeesOn;
    g_pfnPlanActionHandlers[kACFResetAccTransient]                                        = PlanAction::ResetAccTransient;
    g_pfnPlanActionHandlers[kACFThrowBungee]                                              = PlanAction::ThrowBungee;
    g_pfnPlanActionHandlers[kACFResetKeystateFlags]                                       = PlanAction::ResetKeystateFlags;
    g_pfnPlanActionHandlers[kACFLaunchGunDirection]                                       = PlanAction::LaunchGunDirection;
    g_pfnPlanActionHandlers[kACFSetPaletteEffect]                                         = PlanAction::SetPaletteEffect;
    g_pfnPlanActionHandlers[kACFSignalKeystate]                                           = PlanAction::SignalKeystate;
    g_pfnPlanActionHandlers[kACFGetKeystateFlags]                                         = PlanAction::GetKeystateFlags;
    g_pfnPlanActionHandlers[kACFScaleBubToRating]                                         = PlanAction::ScaleBubToRating;
    g_pfnPlanActionHandlers[kACFSetTargetWithData]                                        = PlanAction::SetTargetWithData;
    g_pfnPlanActionHandlers[kACFBungeeTowardTarget]                                       = PlanAction::BungeeTowardTarget;
    g_pfnPlanActionHandlers[kACFGlideOnRamp]                                              = PlanAction::GlideOnRamp;
    g_pfnPlanActionHandlers[kACFGrayscaleRandomizer]                                      = PlanAction::GrayscaleRandomizer;
    g_pfnPlanActionHandlers[kACFSetImmunity]                                              = PlanAction::SetImmunity;
    g_pfnPlanActionHandlers[kACFSpeedBurstSituation]                                      = PlanAction::SpeedBurstSituation;
    g_pfnPlanActionHandlers[kACFIncrAnimSpeedWRating]                                     = PlanAction::IncrAnimSpeedWRating;
    g_pfnPlanActionHandlers[kACFModifyOpposTeamFriction]                                  = PlanAction::ModifyOpposTeamFriction;
    g_pfnPlanActionHandlers[kACFSetDeviceOwner]                                           = PlanAction::SetDeviceOwner;
    g_pfnPlanActionHandlers[kACFSetInputMode]                                             = PlanAction::SetInputMode;
    g_pfnPlanActionHandlers[kACFIncrementScore]                                           = PlanAction::IncrementScore;
    g_pfnPlanActionHandlers[kACFSwapRandomizers]                                          = PlanAction::SwapRandomizers;
    g_pfnPlanActionHandlers[kACFSetShotPowerMeter]                                        = PlanAction::SetShotPowerMeter;
    g_pfnPlanActionHandlers[kACFSetAttackee]                                              = PlanAction::SetAttackee;
    g_pfnPlanActionHandlers[kACFModifySpeedRating]                                        = PlanAction::ModifySpeedRating;
    g_pfnPlanActionHandlers[kACFSetKnockedDownState]                                      = PlanAction::SetKnockedDownState;
    g_pfnPlanActionHandlers[kACFSpinTurnstile]                                            = PlanAction::SpinTurnstile;
    g_pfnPlanActionHandlers[kACFTrace]                                                    = PlanAction::Trace;
    g_pfnPlanActionHandlers[kACFSetKilledDromer]                                          = PlanAction::SetKilledDromer;
    g_pfnPlanActionHandlers[kACFAnimationSemaphore]                                       = PlanAction::AnimationSemaphore;
    g_pfnPlanActionHandlers[kACFResetCameraTrack]                                         = PlanAction::ResetCameraTrack;
    g_pfnPlanActionHandlers[kACFSetInfoBarText]                                           = PlanAction::SetInfoBarText;
    g_pfnPlanActionHandlers[kACFClockStoppedSemaphore]                                    = PlanAction::ClockStoppedSemaphore;
    g_pfnPlanActionHandlers[kACFCreditDamage]                                             = PlanAction::CreditDamage;
    g_pfnPlanActionHandlers[kACFTrackTurnstile]                                           = PlanAction::TrackTurnstile;
    g_pfnPlanActionHandlers[kACFDesiredSubstitute]                                        = PlanAction::DesiredSubstitute;
    g_pfnPlanActionHandlers[kACFSubstituteDromer]                                         = PlanAction::SubstitutePlayer;
    g_pfnPlanActionHandlers[kACFSetInfoTextDromer]                                        = PlanAction::SetInfoTextDromer;
    g_pfnPlanActionHandlers[kACFSetInfoTextTeam]                                          = PlanAction::SetInfoTextTeam;
    g_pfnPlanActionHandlers[kACFClearBehaviorAnimation]                                   = PlanAction::ClearBehaviorAnimation;
    g_pfnPlanActionHandlers[kACFIfNotRokTrackMe]                                          = PlanAction::IfNotRokTrackMe;
    g_pfnPlanActionHandlers[kACFSendMessageToMyCamera]                                    = PlanAction::SendMessageToMyCamera;
    g_pfnPlanActionHandlers[kACFTransferViewOwnership]                                    = PlanAction::TransferViewOwnership;
    g_pfnPlanActionHandlers[kACFSmallBloodSpurt]                                          = PlanAction::SmallBloodSpurt;
    g_pfnPlanActionHandlers[kACFLargeBloodSpurt]                                          = PlanAction::LargeBloodSpurt;
    g_pfnPlanActionHandlers[kACFSetMaxSpeed]                                              = PlanAction::SetMaxSpeed;
    g_pfnPlanActionHandlers[kACFEndPeriodNow]                                             = PlanAction::EndPeriodNow;
    g_pfnPlanActionHandlers[kACFKillBallStaging]                                          = PlanAction::KillBallStaging;
    g_pfnPlanActionHandlers[kACFKillBallExecute]                                          = PlanAction::KillBallExecute;
    g_pfnPlanActionHandlers[kACFKickOffRim]                                               = PlanAction::KickOffRim;
    g_pfnPlanActionHandlers[kACFFlyingSemaphore]                                          = PlanAction::FlyingSemaphore;
    g_pfnPlanActionHandlers[kACFKickDownToSurface]                                        = PlanAction::KickDownToSurface;
    g_pfnPlanActionHandlers[kACFSyncPossession]                                           = PlanAction::SyncPossession;
    g_pfnPlanActionHandlers[kACFSyncSystem]                                               = PlanAction::SyncSystem;
    g_pfnPlanActionHandlers[kACFSyncDamage]                                               = PlanAction::SyncDamage;
    g_pfnPlanActionHandlers[kACFSendLaunchMessage]                                        = PlanAction::SendLaunchMessage;
    g_pfnPlanActionHandlers[kACFSetLaserHurdleCharge]                                     = PlanAction::SetLaserHurdleCharge;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerLaserHurdleCollision].pfnDecide = CollisionCheck::DromerLaserHurdleCollision;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFGetLaserHurdleCharge].pfnDecide       = Util::GetLaserHurdleCharge;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecideAttachment].pfnDecide           = PlanCond::Decide_Attachment;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecideCelebration].pfnDecide          = PlanCond::Decide_Celebration;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecidePassOrRun].pfnDecide            = PlanCond::Decide_PassOrRun;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecideHPSpinRailOrNot].pfnDecide      = PlanCond::Decide_HPSpin_Rail_orNot;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFPickACell].pfnDecide                  = PlanCond::PickACell;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRandom].pfnDecide                     = PlanCond::Random;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFShotPower].pfnDecide                  = PlanCond::ShotPower;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIfInFaceOff].pfnDecide                = PlanCond::IfInFaceOff;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIsSenderTeammate].pfnDecide           = PlanCond::IsSenderTeammate;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAmMainDromer].pfnDecide               = PlanCond::AmMainDromer;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMyID].pfnDecide                       = PlanCond::MyID;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMessageData].pfnDecide                = PlanCond::SetResultDirect;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMessageDataNibl0].pfnDecide           = PlanCond::MessageDataNibl0;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMessageDataNibl1].pfnDecide           = PlanCond::MessageDataNibl1;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMessageDataNibl4].pfnDecide           = PlanCond::MessageDataNibl4;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMsgData4Nibl1X2Nibl0].pfnDecide       = PlanCond::MsgData4Nibl1X2Nibl0;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFSideKeysAreDown].pfnDecide            = PlanCond::SideKeysAreDown;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAmViewOwner].pfnDecide                = PlanCond::AmViewOwner;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTeammateIsViewOwner].pfnDecide        = PlanCond::TeammateIsViewOwner;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOwnerIsViewOwner].pfnDecide           = PlanCond::OwnerIsViewOwner;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFViewOwnersMateInFront].pfnDecide      = PlanCond::ViewOwnersMateInFront;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIHaveRok].pfnDecide                   = PlanCond::IHaveRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerHasRok].pfnDecide               = PlanCond::DromerHasRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRunTimeData].pfnDecide                = PlanCond::SetResultDirect;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIsRokInMyZone].pfnDecide              = PlanCond::IsRokInMyZone;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecidePosition].pfnDecide             = PlanCond::DecidePosition;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFCollisionDetail].pfnDecide            = PlanCond::CollisionDetail;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFEngageWhichOpponent].pfnDecide        = PlanCond::EngageWhichOpponent;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTeammateHasRok].pfnDecide             = PlanCond::TeammateHasRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFGagTheAnnouncer].pfnDecide            = PlanCond::GagTheAnnouncer;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFVictimCamIsOn].pfnDecide              = PlanCond::VictimCamIsOn;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIsScanningModeOn].pfnDecide           = PlanCond::IsScanningModeOn;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecideBlockOrGoOut].pfnDecide         = PlanCond::Decide_BlockOrGoOut;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFWhichOppnIsClosestToRok].pfnDecide    = PlanCond::WhichOppnIsClosestToRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFClosestUncoveredOppn].pfnDecide       = PlanCond::ClosestUncoveredOppn;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAnalyzePadA].pfnDecide                = PlanCond::AnalyzePadA;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAnalyzePadB].pfnDecide                = PlanCond::AnalyzePadB;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAnalyzePadC].pfnDecide                = PlanCond::AnalyzePadC;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAttackerIsInRear].pfnDecide           = PlanCond::AttackerIsInRear;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFSpeedBurstIsRelaxed].pfnDecide        = PlanCond::SpeedBurstIsRelaxed;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOffenseOrWRok].pfnDecide              = PlanCond::OffenseOrWRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFHostIsViewOwner].pfnDecide            = PlanCond::HostIsViewOwner;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRokInWhichTeamsPoss].pfnDecide        = PlanCond::RokInWhichTeamsPoss;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIsDoubleScore].pfnDecide              = PlanCond::IsDoubleScore;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFBungeeVictimLorR].pfnDecide           = PlanCond::BungeeVictimLorR;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFSpinUp].pfnDecide                     = PlanCond::SpinUp;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAmIKnockedLoose].pfnDecide            = PlanCond::AmIKnockedLoose;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRokScoreValue].pfnDecide              = PlanCond::RokScoreValue;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFCatchBlockOrMiss].pfnDecide           = PlanCond::CatchBlockOrMiss;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFJumping].pfnDecide                    = PlanCond::Jumping;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDucking].pfnDecide                    = PlanCond::Ducking;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerHurdleCollision].pfnDecide      = CollisionCheck::DromerHurdleCollision;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerMultiplierCollision].pfnDecide  = CollisionCheck::DromerMultiplierCollision;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerLimboCollision].pfnDecide       = CollisionCheck::DromerLimboCollision;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerRandomizerCollision].pfnDecide  = CollisionCheck::DromerRandomizerCollision;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerRampCollision].pfnDecide        = CollisionCheck::DromerRampCollision;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRokHurdleCollision].pfnDecide         = CollisionCheck::RokHurdleCollision;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFChoosePassingPlay].pfnDecide          = PlanCond::ChoosePassingPlay;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRecvrInRangeNView].pfnDecide          = PlanCond::RecvrInRangeNView;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFWhichOppnIsClosestToMe].pfnDecide     = PlanCond::WhichOppnIsClosestToMe;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTakeDamage].pfnDecide                 = PlanCond::TakeDamage;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIsAmbient].pfnDecide                  = AnimEvent::IsAmbient;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRokIsCharged].pfnDecide               = PlanCond::RokIsCharged;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRokIsChargedAgainst].pfnDecide        = PlanCond::RokIsChargedAgainst;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFClosestTargetToProjectile].pfnDecide  = PlanCond::ClosestTargetToProjectile;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMeOrTeammateClosestToRok].pfnDecide   = PlanCond::MeOrTeammateClosestToRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRokValueInMyFavor].pfnDecide          = PlanCond::RokValueInMyFavor;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRedIsAvailable].pfnDecide             = PlanCond::RedIsAvailable;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRatingMaxedOut].pfnDecide             = PlanCond::RatingMaxedOut;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFPassOrRunByRatings].pfnDecide         = PlanCond::PassOrRunByRatings;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFLooseRokWhatToDo].pfnDecide           = PlanCond::LooseRokWhatToDo;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFGoThroughMuliplier].pfnDecide         = PlanCond::GoThroughMuliplier;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFPickAttackMove].pfnDecide             = PlanCond::PickAttackMove;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOffWingJob].pfnDecide                 = PlanCond::OffWingJob;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFPickFlyingMove].pfnDecide             = PlanCond::PickFlyingMove;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFPickHalfPipeMove].pfnDecide           = PlanCond::PickHalfpipeMove;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAvoidSpinningTurnstile].pfnDecide     = PlanCond::AvoidSpinningTurnstile;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTargetTurnstile].pfnDecide            = PlanCond::DecideNever;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFUseKillball].pfnDecide                = PlanCond::DecideNever;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFUseRandomizer].pfnDecide              = PlanCond::DecideNever;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFShootSituation].pfnDecide             = PlanCond::ShootSituation;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRegroupSituation].pfnDecide           = PlanCond::RegroupSituation;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAggressionOnOthers].pfnDecide         = PlanCond::AggressionOnOthers;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTenOrMorePtSpread].pfnDecide          = PlanCond::TenOrMorePtSpread;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMyTeamLosingByAtLeast].pfnDecide      = PlanCond::MyTeamLosingByAtLeast;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDromerNearMine].pfnDecide             = PlanCond::DromerNearMine;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAttackOrDeployWeapon].pfnDecide       = PlanCond::AttackOrDeployWeapon;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecideWithRating].pfnDecide           = PlanCond::DecideWithRating;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFNearObjectBy].pfnDecide               = PlanCond::NearObjectBy;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFChooseTeamTune].pfnDecide             = PlanCond::ChooseTeamTune;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFSoundBank].pfnDecide                  = PlanCond::SoundBank;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFHomeOrAwayTeam].pfnDecide             = PlanCond::GetTeamParity;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFWinningOrLosing].pfnDecide            = PlanCond::WinningOrLosing;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIsDromerKnockedDown].pfnDecide        = PlanCond::IsDromerKnockedDown;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRandomEmptyArmory].pfnDecide          = PlanCond::RandomEmptyArmory;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRandomInactiveWeapon].pfnDecide       = PlanCond::RandomInactiveWeapon;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFWhoKilledMe].pfnDecide                = PlanCond::WhoKilledMe;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOkToSubstituteDromer].pfnDecide       = PlanAction::SubstitutePlayer;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAnimationSemaphore].pfnDecide         = PlanAction::AnimationSemaphore;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFClockStoppedSemaphore].pfnDecide      = PlanAction::ClockStoppedSemaphore;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAttachedToSender].pfnDecide           = PlanCond::AttachedToSender;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFAttackedAnim].pfnDecide               = PlanCond::AttackedAnim;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFClosestDromerToRok].pfnDecide         = PlanCond::ClosestDromerToRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFRokInRange].pfnDecide                 = PlanCond::RokInRange;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFIsDromerInjured].pfnDecide            = PlanCond::IsDromerInjured;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTeammateIsHumanOrInjured].pfnDecide   = PlanCond::TeammateIsHumanOrInjured;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDesiredSubstitute].pfnDecide          = PlanAction::DesiredSubstitute;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFSubstitutionSemaphore].pfnDecide      = PlanCond::SubstitutionSemaphore;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOkToSwitchToTeammate].pfnDecide       = PlanCond::OkToSwitchToTeammate;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOkToPassToTeammate].pfnDecide         = PlanCond::OkToPassToTeammate;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOkToSubstituteTeammate].pfnDecide     = PlanCond::OkToSubstituteTeammate;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFPickValidSubstitute].pfnDecide        = PlanCond::PickValidSubstitute;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOkToGetRok].pfnDecide                 = PlanCond::OkToGetRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFOkToLaunchRok].pfnDecide              = PlanCond::OkToLaunchRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFGetImmunity].pfnDecide                = PlanCond::GetImmunity;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFInterception].pfnDecide               = PlanCond::Interception;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFSpecialAttacks].pfnDecide             = PlanCond::SpecialAttacks;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFInRangeToAttack].pfnDecide            = PlanCond::InRangeToAttack;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFJostledDropRok].pfnDecide             = PlanCond::JostledDropRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFBouncedInGoal].pfnDecide              = CollisionCheck::BouncedInGoal;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFCheckPassingDistance].pfnDecide       = PlanCond::CheckPassingDistance;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFWhichOppnToAttack].pfnDecide          = PlanCond::WhichOppnToAttack;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFCheckRenderFlag].pfnDecide            = PlanCond::CheckRenderFlag;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTeamIsDead].pfnDecide                 = PlanCond::TeamIsDead;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTeamIsInjured].pfnDecide              = PlanCond::TeamIsInjured;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFMyTeamIsInjured].pfnDecide            = PlanCond::MyTeamIsInjured;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFWhoWonTheGame].pfnDecide              = PlanCond::WhoWonTheGame;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFWhoHasRok].pfnDecide                  = PlanCond::WhoHasRok;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTryForKillball].pfnDecide             = PlanCond::TryForKillball;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFTargetAcquired].pfnDecide             = PlanCond::TargetAcquired;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFDecideGoalieLaunch].pfnDecide         = PlanCond::DecideGoalieLaunch;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFNearestSuspenseMult].pfnDecide        = PlanCond::NearestSuspenseMult;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFBallGotBehindMe].pfnDecide            = PlanCond::BallGotBehindMe;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFGoaliePassingTarget].pfnDecide        = PlanCond::GoaliePassingTarget;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFForceRearView].pfnDecide              = PlanCond::ForceRearView;
    g_pfnPlanOpcodeHandlers[kPlanDecisionBase + kDCFCheckScoreOnOwnGoal].pfnDecide        = PlanCond::CheckScoreOnOwnGoal;

    // Marked in src/graphics/colorpalette.cpp: without /GF each object keeps its own copy.
    // STRING: HYPERX 0x004c4704
    FILE* const pHitFile = fopen(Path::Native(String::Concat(g_gameApp.m_strExeDir.m_str, "HitAnims.lst")).c_str(), "r");
    // STRING: HYPERX 0x004c46f4
    FILE* const pChkFile = fopen(Path::Native(String::Concat(g_gameApp.m_strExeDir.m_str, "ChkAnims.lst")).c_str(), "r");
    // STRING: HYPERX 0x004c46e4
    FILE* const pJmpFile = fopen(Path::Native(String::Concat(g_gameApp.m_strExeDir.m_str, "JmpAnims.lst")).c_str(), "r");
    // STRING: HYPERX 0x004c46d4
    FILE* const pThrFile = fopen(Path::Native(String::Concat(g_gameApp.m_strExeDir.m_str, "ThrAnims.lst")).c_str(), "r");

    if (pHitFile == nullptr || pChkFile == nullptr || pJmpFile == nullptr || pThrFile == nullptr)
    {
        // STRING: HYPERX 0x004c46ac
        g_gameApp.Fatal("Error opening Animation List files.\n");
    }

    short* pwIndex              = g_awReactionByDamageAndSector;
    char (*pName)[kAnimNameMax] = g_aAnimListNames;
    int nLine                   = kAnimListLines;

    do
    {
        ParseUtil::ReadLine(pHitFile, szHitLine);
        ParseUtil::ReadLine(pChkFile, szChkLine);
        ParseUtil::ReadLine(pJmpFile, szJmpLine);
        ParseUtil::ReadLine(pThrFile, szThrLine);

        // STRING: HYPERX 0x004c4694
        const int nHitRead = sscanf(szHitLine, "%s\t%s\t%s\t%s\t%s\t%s\t%s", aHitName[0], aHitName[1], aHitName[2], aHitName[3], aHitName[4], aHitName[5], aHitName[6]);
        const int nChkRead = sscanf(szChkLine, "%s\t%s\t%s\t%s\t%s\t%s\t%s", aChkName[0], aChkName[1], aChkName[2], aChkName[3], aChkName[4], aChkName[5], aChkName[6]);
        const int nJmpRead = sscanf(szJmpLine, "%s\t%s\t%s\t%s\t%s\t%s\t%s", aJmpName[0], aJmpName[1], aJmpName[2], aJmpName[3], aJmpName[4], aJmpName[5], aJmpName[6]);
        const int nThrRead = sscanf(szThrLine, "%s\t%s\t%s\t%s\t%s\t%s\t%s", aThrName[0], aThrName[1], aThrName[2], aThrName[3], aThrName[4], aThrName[5], aThrName[6]);

        if (nHitRead != kAnimNamesPerLine || nChkRead != nHitRead || nJmpRead != nHitRead || nThrRead != nHitRead)
        {
            // STRING: HYPERX 0x004c4670
            g_gameApp.Fatal("Error reading Animation List file.\n");
        }

        char (*pSource)[kAnimNameMax] = aHitName;
        char (*pDest)[kAnimNameMax]   = pName;
        short* pwWork                 = pwIndex;
        int    nName                  = kAnimNamesPerLine;

        do
        {
            *pwWork = g_pAnimRegistry->FindAnimByNameIdx(*pSource);

            strcpy(*pDest, *pSource);

            pwWork++;
            pSource++;
            pDest++;

            nName--;
        } while (nName != 0);

        pName   = pDest;
        pwIndex = pwWork;

        nLine--;
    } while (nLine != 0);

    fclose(pHitFile);
    fclose(pChkFile);
    fclose(pJmpFile);
    fclose(pThrFile);
}

// FUNCTION: HYPERX 0x0047cd60
void PlanVM::InitRuntimeOpcodeTable()
{
    g_apPlanArgFns[PRN_ME]                        = PlanArg::GetMe;
    g_apPlanArgFns[PRN_MY_TEAMMATE]               = PlanArg::GetMyTeammate;
    g_apPlanArgFns[PRN_MY_LEAD_OPPONENT]          = PlanArg::GetMyLeadOpponent;
    g_apPlanArgFns[PRN_MY_OTHER_OPPONENT]         = PlanArg::GetMyOtherOpponent;
    g_apPlanArgFns[PRN_MY_GOAL]                   = PlanArg::GetMyGoal;
    g_apPlanArgFns[PRN_MY_GOALIE]                 = PlanArg::GetMyGoalie;
    g_apPlanArgFns[PRN_MY_LEAD_TEAMMATE]          = PlanArg::GetMyLeadTeammate;
    g_apPlanArgFns[PRN_MY_GOAL_SIDE]              = PlanArg::GetMyGoalSide;
    g_apPlanArgFns[PRN_MY_GOAL_ZONE]              = PlanArg::GetMyGoalZone;
    g_apPlanArgFns[PRN_OPPONENTS_GOAL]            = PlanArg::GetOpponentsGoal;
    g_apPlanArgFns[PRN_OPPN_GOAL_SIDE]            = PlanArg::GetOppnGoalSide;
    g_apPlanArgFns[PRN_OPPN_GOAL_ZONE]            = PlanArg::GetOppnGoalZone;
    g_apPlanArgFns[PRN_MY_COLLISION_BUB]          = PlanArg::GetMyCollisionBub;
    g_apPlanArgFns[PRN_MY_TEAMMATES_COB]          = PlanArg::GetMyTeammatesCOB;
    g_apPlanArgFns[PRN_MY_AWARENESS_BUB]          = PlanArg::GetMyAwarenessBub;
    g_apPlanArgFns[PRN_MY_LEAD_OPPN_COLL_BUB]     = PlanArg::GetMyLeadOppnCollBub;
    g_apPlanArgFns[PRN_MY_OTHER_OPPN_COLL_BUB]    = PlanArg::GetMyOtherOppnCollBub;
    g_apPlanArgFns[PRN_MY_BODY_BUBBLE]            = PlanArg::GetMyBodyBubble;
    g_apPlanArgFns[PRN_LEAD_OPPN_BODY_BUB]        = PlanArg::GetLeadOppnBodyBub;
    g_apPlanArgFns[PRN_OTHER_OPPN_BODY_BUB]       = PlanArg::GetOtherOppnBodyBub;
    g_apPlanArgFns[PRN_THIS_RAMP_COB]             = PlanArg::GetRampCollisionObject;
    g_apPlanArgFns[PRN_THIS_MULTIPLIER_COB]       = PlanArg::GetMultiplierCollisionObject;
    g_apPlanArgFns[PRN_THIS_TURNSTILE_COB]        = PlanArg::GetTurnstileCollisionObject;
    g_apPlanArgFns[PRN_THIS_LIMBO_POLE_COB]       = PlanArg::GetLimboPoleCollisionObject;
    g_apPlanArgFns[PRN_THIS_LASER_HURDLE_COB]     = PlanArg::GetLaserHurdleCollisionObject;
    g_apPlanArgFns[PRN_THIS_FAST_LANE_COB]        = PlanArg::GetFastLaneCollisionObject;
    g_apPlanArgFns[PRN_THIS_SLOW_LANE_COB]        = PlanArg::GetSlowLaneCollisionObject;
    g_apPlanArgFns[PRN_THIS_MINE_COB]             = PlanArg::GetMineCollisionObject;
    g_apPlanArgFns[PRN_THIS_SWARF_COB]            = PlanArg::GetSwarfCollisionObject;
    g_apPlanArgFns[PRN_THIS_EXPLOSION_COB]        = PlanArg::GetExplosionCollisionObject;
    g_apPlanArgFns[PRN_THIS_ARMORY_COB]           = PlanArg::GetArmoryCollisionObject;
    g_apPlanArgFns[PRN_THIS_HURDLE_COB]           = PlanArg::GetHurdleCollisionObject;
    g_apPlanArgFns[PRN_THIS_RED_RANDOMIZER_COB]   = PlanArg::GetRedRandomizerCollisionObject;
    g_apPlanArgFns[PRN_THIS_GREEN_RANDOMIZER_COB] = PlanArg::GetGreenRandomizerCollisionObject;
    g_apPlanArgFns[PRN_MY_BACK_DEF_WG_POS]        = PlanArg::GetMyBackDefWgPos;
    g_apPlanArgFns[PRN_MY_GOAL_POS]               = PlanArg::GetMyGoalPos;
    g_apPlanArgFns[PRN_MY_DRAG_START_POS]         = PlanArg::GetMyDragStartPos;
    g_apPlanArgFns[PRN_VIEW_OWNER]                = PlanArg::GetViewOwner;
    g_apPlanArgFns[PRN_VIEW_OWNER_COB]            = PlanArg::GetViewOwnerCOB;
    g_apPlanArgFns[PRN_VW_OWNR_AWARNS_BUB]        = PlanArg::GetVwOwnrAwarnsBub;
    g_apPlanArgFns[PRN_VIEW_ONRS_OPPN1_COB]       = PlanArg::GetViewOnrsOppn1COB;
    g_apPlanArgFns[PRN_VIEW_ONRS_OPPN2_COB]       = PlanArg::GetViewOnrsOppn2COB;
    g_apPlanArgFns[PRN_WATCHED_OBJECT]            = PlanArg::GetWatchedObject;
    g_apPlanArgFns[PRN_TRACKED_OBJECT]            = PlanArg::GetTrackedObject;
    g_apPlanArgFns[PRN_TRACKED_OBJS_TEAMMATE]     = PlanArg::GetTrackedObjsTeammate;
    g_apPlanArgFns[PRN_VIEW_OWNERS_GOAL]          = PlanArg::GetViewOwnersGoal;
    g_apPlanArgFns[PRN_VW_OWNR_OPPN_GOALIE]       = PlanArg::GetVwOwnrOppnGoalie;
    g_apPlanArgFns[PRN_VIEW_OWNERS_TEAMMATE]      = PlanArg::GetViewOwnersTeammate;
    g_apPlanArgFns[PRN_VIEW_ONRS_OPPN1]           = PlanArg::GetViewOnrsOppn1;
    g_apPlanArgFns[PRN_VIEW_ONRS_OPPN2]           = PlanArg::GetViewOnrsOppn2;
    g_apPlanArgFns[PRN_VW_OWNRS_OPPN_GOAL]        = PlanArg::GetVwOwnrsOppnGoal;
    g_apPlanArgFns[PRN_ROK_CARRIER]               = PlanArg::GetRokCarrier;
    g_apPlanArgFns[PRN_LAST_ROK_CARRIER]          = PlanArg::GetLastRokCarrier;
    g_apPlanArgFns[PRN_PREV_ROK_CARRIER]          = PlanArg::GetPrevRokCarrier;
    g_apPlanArgFns[PRN_ROK_CARRIER_COB]           = PlanArg::GetRokCarrierCOB;
    g_apPlanArgFns[PRN_NON_ROK_CARRYING_OPPN]     = PlanArg::GetNonRokCarryingOppn;

    g_apPlanStatFns[PRN_VIEW_OWNER]            = PlanFocusArg::GetViewOwner;
    g_apPlanStatFns[PRN_VIEW_OWNER_COB]        = PlanFocusArg::GetViewOwnerCOB;
    g_apPlanStatFns[PRN_VW_OWNR_AWARNS_BUB]    = PlanFocusArg::GetVwOwnrAwarnsBub;
    g_apPlanStatFns[PRN_VIEW_ONRS_OPPN1_COB]   = PlanFocusArg::GetViewOnrsOppn1COB;
    g_apPlanStatFns[PRN_VIEW_ONRS_OPPN2_COB]   = PlanFocusArg::GetViewOnrsOppn2COB;
    g_apPlanStatFns[PRN_WATCHED_OBJECT]        = PlanFocusArg::GetWatchedObject;
    g_apPlanStatFns[PRN_TRACKED_OBJECT]        = PlanFocusArg::GetTrackedObject;
    g_apPlanStatFns[PRN_TRACKED_OBJS_TEAMMATE] = PlanFocusArg::GetTrackedObjsTeammate;
    g_apPlanStatFns[PRN_VIEW_OWNERS_GOAL]      = PlanFocusArg::GetViewOwnersGoal;
    g_apPlanStatFns[PRN_VW_OWNR_OPPN_GOALIE]   = PlanFocusArg::GetVwOwnrOppnGoalie;
    g_apPlanStatFns[PRN_VIEW_OWNERS_TEAMMATE]  = PlanFocusArg::GetViewOwnersTeammate;
    g_apPlanStatFns[PRN_VIEW_ONRS_OPPN1]       = PlanFocusArg::GetViewOnrsOppn1;
    g_apPlanStatFns[PRN_VIEW_ONRS_OPPN2]       = PlanFocusArg::GetViewOnrsOppn2;
    g_apPlanStatFns[PRN_ROK_CARRIER]           = PlanFocusArg::GetRokCarrier;
    g_apPlanStatFns[PRN_LAST_ROK_CARRIER]      = PlanFocusArg::GetLastRokCarrier;
    g_apPlanStatFns[PRN_PREV_ROK_CARRIER]      = PlanFocusArg::GetLastRokCarrier;
    g_apPlanStatFns[PRN_ROK_CARRIER_COB]       = PlanFocusArg::GetRokCarrierCOB;
    g_apPlanStatFns[PRN_NON_ROK_CARRYING_OPPN] = PlanFocusArg::GetNonRokCarryingOppn;

    g_apPlanArgFns[PRN_THIS_BODY_PART_COB] = PlanArg::GetBodyPartCollisionObject;
}

// The tokens the compiler seeds by name before the numbered _0_ to _50_.
constexpr auto kPlanNamedTokens       = 6;
constexpr auto kPlanLastNumberedToken = 50;

// Builds the two keyword tables; each ends in an empty-name entry valued minus one.
// FUNCTION: HYPERX 0x0047d340
PlanVM::PlanVM()
{
    char szToken[8];

    m_pszText = nullptr;
    m_pStream = nullptr;

    PlanOpcodeRef* pEntry = &m_aOpcodes[0];

    // STRING: HYPERX 0x004c6d0c
    strcpy(pEntry->szName, "SET_COLLISION_INTERACTOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_COLLISION_INTERACTOR) | MEMBER_SET_COLLISION_INTERACTOR;

    pEntry = &m_aOpcodes[1];

    // STRING: HYPERX 0x004c6cf4
    strcpy(pEntry->szName, "SET_MESSAGE_INTERACTOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_MESSAGE_INTERACTOR) | MEMBER_SET_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[2];

    // STRING: HYPERX 0x004c6cd4
    strcpy(pEntry->szName, "SET_LOC_COLLISION_INTERACTOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_COLLISION_INTERACTOR) | MEMBER_SET_LOC_COLLISION_INTERACTOR;

    pEntry = &m_aOpcodes[3];

    // STRING: HYPERX 0x004c6cb4
    strcpy(pEntry->szName, "SET_NET_COLLISION_INTERACTOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_MESSAGE_INTERACTOR) | MEMBER_SET_NET_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[4];

    // STRING: HYPERX 0x004c6c98
    strcpy(pEntry->szName, "SET_LOC_MESSAGE_INTERACTOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_MESSAGE_INTERACTOR) | MEMBER_SET_LOC_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[5];

    // STRING: HYPERX 0x004c6c7c
    strcpy(pEntry->szName, "SET_NET_MESSAGE_INTERACTOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_MESSAGE_INTERACTOR) | MEMBER_SET_NET_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[6];

    // STRING: HYPERX 0x004c6c6c
    strcpy(pEntry->szName, "ADD_AGENDA_ITEM");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_AGENDA) | MEMBER_ADD_AGENDA_ITEM;

    pEntry = &m_aOpcodes[7];

    // STRING: HYPERX 0x004c6c58
    strcpy(pEntry->szName, "REMOVE_AGENDA_ITEM");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_AGENDA) | MEMBER_REMOVE_AGENDA_ITEM;

    pEntry = &m_aOpcodes[8];

    // STRING: HYPERX 0x004c6c48
    strcpy(pEntry->szName, "SET_TIMEOUTMSG");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_AGENDA) | MEMBER_SET_TIMEOUTMSG;

    pEntry = &m_aOpcodes[9];

    // STRING: HYPERX 0x004c6c34
    strcpy(pEntry->szName, "GLOBAL_INTERACTORS");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = CATEGORY_GLOBAL_INTERACTORS;

    pEntry = &m_aOpcodes[10];

    // STRING: HYPERX 0x004c6c24
    strcpy(pEntry->szName, "CHANGE_OF_PLAN");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_PLAN_CHANGE) | MEMBER_CHANGE_OF_PLAN;

    pEntry = &m_aOpcodes[11];

    // STRING: HYPERX 0x004c6c10
    strcpy(pEntry->szName, "CHANGE_TO_BEHAVIOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_CHANGE_TO_BEHAVIOR;

    pEntry = &m_aOpcodes[12];

    // STRING: HYPERX 0x004c6bfc
    strcpy(pEntry->szName, "DECLARE_BEHAVIOR");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_DECLARE_BEHAVIOR;

    pEntry = &m_aOpcodes[13];

    // STRING: HYPERX 0x004c6be8
    strcpy(pEntry->szName, "DECLARE_LOCAL_REALS");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_PLAN_CHANGE) | MEMBER_DECLARE_LOCAL_REALS;

    pEntry = &m_aOpcodes[14];

    // STRING: HYPERX 0x004c6bd4
    strcpy(pEntry->szName, "SET_BEHAVIOR_PARAM");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_SET_BEHAVIOR_PARAM;

    pEntry = &m_aOpcodes[15];

    // STRING: HYPERX 0x004c6bbc
    strcpy(pEntry->szName, "SET_INTERACTOR_PARAM");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_ACTION) | MEMBER_SET_INTERACTOR_PARAM;

    pEntry = &m_aOpcodes[16];

    // STRING: HYPERX 0x004c6bb0
    strcpy(pEntry->szName, "DO_ACTION");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_ACTION) | MEMBER_DO_ACTION;

    pEntry = &m_aOpcodes[17];

    // STRING: HYPERX 0x004c6ba0
    strcpy(pEntry->szName, "DO_LOCAL_ACTION");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_ACTION) | MEMBER_DO_LOCAL_ACTION;

    pEntry = &m_aOpcodes[18];

    // STRING: HYPERX 0x004c6b90
    strcpy(pEntry->szName, "DO_NET_ACTION");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_ACTION) | MEMBER_DO_NET_ACTION;

    pEntry = &m_aOpcodes[19];

    // STRING: HYPERX 0x004c6b80
    strcpy(pEntry->szName, "DECIDE_BY_AMONG");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_DECIDE) | MEMBER_DECIDE_BY_AMONG;

    pEntry = &m_aOpcodes[20];

    // STRING: HYPERX 0x004c6b68
    strcpy(pEntry->szName, "DECIDE_BY_WITH_AMONG");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_DECIDE) | MEMBER_DECIDE_BY_WITH_AMONG;

    pEntry = &m_aOpcodes[21];

    // STRING: HYPERX 0x004c6b5c
    strcpy(pEntry->szName, "SET_DEBUG");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_DEBUG) | MEMBER_SET_DEBUG;

    pEntry = &m_aOpcodes[22];

    // STRING: HYPERX 0x004c6b50
    strcpy(pEntry->szName, "SET_TRACE");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<int>(CATEGORY_DEBUG) | MEMBER_SET_TRACE;

    pEntry = &m_aOpcodes[23];

    // STRING: HYPERX 0x004c6b48
    strcpy(pEntry->szName, "Enable");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<unsigned int>(CATEGORY_ENABLE);

    pEntry = &m_aOpcodes[24];

    // STRING: HYPERX 0x004c6b40
    strcpy(pEntry->szName, "Block");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = CATEGORY_BLOCK;

    pEntry = &m_aOpcodes[25];

    // STRING: HYPERX 0x004c6b34
    strcpy(pEntry->szName, "DataBlock");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = CATEGORY_DATA_BLOCK;

    pEntry = &m_aOpcodes[26];

    // STRING: HYPERX 0x004c6b28
    strcpy(pEntry->szName, "END_PLAN");
    pEntry->nMagic = kPlanMarkerNode;
    pEntry->nValue = static_cast<unsigned int>(CATEGORY_END_PLAN);

    pEntry = &m_aOpcodes[27];

    strcpy(pEntry->szName, g_pszPlanVmEmptyName);
    pEntry->nMagic = 0xffffffff;
    pEntry->nValue = kPlanOpcodeNone;

    pEntry = &m_aTokenRefs[0];

    // STRING: HYPERX 0x004c6b1c
    strcpy(pEntry->szName, "_BROADCAST_");
    pEntry->nValue = kPlanTokenBroadcast;

    pEntry = &m_aTokenRefs[1];

    // STRING: HYPERX 0x004c6b0c
    strcpy(pEntry->szName, "_STRAIGHTAHEAD_");
    pEntry->nValue = static_cast<unsigned int>(PLAN_TOKEN_STRAIGHTAHEAD);

    pEntry = &m_aTokenRefs[2];

    // STRING: HYPERX 0x004c6afc
    strcpy(pEntry->szName, "_TEMPORARILY_");
    pEntry->nValue = static_cast<unsigned int>(PLAN_TOKEN_TEMPORARILY);

    pEntry = &m_aTokenRefs[3];

    // STRING: HYPERX 0x004c6aec
    strcpy(pEntry->szName, "_MESSAGE_DATA_");
    pEntry->nValue = static_cast<unsigned int>(PLAN_TOKEN_MESSAGE_DATA);

    pEntry = &m_aTokenRefs[4];

    // STRING: HYPERX 0x004c6adc
    strcpy(pEntry->szName, "_NO_BEH_CHANGE_");
    pEntry->nValue = static_cast<unsigned int>(PLAN_TOKEN_NO_BEH_CHANGE);

    short nSlot = kPlanNamedTokens;

    pEntry = &m_aTokenRefs[5];

    // STRING: HYPERX 0x004c6acc
    strcpy(pEntry->szName, "_PREVIOUS_BEH_");
    pEntry->nValue = static_cast<unsigned int>(PLAN_TOKEN_PREVIOUS_BEH);

    int nIndex = 0;

    do
    {
        // STRING: HYPERX 0x004c6ac4
        sprintf(szToken, "_%hd_", nIndex);

        pEntry = &m_aTokenRefs[nSlot++];

        strcpy(pEntry->szName, szToken);
        pEntry->nValue = nIndex;

        nIndex++;
    } while (static_cast<short>(nSlot - kPlanNamedTokens) <= kPlanLastNumberedToken);

    pEntry = &m_aTokenRefs[nSlot];

    strcpy(pEntry->szName, g_pszPlanVmEmptyName);
    pEntry->nValue = static_cast<unsigned int>(-1);

    m_pszText = new char[kPlanSourceChars];

    if (m_pszText != nullptr)
    {
        m_pszText[0] = 0;
    }

    m_pStream = new PlanWord[kPlanStreamWords]();
}

// FUNCTION: HYPERX 0x0047dab0
PlanVM::~PlanVM()
{
    if (m_pszText != nullptr)
    {
        delete[] m_pszText;
        m_pszText = nullptr;
    }

    if (m_pStream != nullptr)
    {
        delete[] m_pStream;
        m_pStream = nullptr;
    }
}

constexpr auto kPathChars = 0x100;

// The sentinel goes out too, and the descriptor is closed twice on the success path.
// FUNCTION: HYPERX 0x0047daf0
char PlanVM::SaveBinary(char* pszName)
{
    OwnedFile file;
    char      szPath[kPathChars];
    int       nWord;

    snprintf(szPath, sizeof(szPath), "%s\\%s.pbn", g_gameApp.m_strPlansPath.m_str.m_pchData, pszName);

    if (file.m_file.Open(szPath, FILE_MODE_WRITE) != 0)
    {
        file.m_file.Write(reinterpret_cast<char*>(&m_nPos), sizeof(m_nPos));

        int nIndex = 0;

        do
        {
            nWord = m_pStream[nIndex++].n;

            file.m_file.Write((const void*)&nWord, 4);
        } while (nWord != static_cast<int>(CATEGORY_END_PLAN));

        file.m_file.CloseAndTest();

        return 1;
    }

    // STRING: HYPERX 0x004c6d28
    throw "Cannot open binary plan file. Is it locked?\n";
}

// One short of the buffer, so the terminator fits.
constexpr auto kPlanTextLimit = 0x1387f;

// Only the first byte of each of the four tables is cleared: a leading zero marks an empty slot.
// FUNCTION: HYPERX 0x0047dca0
char PlanVM::ReadAll(FileObj* pFile)
{
    char ch;
    int  nSlot;

    m_nLength = 0;

    if (m_pszText == nullptr || m_pStream == nullptr)
    {
        return 0;
    }

    while (pFile->Read(&ch, 1) != 0)
    {
        m_pszText[m_nLength] = ch;

        m_nLength++;

        if (m_nLength >= kPlanTextLimit)
        {
            // STRING: HYPERX 0x004c6d6c
            throw "This plan file is too large.  Why not break it up into separate plans?\n";
        }
    }

    m_pszText[m_nLength] = 0;

    m_nCursor = 0;
    m_nPos    = 0;

    for (nSlot = 0; nSlot < kPlanVMBehaviorLimit; nSlot++)
    {
        m_aBehaviorNames[nSlot][0]  = 0;
        m_aLocalRealNames[nSlot][0] = 0;

        for (int nRow = 0; nRow < kPlanVMMessagesPerBehavior; nRow++)
        {
            m_aMessageNames[nSlot * kPlanVMMessagesPerBehavior + nRow][0] = 0;
        }
    }

    for (nSlot = 0; nSlot < kPlanVMInteractorNameLimit; nSlot++)
    {
        m_aInteractorNames[nSlot][0] = 0;
    }

    return 1;
}

constexpr auto kPlanPathChars = 0x100;

// The path is built beside the executable; a file that will not open throws.
// FUNCTION: HYPERX 0x0047dd80
char PlanVM::LoadText(short nCamera, char* pszName)
{
    char      szPath[kPlanPathChars];
    OwnedFile file;

    snprintf(szPath, sizeof(szPath), "%s\\%s.txt", g_gameApp.m_strPlansPath.m_str.m_pchData, pszName);

    m_nCamera = nCamera;

    if (file.m_file.Open(szPath, FILE_MODE_READ) != 0)
    {
        const char bResult = ReadAll(&file.m_file);

        file.m_file.CloseAndTest();

        return bResult;
    }

    // STRING: HYPERX 0x004c6db4
    throw "Plan file could not be opened.\n";
}

// An emit that fails throws, so the compile has one error path, which the editor dialog catches.

// FUNCTION: HYPERX 0x0047df00
unsigned char PlanVM::Compile()
{
    if (ParseHeaderDeclarations() == 0)
    {
        return 0;
    }

    m_nCursor = 0;

    while (m_nCursor < m_nLength)
    {
        if (m_nCursor > m_nLength)
        {
            break;
        }

        if (m_pszText[m_nCursor] == 0)
        {
            break;
        }

        if (EmitArgToken() == 0)
        {
            // STRING: HYPERX 0x004c6ddc
            throw (char*)"Aborting Plan Parse.\n";
        }

        if (m_nCursor >= m_nLength)
        {
            break;
        }
    }

    return 1;
}

// The message name keeps its quotes, and the first behaviour's messages land in row one.
// FUNCTION: HYPERX 0x0047df70
char PlanVM::ParseHeaderDeclarations()
{
    PlanWord count;
    char     szToken[0x28];

    short nBehaviors = 0;
    short nMessages  = 0;
    short nParams    = 0;

    while (m_nCursor < m_nLength && m_pszText[m_nCursor] != 0)
    {
        ReadNextToken(szToken);

        // STRING: HYPERX 0x004c6be8
        if (SDL_strcasecmp(szToken, "DECLARE_LOCAL_REALS") == 0)
        {
            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken) && !IsNumericChar(szToken))
            {
                // STRING: HYPERX 0x004c6ec0
                throw "Expected an argument enumerator.  First two args are id and string name.\n";
            }

            ParseNumber(szToken, &count);

            for (short nSlot = 0; nSlot < static_cast<short>(count.n); nSlot++)
            {
                ReadNextToken(szToken);

                if (szToken[0] != '"')
                {
                    // STRING: HYPERX 0x004c6e8c
                    throw "Expected a string name in LocalVar Declaration.\n";
                }

                strcpy(m_aLocalRealNames[nSlot],
                       PlanVM::UnquoteString(szToken));
            }
        }

        // STRING: HYPERX 0x004c6bfc
        if (SDL_strcasecmp(szToken, "DECLARE_BEHAVIOR") == 0)
        {
            nMessages = 0;

            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken) && !IsNumericChar(szToken))
            {
                // STRING: HYPERX 0x004c6e68
                throw "Expected an argument enumerator.\n";
            }

            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken))
            {
                // STRING: HYPERX 0x004c6e48
                throw "Expected an id_BEH_ type id.\n";
            }

            ReadNextToken(szToken);

            if (szToken[0] != '"')
            {
                // STRING: HYPERX 0x004c6e14
                throw "Expected a string name in Behavior Declaration.\n";
            }

            strcpy(m_aBehaviorNames[nBehaviors++],
                   PlanVM::UnquoteString(szToken));
        }

        // STRING: HYPERX 0x004c6cf4
        if (SDL_strcasecmp(szToken, "SET_MESSAGE_INTERACTOR") == 0)
        {
            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken) && !IsNumericChar(szToken))
            {
                throw "Expected an argument enumerator.\n";
            }

            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken) && !IsNumericChar(szToken))
            {
                // STRING: HYPERX 0x004c6df4
                throw "Expected an id_MSG_ type id.\n";
            }

            strcpy(m_aMessageNames[nBehaviors * kPlanVMMessagesPerBehavior + nMessages++],
                   szToken);
        }

        // STRING: HYPERX 0x004c6bbc
        if (SDL_strcasecmp(szToken, "SET_INTERACTOR_PARAM") == 0)
        {
            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken) && !IsNumericChar(szToken))
            {
                throw "Expected an argument enumerator.\n";
            }

            ReadNextToken(szToken);

            // STRING: HYPERX 0x004c6f0c
            if (SDL_strcasecmp(szToken, "id_SET_InteractorName") == 0)
            {
                ReadNextToken(szToken);

                if (szToken[0] != '"')
                {
                    throw "Expected a string name in Behavior Declaration.\n";
                }

                strcpy(m_aInteractorNames[nParams++],
                       PlanVM::UnquoteString(szToken));
            }
        }

        if (m_nCursor >= m_nLength)
        {
            break;
        }
    }

    return 1;
}

// The one character that makes a token a float.
// GLOBAL: HYPERX 0x004b3e50
extern const char* const g_pszDecimalPoint;

// Four characters end a token: the two whitespace ones, the comma and the ampersand.
// FUNCTION: HYPERX 0x0047e3a0
int PlanVM::IsSeparatorChar()
{
    const char cAt = m_pszText[m_nCursor];

    if (cAt == ' ' || cAt == '	' || cAt == ',' || cAt == '&')
    {
        return 1;
    }

    return 0;
}

// Whether the token under the cursor opens a comment or a group.
// FUNCTION: HYPERX 0x0047e3d0
int PlanVM::IsCommentOrGroup()
{
    if ((m_pszText[m_nCursor] == '/' && (m_pszText[m_nCursor + 1] == m_pszText[m_nCursor] || m_pszText[m_nCursor + 1] == '*')) || m_pszText[m_nCursor] == '(' || m_pszText[m_nCursor] == '#')
    {
        return 1;
    }

    return 0;
}

// Accepts four prefixes: "id", a leading underscore, "A_" and "AA_".
// FUNCTION: HYPERX 0x0047e400
unsigned char PlanVM::IsIdentifierPrefix(const char* pszToken)
{
    int bMatch;

    if ((pszToken[0] == 'i' && pszToken[1] == 'd') || pszToken[0] == '_' || (pszToken[0] == 'A' && pszToken[1] == '_') || (pszToken[0] == 'A' && pszToken[1] == 'A' && pszToken[2] == '_'))
    {
        bMatch = 1;
    }
    else
    {
        bMatch = 0;
    }

    return static_cast<unsigned char>(bMatch);
}

// FUNCTION: HYPERX 0x0047e440
int PlanVM::IsAAPrefix(const char* pszToken)
{
    if (pszToken[0] == 'A' && pszToken[1] == 'A' && pszToken[2] == '_')
    {
        return 1;
    }

    return 0;
}

// Nothing checks what follows, so "+" alone is numeric.
// FUNCTION: HYPERX 0x0047e470
unsigned char PlanVM::IsNumericChar(const char* pszToken)
{
    const char cFirst = pszToken[0];

    int bMatch;

    if ((cFirst >= '0' && cFirst <= '9') || cFirst == '.' || cFirst == '+' || cFirst == '-')
    {
        bMatch = 1;
    }
    else
    {
        bMatch = 0;
    }

    return static_cast<unsigned char>(bMatch);
}

// FUNCTION: HYPERX 0x0047e4a0
unsigned char PlanVM::IsPRN(const char* pOpcode)
{
    int bMatch;

    if (pOpcode[3] == 'P' && pOpcode[4] == 'R' && pOpcode[5] == 'N')
    {
        bMatch = 1;
    }
    else
    {
        bMatch = 0;
    }

    return static_cast<unsigned char>(bMatch);
}

// FUNCTION: HYPERX 0x0047e4d0
char PlanVM::EmitArgToken()
{
    PlanOpcodeRef tok;
    char          szToken[kTokenBuffer];

    ReadNextToken(szToken);

    g_wArgsSinceDeclareBehavior++;
    g_wArgsSinceChangeToBehavior++;
    g_wArgsSinceChangeOfPlan++;
    g_wArgsSinceSetInteractorParam++;
    g_wArgsSinceDeclareLocalReals++;

    if (IsIdentifierPrefix(szToken))
    {
        tok = LookupSymbol(szToken);

        if (tok.nValue == kPlanOpcodeNone)
        {
            szToken[kTokenMessageLength]                = 0;
            g_szBadPlanIdValue[kBadPlanIDMessageLength] = 0;
            strcat(g_szBadPlanIdValue, szToken);

            throw g_szBadPlanIdValue;
        }

        if (IsPRN(tok.szName))
        {
            tok.nValue = PlanWord::Pack(kPlanArgPronoun, tok.nValue);
        }

        if (g_wArgsSinceDeclareLocalReals == 1 && ParseNumber(szToken, &g_planParsedLocalReal) == 0)
        {
            throw g_szExpectedArgEnumeration;
        }

        m_pStream[m_nPos].n = tok.nValue;
        m_nPos++;
    }
    else if (IsNumericChar(szToken))
    {
        PlanWord* const pSlot = &m_pStream[m_nPos];
        m_nPos++;

        if (ParseNumber(szToken, pSlot) == 0)
        {
            szToken[kTokenMessageLength]                 = 0;
            g_szBadNumberInPlan[kBadNumberMessageLength] = 0;
            strcat(g_szBadNumberInPlan, szToken);

            throw g_szBadNumberInPlan;
        }
    }

    if (szToken[0] == '"')
    {
        short nIndex = FindLocalRealIndex(szToken);

        if (nIndex >= 0)
        {
            m_pStream[m_nPos].n = PlanWord::Pack(kPlanArgLocalReal, nIndex);
            m_nPos++;

            return 1;
        }

        nIndex = FindInteractorNameIndex(szToken);

        if (nIndex >= 0)
        {
            m_pStream[m_nPos].n = PlanWord::Pack(kPlanArgInteractorName, nIndex);
            m_nPos++;

            return 1;
        }
        else if (g_wArgsSinceDeclareBehavior == 3)
        {
            if (EmitStringLiteral(szToken, m_pStream, &m_nPos) == 0)
            {
                g_szInabilityToReadString[kUnreadableStringMessageLength] = 0;
                strcat(g_szInabilityToReadString, szToken);

                throw g_szInabilityToReadString;
            }

            return 1;
        }
        else if (g_wArgsSinceChangeOfPlan == 2)
        {
            if (EmitStringLiteral(szToken, m_pStream, &m_nPos) == 0)
            {
                g_szInabilityToReadString[kUnreadableStringMessageLength] = 0;
                strcat(g_szInabilityToReadString, szToken);

                throw g_szInabilityToReadString;
            }

            strcpy(tok.szName, PlanVM::UnquoteString(szToken));

            return 1;
        }
        else if (g_wArgsSinceChangeOfPlan == 3)
        {
            auto* const pSub = new PlanVM;

            if (pSub != nullptr)
            {
                pSub->LoadSymbolTables();

                pSub->LoadText(m_nCamera, tok.szName);

                pSub->ParseHeaderDeclarations();

                m_pStream[m_nPos].n = pSub->FindBehaviorIndexOrThrow(PlanVM::UnquoteString(szToken));
                m_nPos++;

                delete pSub;
            }

            return 1;
        }
        else if (g_wArgsSinceChangeToBehavior == 2)
        {
            nIndex = FindBehaviorIndexOrThrow(PlanVM::UnquoteString(szToken));

            if (nIndex >= 0)
            {
                m_pStream[m_nPos].n = nIndex;
                m_nPos++;

                return 1;
            }

            return 0;
        }
        else if (g_wArgsSinceSetInteractorParam == 3)
        {
            nIndex = FindInteractorNameIndexOrThrow(PlanVM::UnquoteString(szToken));

            if (nIndex >= 0)
            {
                m_pStream[m_nPos].n = nIndex;
                m_nPos++;

                return 1;
            }

            return 0;
        }
        else
        {
            if (EmitStringLiteral(szToken, m_pStream, &m_nPos) == 0)
            {
                g_szInabilityToReadString[kUnreadableStringMessageLength] = 0;
                strcat(g_szInabilityToReadString, szToken);

                throw g_szInabilityToReadString;
            }

            return 1;
        }
    }

    tok = LookupKeyword(szToken);

    switch (tok.nValue)
    {
        case static_cast<int>(CATEGORY_PLAN_CHANGE) | MEMBER_CHANGE_OF_PLAN:
            g_wArgsSinceChangeOfPlan = 0;
            break;

        case static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_DECLARE_BEHAVIOR:
            g_wArgsSinceDeclareBehavior = 0;
            break;

        case static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_CHANGE_TO_BEHAVIOR:
            g_wArgsSinceChangeToBehavior = 0;
            break;

        case static_cast<int>(CATEGORY_PLAN_CHANGE) | MEMBER_DECLARE_LOCAL_REALS:
            g_wArgsSinceDeclareLocalReals = 0;
            break;

        case static_cast<int>(CATEGORY_ACTION) | MEMBER_SET_INTERACTOR_PARAM:
            g_wArgsSinceSetInteractorParam = 0;
            break;
    }

    if (strlen(tok.szName) != 0)
    {
        for (short nWord = 0; nWord < 2; nWord++)
        {
            if (nWord == 0)
            {
                m_pStream[m_nPos].n = tok.nMagic;
            }
            else if (nWord == 1)
            {
                m_pStream[m_nPos].n = tok.nValue;
            }
            else
            {
                continue;
            }

            m_nPos++;
        }
    }

    return 1;
}

// A float token's bit pattern is stored raw where the other readings store an integer.
// FUNCTION: HYPERX 0x0047ea50
char PlanVM::ParseNumber(char* pszText, PlanWord* pOut)
{
    PlanWord value;

    // STRING: HYPERX 0x004c6f94
    if (pszText[strspn(pszText, "xXABCDEFabcdef0123456789.+-_")] == 0)
    {
        const int nLength = static_cast<int>(strlen(pszText)) + 1;

        char* pszCopy = new char[nLength];

        // STRING: HYPERX 0x004bb810
        sscanf(pszText, "%s", pszCopy);

        if (*pszCopy == '_' && pszCopy[nLength - 2] == '_')
        {
            value.n = atol(PlanVM::UnquoteString(pszCopy));
        }
        else if (nLength < 3 || *pszCopy != '0' || (pszCopy[1] != 'x' && pszCopy[1] != 'X')
                 // STRING: HYPERX 0x004bb808
                 || sscanf(pszText, "%x", &value.n) == 0)
        {
            if (strpbrk(pszText, g_pszDecimalPoint) != nullptr)
            {
                value.fl = static_cast<float>(atof(pszText));
            }
            else
            {
                value.n = atol(pszText);
            }
        }

        delete[] pszCopy;

        *pOut = value;

        return 1;
    }

    return 0;
}

// The last character over the limit is already in the stream when the failure is reported.
// FUNCTION: HYPERX 0x0047eb30
char PlanVM::EmitStringLiteral(const char* pszToken, PlanWord* paStream, int* pnPos)
{
    paStream[*pnPos].n = kPlanMarkerString;

    short iChar = 1;

    char cAt = pszToken[1];

    while (cAt != '"')
    {
        if (cAt == '_')
        {
            cAt = ' ';
        }

        paStream[*pnPos + iChar].n = cAt;

        if (iChar > kPlanStringLimit)
        {
            return 0;
        }

        iChar++;

        cAt = pszToken[iChar];
    }

    paStream[*pnPos + iChar].n = 0;

    iChar++;

    *pnPos = *pnPos + iChar;

    return 1;
}

// A closing brace with nothing copied is itself the token; an empty run goes round again.
// FUNCTION: HYPERX 0x0047ebb0
void PlanVM::ReadNextToken(char* pToken)
{
    char cSeparator;
    char cAt;

    char  cComment    = 0;
    bool  bNewline    = false;
    char  bCloseBrace = 0;
    short nLength     = 0;
    char  bEnd        = 0;

    SkipSeparators();

    for (;;)
    {
        cSeparator = static_cast<char>(IsSeparatorChar());

        while (cSeparator == 0)
        {
            cComment = static_cast<char>(IsCommentOrGroup());

            if (cComment != 0)
            {
                break;
            }

            cAt = m_pszText[m_nCursor];

            if (cAt == '\r' || cAt == '\n')
            {
                bNewline = true;
                break;
            }

            bNewline = false;

            bCloseBrace = cAt == '}';

            if (bCloseBrace != 0)
            {
                break;
            }

            if (m_nCursor > m_nLength)
            {
                break;
            }

            if (cAt == 0)
            {
                break;
            }

            pToken[nLength] = cAt;
            nLength++;

            m_nCursor++;

            cSeparator = static_cast<char>(IsSeparatorChar());

            if (cSeparator != 0)
            {
                break;
            }
        }

        if (cSeparator != 0)
        {
            SkipSeparators();
        }

        if (cComment != 0 || bNewline)
        {
            SkipLine();

            bNewline = false;
            cComment = 0;
        }

        if (bCloseBrace != 0 && nLength == 0)
        {
            pToken[0] = m_pszText[m_nCursor];

            m_nCursor++;

            pToken[1] = 0;
            return;
        }

        if (m_nCursor > m_nLength || m_pszText[m_nCursor] == 0)
        {
            bEnd = 1;
            break;
        }

        if (nLength != 0)
        {
            break;
        }
    }

    if (bEnd != 0)
    {
        pToken[nLength] = 0;
        return;
    }

    pToken[nLength] = 0;
}

// Skips the rest of the line; the second scan only moves past a lone carriage return.
// FUNCTION: HYPERX 0x0047ecd0
short PlanVM::SkipLine()
{
    while (!AtLineEnd())
    {
        m_nCursor++;
    }

    if (m_pszText[m_nCursor] == '\r')
    {
        m_nCursor++;
    }

    while (!AtLineEnd())
    {
        m_nCursor++;
    }

    if (m_pszText[m_nCursor] == '\n')
    {
        m_nCursor++;
    }

    return 0;
}

// FUNCTION: HYPERX 0x0047ed20
short PlanVM::SkipSeparators()
{
    short nSkipped = 0;

    while (static_cast<char>(IsSeparatorChar()))
    {
        m_nCursor++;

        nSkipped++;
    }

    return nSkipped;
}

// The address test on each entry is the array's own, so it never fails; the original has it too.
// FUNCTION: HYPERX 0x0047ed50
short PlanVM::FindLocalRealIndex(char* pszName)
{
    char szName[0x28];

    short nSlot = 0;

    strcpy(szName, pszName);
    strcpy(szName, PlanVM::UnquoteString(szName));

    while (nSlot < kPlanVMBehaviorLimit)
    {
        if (strcmp(szName, m_aLocalRealNames[nSlot]) == 0)
        {
            return nSlot;
        }

        nSlot++;
    }

    return -1;
}

// FUNCTION: HYPERX 0x0047ee30
short PlanVM::FindInteractorNameIndex(char* pszName)
{
    char szName[0x28];

    short nSlot = 0;

    strcpy(szName, pszName);
    strcpy(szName, PlanVM::UnquoteString(szName));

    while (nSlot < kPlanVMInteractorNameLimit)
    {
        if (strcmp(szName, m_aInteractorNames[nSlot]) == 0)
        {
            return nSlot;
        }

        nSlot++;
    }

    return -1;
}

// FUNCTION: HYPERX 0x0047ef10
PlanOpcodeRef PlanVM::LookupKeyword(char* pszName)
{
    short nSlot = 0;

    while (m_aOpcodes[nSlot].nValue != kPlanOpcodeNone)
    {
        if (strcmp(pszName, m_aOpcodes[nSlot].szName) == 0)
        {
            return m_aOpcodes[nSlot];
        }

        nSlot++;
    }

    return m_aOpcodes[nSlot];
}

// The token table is compared exactly and the loaded table without case.
// FUNCTION: HYPERX 0x0047efc0
PlanOpcodeRef PlanVM::LookupSymbol(char* pszName)
{
    short nToken = 0;

    while (m_aTokenRefs[nToken].nValue != kPlanOpcodeNone)
    {
        if (strcmp(pszName, m_aTokenRefs[nToken].szName) == 0)
        {
            return m_aTokenRefs[nToken];
        }

        nToken++;
    }

    short nSymbol = 0;

    while (m_aSymbols[nSymbol].nValue != kPlanOpcodeNone)
    {
        if (SDL_strcasecmp(pszName, m_aSymbols[nSymbol].szName) == 0)
        {
            return m_aSymbols[nSymbol];
        }

        nSymbol++;
    }

    return m_aSymbols[nSymbol];
}

// A refusal appends the name to a global message and throws it as a char pointer.

// Each base message's own length, which is where the appended name starts.
constexpr auto kBehaviorMessageLength   = 31;
constexpr auto kInteractorMessageLength = 33;

// The messages the refusals build; marked in src/globals.cpp.
extern char g_szCouldNotFindBehaviorNamed[32];
extern char g_szCouldNotFindInteractorNamed[34];

// The behaviour table ends at the first empty name, and reaching it is the refusal.

// FUNCTION: HYPERX 0x0047f0c0
short PlanVM::FindBehaviorIndexOrThrow(char* pszName)
{
    short nSlot = 0;

    while (strcmp(m_aBehaviorNames[nSlot], pszName) != 0)
    {
        if (m_aBehaviorNames[nSlot][0] == 0)
        {
            g_szCouldNotFindBehaviorNamed[kBehaviorMessageLength] = 0;
            strcat(g_szCouldNotFindBehaviorNamed, pszName);

            throw (char*)g_szCouldNotFindBehaviorNamed;
        }

        nSlot++;
    }

    return nSlot;
}

// FUNCTION: HYPERX 0x0047f1a0
short PlanVM::FindInteractorNameIndexOrThrow(char* pszName)
{
    for (short nSlot = 0; nSlot < kPlanVMInteractorNameLimit; nSlot++)
    {
        if (strcmp(m_aInteractorNames[nSlot], pszName) == 0)
        {
            return nSlot;
        }
    }

    g_szCouldNotFindInteractorNamed[kInteractorMessageLength] = 0;
    strcat(g_szCouldNotFindInteractorNamed, pszName);

    throw (char*)g_szCouldNotFindInteractorNamed;
}

constexpr auto kSymbolNameMax = 40;

#define TOKEN_OPEN  '{'
#define TOKEN_CLOSE '}'

// A name of thirty-three characters or more overruns both buffers, as in the original.
// FUNCTION: HYPERX 0x0047f250
void PlanVM::LoadSymbolTables()
{
    PlanOpcodeRef* const pTable = m_aSymbols;

    short nSlot     = 0;
    short nFileSlot = 0;

    OwnedFile fileEnum;
    OwnedFile fileAnim;
    OwnedFile fileSound;
    OwnedFile fileText;
    char      szToken[kSymbolNameMax];
    char      szPrefixed[kSymbolNameMax];

    // STRING: HYPERX 0x004c704c
    if (fileEnum.m_file.Open("Data\\enumIDs.h", FILE_MODE_READ) != 0)
    {
        m_nLength = 0;

        ReadAll(&fileEnum.m_file);

        fileEnum.m_file.CloseAndTest();
    }

    m_nCursor = 0;

    while (m_pszText[m_nCursor] != TOKEN_OPEN)
    {
        m_nCursor++;
    }

    // Each braced enum numbers its names from zero.
    m_nCursor++;

    while (!AtEnd())
    {
        ReadNextToken(szToken);

        if (szToken[0] != TOKEN_CLOSE)
        {
            const int nValue = nFileSlot;
            nFileSlot++;

            PlanOpcodeRef* const pSymbol = &pTable[nSlot];
            nSlot++;

            strcpy(pSymbol->szName, szToken);

            pSymbol->nValue = nValue;
        }
        else
        {
            nFileSlot = 0;

            while (m_pszText[m_nCursor] != TOKEN_OPEN && !AtEnd())
            {
                m_nCursor++;
            }

            m_nCursor++;
        }
    }

    // STRING: HYPERX 0x004c7030
    if (fileAnim.m_file.Open("Data\\AnimData\\Animassm.txt", FILE_MODE_READ) != 0)
    {
        m_nLength = 0;

        ReadAll(&fileAnim.m_file);

        fileAnim.m_file.CloseAndTest();
    }

    nFileSlot = 0;

    SkipLine();

    while (!AtEnd())
    {
        ReadNextToken(szToken);

        if (static_cast<char>(IsAAPrefix(szToken)) != 0)
        {
            const int nValue = nFileSlot;
            nFileSlot++;

            PlanOpcodeRef* const pSymbol = &pTable[nSlot];
            nSlot++;

            strcpy(pSymbol->szName, szToken);

            pSymbol->nValue = nValue;
        }
        else
        {
            SkipLine();
        }
    }

    // STRING: HYPERX 0x004c701c
    if (fileSound.m_file.Open("Data\\Sndfiles.lst", FILE_MODE_READ) != 0)
    {
        m_nLength = 0;

        ReadAll(&fileSound.m_file);

        fileSound.m_file.CloseAndTest();
    }

    nFileSlot = 0;

    while (!AtEnd())
    {
        if (static_cast<char>(IsCommentOrGroup()) != 0)
        {
            SkipLine();
        }

        ReadNextToken(szToken);

        if (IsNumericChar(szToken))
        {
            SkipLine();
        }
        else
        {
            // STRING: HYPERX 0x004c7014
            strcpy(szPrefixed, "id_SND_");
            strcat(szPrefixed, szToken);

            const int nValue = nFileSlot;
            nFileSlot++;

            PlanOpcodeRef* const pSymbol = &pTable[nSlot];
            nSlot++;

            strcpy(pSymbol->szName, szPrefixed);

            pSymbol->nValue = nValue;
        }
    }

    // STRING: HYPERX 0x004c7000
    if (fileText.m_file.Open("Data\\Infobtxt.lst", FILE_MODE_READ) != 0)
    {
        m_nLength = 0;

        ReadAll(&fileText.m_file);

        fileText.m_file.CloseAndTest();
    }

    nFileSlot = 0;

    while (!AtEnd())
    {
        if (static_cast<char>(IsCommentOrGroup()) == 0)
        {
            ReadNextToken(szToken);

            // STRING: HYPERX 0x004c6ff8
            strcpy(szPrefixed, "id_TXT_");
            strcat(szPrefixed, szToken);

            const int nValue = nFileSlot;
            nFileSlot++;

            PlanOpcodeRef* const pSymbol = &pTable[nSlot];
            nSlot++;

            strcpy(pSymbol->szName, szPrefixed);

            pSymbol->nValue = nValue;
        }

        SkipLine();
    }

    strcpy(pTable[nSlot].szName, g_pszPlanVmEmptyName);

    pTable[nSlot].nValue = static_cast<unsigned int>(-1);
}

// Takes the quotes off a token in place, without a copy.
// FUNCTION: HYPERX 0x0047f730
char* PlanVM::UnquoteString(char* pszText)
{
    pszText[strlen(pszText) - 1] = 0;

    return pszText + 1;
}

// Each counts arguments since its keyword. They start at a hundred, past every test.
// GLOBAL: HYPERX 0x004c6ab8
extern short g_wArgsSinceDeclareBehavior;
short        g_wArgsSinceDeclareBehavior = 100;

// GLOBAL: HYPERX 0x004c6aba
extern short g_wArgsSinceChangeToBehavior;
short        g_wArgsSinceChangeToBehavior = 100;

// GLOBAL: HYPERX 0x004c6abc
extern short g_wArgsSinceChangeOfPlan;
short        g_wArgsSinceChangeOfPlan = 100;

// GLOBAL: HYPERX 0x004c6abe
extern short g_wArgsSinceSetInteractorParam;
short        g_wArgsSinceSetInteractorParam = 100;

// GLOBAL: HYPERX 0x004c6ac0
extern short g_wArgsSinceDeclareLocalReals;
short        g_wArgsSinceDeclareLocalReals = 100;

// Three get the offending token appended, so each is a writable array.
// GLOBAL: HYPERX 0x004c6f24
extern char g_szInabilityToReadString[28];
char        g_szInabilityToReadString[28] = "Inability to read string: ";

// GLOBAL: HYPERX 0x004c6f40
extern char g_szBadNumberInPlan[24];
char        g_szBadNumberInPlan[24] = "Bad number in plan: ";

// GLOBAL: HYPERX 0x004c6f58
extern char g_szExpectedArgEnumeration[40];
char        g_szExpectedArgEnumeration[40] = "expected arg enumeration of localvars.";

// GLOBAL: HYPERX 0x004c6f80
extern char g_szBadPlanIdValue[20];
char        g_szBadPlanIdValue[20] = "Bad plan id value: ";

// Written into: the terminator goes back at 31.
// GLOBAL: HYPERX 0x004c6fb4
extern char g_szCouldNotFindBehaviorNamed[32];
char        g_szCouldNotFindBehaviorNamed[] = "Could not find behavior named: ";

// Written into: the refusal puts the terminator back at 33 and appends the name.
// GLOBAL: HYPERX 0x004c6fd4
extern char g_szCouldNotFindInteractorNamed[34];
char        g_szCouldNotFindInteractorNamed[] = "Could not find interactor named: ";
