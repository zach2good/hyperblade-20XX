#include "common/compat.h"
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mbstring.h>
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

// The reaction table at 0x004cf9fc, defined in src/engine/object.cpp.
extern short g_awReactionByDamageAndSector[35];

// The names the hit list's entries were found under; nothing reads them.
// GLOBAL: HYPERX 0x004cf4f4
extern char g_aAnimListNames[35][32];

#define ANIM_LIST_LINES 5

// Seven names to a line, five lines to a file.
#define ANIM_NAMES_PER_LINE 7

#define ANIM_NAME_MAX 32

#define ANIM_LINE_MAX 100

// GLOBAL: HYPERX 0x004cf4f0
extern int g_nAnimListNamesLead;

// The condition and opcode half of the dispatch array, and the action half: one storage, two bases.
// GLOBAL: HYPERX 0x004cf084
extern PlanOpcodeSlot g_pfnPlanOpcodeHandlers[141];

// GLOBAL: HYPERX 0x004cf2b8
extern PlanHandlerFn g_pfnPlanActionHandlers[140];

#define TOKEN_BUFFER 0x28

#define PLAN_SOURCE_CHARS 80000
#define PLAN_STREAM_WORDS 30000

// The five counters and a number token's scratch; marked in src/globals.cpp.
extern short g_wArgsSinceDeclareBehavior;

extern short g_wArgsSinceChangeToBehavior;

extern short g_wArgsSinceChangeOfPlan;

extern short g_wArgsSinceSetInteractorParam;

extern short g_wArgsSinceDeclareLocalReals;

#define PLAN_OPCODE_NONE 0xffffffff

// How far into the token and into each message the appended text is cut off.
#define TOKEN_MESSAGE_LENGTH 20

extern char g_szBadPlanIdValue[20];

#define BAD_PLAN_ID_MESSAGE_LENGTH 19

extern PlanWord g_planParsedLocalReal;

extern char g_szExpectedArgEnumeration[40];

extern char g_szBadNumberInPlan[24];

#define BAD_NUMBER_MESSAGE_LENGTH 20

// The four messages, three of which the offending token is appended to.
extern char g_szInabilityToReadString[28];

#define UNREADABLE_STRING_MESSAGE_LENGTH 26

#define PLAN_STRING_LIMIT 0x28

// Five action slots are never written, and only the hit list's names are kept.
// FUNCTION: HYPERX 0x0046ac10
void PlanVM::LoadAnimationLists(void)
{
    char aHitName[ANIM_NAMES_PER_LINE][ANIM_NAME_MAX];
    char aChkName[ANIM_NAMES_PER_LINE][ANIM_NAME_MAX];
    char aThrName[ANIM_NAMES_PER_LINE][ANIM_NAME_MAX];
    char aJmpName[ANIM_NAMES_PER_LINE][ANIM_NAME_MAX];
    char szHitLine[ANIM_LINE_MAX];
    char szThrLine[ANIM_LINE_MAX];
    char szJmpLine[ANIM_LINE_MAX];
    char szChkLine[ANIM_LINE_MAX];

    g_pCollInteractorPool = new CollInteractorPool();
    g_pMsgInteractorPool  = new MsgInteractorPool();

    g_nAnimListNamesLead                                                                      = 0;
    g_pfnPlanOpcodeHandlers[SET_INTERACTOR_NAME].pfnSetParam                                  = PlanCond::SetAnonLocal;
    g_pfnPlanOpcodeHandlers[SET_ACTIVE_STATE].pfnSetParam                                     = PlanCond::SetActiveState;
    g_pfnPlanActionHandlers[ACF_SEND_MESSAGE]                                                 = PlanAction::SendMsg;
    g_pfnPlanActionHandlers[ACF_REALLY_SEND_MSG]                                              = PlanAction::ReallySendMsg;
    g_pfnPlanActionHandlers[ACF_SET_ATTACH_PAK]                                               = PlanAction::Set_AttachPak;
    g_pfnPlanActionHandlers[ACF_UNSET_ATTACH_PAK]                                             = PlanAction::Unset_AttachPak;
    g_pfnPlanActionHandlers[ACF_RECOIL_OFF_WALL]                                              = PlanAction::RecoilOffWall;
    g_pfnPlanActionHandlers[ACF_REFLECT_OFF_WALL]                                             = PlanAction::ReflectOffWall;
    g_pfnPlanActionHandlers[ACF_REFLECT_OFF_MODEL]                                            = PlanAction::ReflectOffModel;
    g_pfnPlanActionHandlers[ACF_CLOTHES_LINE_RECOIL]                                          = PlanAction::ClothesLineRecoil;
    g_pfnPlanActionHandlers[ACF_MAN_ON_MAN_RECOIL]                                            = PlanAction::ManOnManRecoil;
    g_pfnPlanActionHandlers[ACF_BOUNCE_OFF_DROMER]                                            = PlanAction::BounceOffDromer;
    g_pfnPlanActionHandlers[ACF_LAUNCH]                                                       = PlanAction::Launch;
    g_pfnPlanActionHandlers[ACF_LAUNCH_TOWARD]                                                = PlanAction::LaunchToward;
    g_pfnPlanActionHandlers[ACF_LAUNCH_AT]                                                    = PlanAction::LaunchAt;
    g_pfnPlanActionHandlers[ACF_THROW]                                                        = PlanAction::Throw;
    g_pfnPlanActionHandlers[ACF_EXPLODE_FLAT]                                                 = PlanAction::ExplodeFlat;
    g_pfnPlanActionHandlers[ACF_EXPLODE_AGAINST_VELOCITY]                                     = PlanAction::ExplodeAgainstVelocity;
    g_pfnPlanActionHandlers[ACF_EXPLODE_WITH_VELOCITY]                                        = PlanAction::ExplodeWithVelocity;
    g_pfnPlanActionHandlers[ACF_EXPLODE_UNIFORM]                                              = PlanAction::ExplodeUniform;
    g_pfnPlanActionHandlers[ACF_LOSE_HEAD]                                                    = PlanAction::LoseHead;
    g_pfnPlanActionHandlers[ACF_REGAIN_HEAD]                                                  = PlanAction::RegainHead;
    g_pfnPlanActionHandlers[ACF_BODY_SPLIT]                                                   = PlanAction::BodySplit;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_BODY_PART_ANIMATION].pfnDecide           = PlanCond::GetTeamParity;
    g_pfnPlanActionHandlers[ACF_SET_SPEED_BURST]                                              = PlanAction::Set_SpeedBurst;
    g_pfnPlanActionHandlers[ACF_ENGAGE_OPPONENT]                                              = PlanAction::Engage_Opponent;
    g_pfnPlanActionHandlers[ACF_PLAY_SOUND]                                                   = AnimEvent::PlaySnd;
    g_pfnPlanActionHandlers[ACF_PLAY_SND_TYPE]                                                = AnimEvent::PlaySndType;
    g_pfnPlanActionHandlers[ACF_PAN_SND_TYPE]                                                 = AnimEvent::PanSndType;
    g_pfnPlanActionHandlers[ACF_DOPPLER_SOUND]                                                = AnimEvent::DopplerSound;
    g_pfnPlanActionHandlers[ACF_PLACE_SOUND]                                                  = AnimEvent::PlaySoundByDistance;
    g_pfnPlanActionHandlers[ACF_NOTE_STATS4_POSSESSORS]                                       = PlanAction::NoteStats4Possessors;
    g_pfnPlanActionHandlers[ACF_INCREMENT_STAT]                                               = PlanAction::IncrementStat;
    g_pfnPlanActionHandlers[ACF_START_STAT_TIMER]                                             = PlanAction::StartStatTimer;
    g_pfnPlanActionHandlers[ACF_END_STAT_TIMER]                                               = PlanAction::EndStatTimer;
    g_pfnPlanActionHandlers[ACF_SET_LOCATION]                                                 = PlanAction::SetLocation;
    g_pfnPlanActionHandlers[ACF_SET_VELOCITY]                                                 = PlanAction::SetVelocity;
    g_pfnPlanActionHandlers[ACF_SET_OBJECT_TO_TRACK]                                          = PlanAction::SetObjectToTrack;
    g_pfnPlanActionHandlers[ACF_SET_TRACK_TO_VIEW_OWNER]                                      = PlanAction::SetTrackToViewOwner;
    g_pfnPlanActionHandlers[ACF_SET_TRACKING_OFFSET]                                          = PlanAction::SetTrackingOffset;
    g_pfnPlanActionHandlers[ACF_INCREMENT_TRACKING_OFFSET]                                    = PlanAction::IncrementTrackingOffset;
    g_pfnPlanActionHandlers[ACF_INCREMENT_CAMERA_DIR]                                         = PlanAction::IncrementCameraDir;
    g_pfnPlanActionHandlers[ACF_INCREMENT_BUNGEE_LENGTH]                                      = PlanAction::IncrementBungeeLength;
    g_pfnPlanActionHandlers[ACF_SET_BUNGEE_LENGTH]                                            = PlanAction::SetBungeeLength;
    g_pfnPlanActionHandlers[ACF_SET_TRACK_HEIGHT]                                             = PlanAction::SetTrackHeight;
    g_pfnPlanActionHandlers[ACF_SET_MIN_HEIGHT]                                               = PlanAction::SetMinHeight;
    g_pfnPlanActionHandlers[ACF_SET_LOOK_HEIGHT]                                              = PlanAction::SetLookHeight;
    g_pfnPlanActionHandlers[ACF_SET_CAM_BUFFER_FACTOR]                                        = PlanAction::SetCamBufferFactor;
    g_pfnPlanActionHandlers[ACF_SET_SCAN_MODES]                                               = PlanAction::SetScanModes;
    g_pfnPlanActionHandlers[ACF_SET_SOLAR_WIND_FACTOR]                                        = PlanAction::SetSolarWindFactor;
    g_pfnPlanActionHandlers[ACF_SET_OFFSET_VALUES]                                            = PlanAction::SetOffsetValues;
    g_pfnPlanActionHandlers[ACF_SET_OBJECT_TO_WATCH]                                          = PlanAction::SetObjectToWatch;
    g_pfnPlanActionHandlers[ACF_INCREMENT_OBJECT_TO_WATCH]                                    = PlanAction::IncrementObjectToWatch;
    g_pfnPlanActionHandlers[ACF_INCREMENT_OBJECT_TO_TRACK]                                    = PlanAction::IncrementObjectToTrack;
    g_pfnPlanActionHandlers[ACF_RESET_CAMERA_BUFFER_SIZE]                                     = PlanAction::ResetCameraBufferSize;
    g_pfnPlanActionHandlers[ACF_RESET_RETICLE]                                                = PlanAction::ResetReticle;
    g_pfnPlanActionHandlers[ACF_SET_HEAD_CAM_TEXT]                                            = PlanAction::SetHeadCamText;
    g_pfnPlanActionHandlers[ACF_INIT_REAR_VIEW_POS]                                           = PlanAction::InitRearViewPos;
    g_pfnPlanActionHandlers[ACF_SET_CAM_OFFSET_TO_LOCATION]                                   = PlanAction::SetCamOffsetToLocation;
    g_pfnPlanActionHandlers[ACF_RELAX_YAW_TO_FACE]                                            = PlanAction::RelaxYawToFace;
    g_pfnPlanActionHandlers[ACF_SET_LOCAL_REAL]                                               = PlanAction::SetLocalReal;
    g_pfnPlanActionHandlers[ACF_SET_RENDER_FLAG]                                              = PlanAction::SetRenderFlag;
    g_pfnPlanActionHandlers[ACF_INC_ROK_VALUE]                                                = PlanAction::IncRokValue;
    g_pfnPlanActionHandlers[ACF_SET_ROK_KILL_STATE]                                           = PlanAction::SetRokKillState;
    g_pfnPlanActionHandlers[ACF_SET_MY_GRAVITY]                                               = PlanAction::SetMyGravity;
    g_pfnPlanActionHandlers[ACF_MODIFY_DAMAGE]                                                = PlanAction::ModifyDamage;
    g_pfnPlanActionHandlers[ACF_MODIFY_PITCH]                                                 = PlanAction::ModifyPitch;
    g_pfnPlanActionHandlers[ACF_HOVER_ABOVE_ARMORY]                                           = PlanAction::HoverAboveArmory;
    g_pfnPlanActionHandlers[ACF_DEPLOY_WEAPON]                                                = PlanAction::DeployWeapon;
    g_pfnPlanActionHandlers[ACF_SET_IN_DRAG]                                                  = PlanAction::SetInDrag;
    g_pfnPlanActionHandlers[ACF_SET_ON_RIM]                                                   = PlanAction::SetOnRim;
    g_pfnPlanActionHandlers[ACF_SET_FRICTION]                                                 = PlanAction::SetFriction;
    g_pfnPlanActionHandlers[ACF_MODIFY_RATING]                                                = PlanAction::ModifyRating;
    g_pfnPlanActionHandlers[ACF_MODIFY_AVAILABLE_REDS]                                        = PlanAction::ModifyAvailableReds;
    g_pfnPlanActionHandlers[ACF_SET_ANIMATION]                                                = PlanAction::SetAnimation;
    g_pfnPlanActionHandlers[ACF_RESTORE_PERSISTENT_ANIMATION]                                 = PlanAction::RestorePersistentAnimation;
    g_pfnPlanActionHandlers[ACF_SET_ANIMATION_EVENT]                                          = PlanAction::SetAnimationEvent;
    g_pfnPlanActionHandlers[ACF_SET_ANIMATION_STATE]                                          = PlanAction::SetAnimationState;
    g_pfnPlanActionHandlers[ACF_SET_ANIMATION_SPEED]                                          = PlanAction::SetAnimationSpeed;
    g_pfnPlanActionHandlers[ACF_INCR_ANIMATION_SPEED]                                         = PlanAction::IncrAnimationSpeed;
    g_pfnPlanActionHandlers[ACF_SET_ANIMATION_DONE_MESSAGE]                                   = PlanAction::SetAnimationDoneMessage;
    g_pfnPlanActionHandlers[ACF_ANIMATION_PEEK]                                               = GameApp::AnimationPeek;
    g_pfnPlanActionHandlers[ACF_HOOK_DRAG_BUNGEES_ON]                                         = PlanAction::HookDragBungeesOn;
    g_pfnPlanActionHandlers[ACF_RESET_ACC_TRANSIENT]                                          = PlanAction::ResetAccTransient;
    g_pfnPlanActionHandlers[ACF_THROW_BUNGEE]                                                 = PlanAction::ThrowBungee;
    g_pfnPlanActionHandlers[ACF_RESET_KEYSTATE_FLAGS]                                         = PlanAction::ResetKeystateFlags;
    g_pfnPlanActionHandlers[ACF_LAUNCH_GUN_DIRECTION]                                         = PlanAction::LaunchGunDirection;
    g_pfnPlanActionHandlers[ACF_SET_PALETTE_EFFECT]                                           = PlanAction::SetPaletteEffect;
    g_pfnPlanActionHandlers[ACF_SIGNAL_KEYSTATE]                                              = PlanAction::SignalKeystate;
    g_pfnPlanActionHandlers[ACF_GET_KEYSTATE_FLAGS]                                           = PlanAction::GetKeystateFlags;
    g_pfnPlanActionHandlers[ACF_SCALE_BUB_TO_RATING]                                          = PlanAction::ScaleBubToRating;
    g_pfnPlanActionHandlers[ACF_SET_TARGET_WITH_DATA]                                         = PlanAction::SetTargetWithData;
    g_pfnPlanActionHandlers[ACF_BUNGEE_TOWARD_TARGET]                                         = PlanAction::BungeeTowardTarget;
    g_pfnPlanActionHandlers[ACF_GLIDE_ON_RAMP]                                                = PlanAction::GlideOnRamp;
    g_pfnPlanActionHandlers[ACF_GRAYSCALE_RANDOMIZER]                                         = PlanAction::GrayscaleRandomizer;
    g_pfnPlanActionHandlers[ACF_SET_IMMUNITY]                                                 = PlanAction::SetImmunity;
    g_pfnPlanActionHandlers[ACF_SPEED_BURST_SITUATION]                                        = PlanAction::SpeedBurstSituation;
    g_pfnPlanActionHandlers[ACF_INCR_ANIM_SPEED_W_RATING]                                     = PlanAction::IncrAnimSpeedWRating;
    g_pfnPlanActionHandlers[ACF_MODIFY_OPPOS_TEAM_FRICTION]                                   = PlanAction::ModifyOpposTeamFriction;
    g_pfnPlanActionHandlers[ACF_SET_DEVICE_OWNER]                                             = PlanAction::SetDeviceOwner;
    g_pfnPlanActionHandlers[ACF_SET_INPUT_MODE]                                               = PlanAction::SetInputMode;
    g_pfnPlanActionHandlers[ACF_INCREMENT_SCORE]                                              = PlanAction::IncrementScore;
    g_pfnPlanActionHandlers[ACF_SWAP_RANDOMIZERS]                                             = PlanAction::SwapRandomizers;
    g_pfnPlanActionHandlers[ACF_SET_SHOT_POWER_METER]                                         = PlanAction::SetShotPowerMeter;
    g_pfnPlanActionHandlers[ACF_SET_ATTACKEE]                                                 = PlanAction::SetAttackee;
    g_pfnPlanActionHandlers[ACF_MODIFY_SPEED_RATING]                                          = PlanAction::ModifySpeedRating;
    g_pfnPlanActionHandlers[ACF_SET_KNOCKED_DOWN_STATE]                                       = PlanAction::SetKnockedDownState;
    g_pfnPlanActionHandlers[ACF_SPIN_TURNSTILE]                                               = PlanAction::SpinTurnstile;
    g_pfnPlanActionHandlers[ACF_TRACE]                                                        = PlanAction::Trace;
    g_pfnPlanActionHandlers[ACF_SET_KILLED_DROMER]                                            = PlanAction::SetKilledDromer;
    g_pfnPlanActionHandlers[ACF_ANIMATION_SEMAPHORE]                                          = PlanAction::AnimationSemaphore;
    g_pfnPlanActionHandlers[ACF_RESET_CAMERA_TRACK]                                           = PlanAction::ResetCameraTrack;
    g_pfnPlanActionHandlers[ACF_SET_INFO_BAR_TEXT]                                            = PlanAction::SetInfoBarText;
    g_pfnPlanActionHandlers[ACF_CLOCK_STOPPED_SEMAPHORE]                                      = PlanAction::ClockStoppedSemaphore;
    g_pfnPlanActionHandlers[ACF_CREDIT_DAMAGE]                                                = PlanAction::CreditDamage;
    g_pfnPlanActionHandlers[ACF_TRACK_TURNSTILE]                                              = PlanAction::TrackTurnstile;
    g_pfnPlanActionHandlers[ACF_DESIRED_SUBSTITUTE]                                           = PlanAction::DesiredSubstitute;
    g_pfnPlanActionHandlers[ACF_SUBSTITUTE_DROMER]                                            = PlanAction::SubstitutePlayer;
    g_pfnPlanActionHandlers[ACF_SET_INFO_TEXT_DROMER]                                         = PlanAction::SetInfoTextDromer;
    g_pfnPlanActionHandlers[ACF_SET_INFO_TEXT_TEAM]                                           = PlanAction::SetInfoTextTeam;
    g_pfnPlanActionHandlers[ACF_CLEAR_BEHAVIOR_ANIMATION]                                     = PlanAction::ClearBehaviorAnimation;
    g_pfnPlanActionHandlers[ACF_IF_NOT_ROK_TRACK_ME]                                          = PlanAction::IfNotRokTrackMe;
    g_pfnPlanActionHandlers[ACF_SEND_MESSAGE_TO_MY_CAMERA]                                    = PlanAction::SendMessageToMyCamera;
    g_pfnPlanActionHandlers[ACF_TRANSFER_VIEW_OWNERSHIP]                                      = PlanAction::TransferViewOwnership;
    g_pfnPlanActionHandlers[ACF_SMALL_BLOOD_SPURT]                                            = PlanAction::SmallBloodSpurt;
    g_pfnPlanActionHandlers[ACF_LARGE_BLOOD_SPURT]                                            = PlanAction::LargeBloodSpurt;
    g_pfnPlanActionHandlers[ACF_SET_MAX_SPEED]                                                = PlanAction::SetMaxSpeed;
    g_pfnPlanActionHandlers[ACF_END_PERIOD_NOW]                                               = PlanAction::EndPeriodNow;
    g_pfnPlanActionHandlers[ACF_KILL_BALL_STAGING]                                            = PlanAction::KillBallStaging;
    g_pfnPlanActionHandlers[ACF_KILL_BALL_EXECUTE]                                            = PlanAction::KillBallExecute;
    g_pfnPlanActionHandlers[ACF_KICK_OFF_RIM]                                                 = PlanAction::KickOffRim;
    g_pfnPlanActionHandlers[ACF_FLYING_SEMAPHORE]                                             = PlanAction::FlyingSemaphore;
    g_pfnPlanActionHandlers[ACF_KICK_DOWN_TO_SURFACE]                                         = PlanAction::KickDownToSurface;
    g_pfnPlanActionHandlers[ACF_SYNC_POSSESSION]                                              = PlanAction::SyncPossession;
    g_pfnPlanActionHandlers[ACF_SYNC_SYSTEM]                                                  = PlanAction::SyncSystem;
    g_pfnPlanActionHandlers[ACF_SYNC_DAMAGE]                                                  = PlanAction::SyncDamage;
    g_pfnPlanActionHandlers[ACF_SEND_LAUNCH_MESSAGE]                                          = PlanAction::SendLaunchMessage;
    g_pfnPlanActionHandlers[ACF_SET_LASER_HURDLE_CHARGE]                                      = PlanAction::SetLaserHurdleCharge;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_LASER_HURDLE_COLLISION].pfnDecide = CollisionCheck::DromerLaserHurdleCollision;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_GET_LASER_HURDLE_CHARGE].pfnDecide       = Util::GetLaserHurdleCharge;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_ATTACHMENT].pfnDecide             = PlanCond::Decide_Attachment;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_CELEBRATION].pfnDecide            = PlanCond::Decide_Celebration;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_PASS_OR_RUN].pfnDecide            = PlanCond::Decide_PassOrRun;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_HP_SPIN_RAIL_OR_NOT].pfnDecide    = PlanCond::Decide_HPSpin_Rail_orNot;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_PICK_A_CELL].pfnDecide                   = PlanCond::PickACell;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_RANDOM].pfnDecide                        = PlanCond::Random;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SHOT_POWER].pfnDecide                    = PlanCond::ShotPower;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IF_IN_FACE_OFF].pfnDecide                = PlanCond::IfInFaceOff;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IS_SENDER_TEAMMATE].pfnDecide            = PlanCond::IsSenderTeammate;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_AM_MAIN_DROMER].pfnDecide                = PlanCond::AmMainDromer;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MY_ID].pfnDecide                         = PlanCond::MyID;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MESSAGE_DATA].pfnDecide                  = PlanCond::SetResultDirect;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MESSAGE_DATA_NIBL0].pfnDecide            = PlanCond::MessageDataNibl0;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MESSAGE_DATA_NIBL1].pfnDecide            = PlanCond::MessageDataNibl1;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MESSAGE_DATA_NIBL4].pfnDecide            = PlanCond::MessageDataNibl4;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MSG_DATA4_NIBL1_X2_NIBL0].pfnDecide      = PlanCond::MsgData4Nibl1X2Nibl0;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SIDE_KEYS_ARE_DOWN].pfnDecide            = PlanCond::SideKeysAreDown;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_AM_VIEW_OWNER].pfnDecide                 = PlanCond::AmViewOwner;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TEAMMATE_IS_VIEW_OWNER].pfnDecide        = PlanCond::TeammateIsViewOwner;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OWNER_IS_VIEW_OWNER].pfnDecide           = PlanCond::OwnerIsViewOwner;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_VIEW_OWNERS_MATE_IN_FRONT].pfnDecide     = PlanCond::ViewOwnersMateInFront;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_I_HAVE_ROK].pfnDecide                    = PlanCond::IHaveRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_HAS_ROK].pfnDecide                = PlanCond::DromerHasRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_RUN_TIME_DATA].pfnDecide                 = PlanCond::SetResultDirect;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IS_ROK_IN_MY_ZONE].pfnDecide             = PlanCond::IsRokInMyZone;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_POSITION].pfnDecide               = PlanCond::DecidePosition;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_COLLISION_DETAIL].pfnDecide              = PlanCond::CollisionDetail;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ENGAGE_WHICH_OPPONENT].pfnDecide         = PlanCond::EngageWhichOpponent;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TEAMMATE_HAS_ROK].pfnDecide              = PlanCond::TeammateHasRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_GAG_THE_ANNOUNCER].pfnDecide             = PlanCond::GagTheAnnouncer;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_VICTIM_CAM_IS_ON].pfnDecide              = PlanCond::VictimCamIsOn;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IS_SCANNING_MODE_ON].pfnDecide           = PlanCond::IsScanningModeOn;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_BLOCK_OR_GO_OUT].pfnDecide        = PlanCond::Decide_BlockOrGoOut;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_WHICH_OPPN_IS_CLOSEST_TO_ROK].pfnDecide  = PlanCond::WhichOppnIsClosestToRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CLOSEST_UNCOVERED_OPPN].pfnDecide        = PlanCond::ClosestUncoveredOppn;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ANALYZE_PAD_A].pfnDecide                 = PlanCond::AnalyzePadA;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ANALYZE_PAD_B].pfnDecide                 = PlanCond::AnalyzePadB;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ANALYZE_PAD_C].pfnDecide                 = PlanCond::AnalyzePadC;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ATTACKER_IS_IN_REAR].pfnDecide           = PlanCond::AttackerIsInRear;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SPEED_BURST_IS_RELAXED].pfnDecide        = PlanCond::SpeedBurstIsRelaxed;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OFFENSE_OR_W_ROK].pfnDecide              = PlanCond::OffenseOrWRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_HOST_IS_VIEW_OWNER].pfnDecide            = PlanCond::HostIsViewOwner;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ROK_IN_WHICH_TEAMS_POSS].pfnDecide       = PlanCond::RokInWhichTeamsPoss;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IS_DOUBLE_SCORE].pfnDecide               = PlanCond::IsDoubleScore;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_BUNGEE_VICTIM_LOR_R].pfnDecide           = PlanCond::BungeeVictimLorR;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SPIN_UP].pfnDecide                       = PlanCond::SpinUp;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_AM_I_KNOCKED_LOOSE].pfnDecide            = PlanCond::AmIKnockedLoose;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ROK_SCORE_VALUE].pfnDecide               = PlanCond::RokScoreValue;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CATCH_BLOCK_OR_MISS].pfnDecide           = PlanCond::CatchBlockOrMiss;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_JUMPING].pfnDecide                       = PlanCond::Jumping;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DUCKING].pfnDecide                       = PlanCond::Ducking;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_HURDLE_COLLISION].pfnDecide       = CollisionCheck::DromerHurdleCollision;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_MULTIPLIER_COLLISION].pfnDecide   = CollisionCheck::DromerMultiplierCollision;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_LIMBO_COLLISION].pfnDecide        = CollisionCheck::DromerLimboCollision;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_RANDOMIZER_COLLISION].pfnDecide   = CollisionCheck::DromerRandomizerCollision;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_RAMP_COLLISION].pfnDecide         = CollisionCheck::DromerRampCollision;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ROK_HURDLE_COLLISION].pfnDecide          = CollisionCheck::RokHurdleCollision;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CHOOSE_PASSING_PLAY].pfnDecide           = PlanCond::ChoosePassingPlay;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_RECVR_IN_RANGE_N_VIEW].pfnDecide         = PlanCond::RecvrInRangeNView;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_WHICH_OPPN_IS_CLOSEST_TO_ME].pfnDecide   = PlanCond::WhichOppnIsClosestToMe;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TAKE_DAMAGE].pfnDecide                   = PlanCond::TakeDamage;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IS_AMBIENT].pfnDecide                    = AnimEvent::IsAmbient;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ROK_IS_CHARGED].pfnDecide                = PlanCond::RokIsCharged;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ROK_IS_CHARGED_AGAINST].pfnDecide        = PlanCond::RokIsChargedAgainst;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CLOSEST_TARGET_TO_PROJECTILE].pfnDecide  = PlanCond::ClosestTargetToProjectile;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ME_OR_TEAMMATE_CLOSEST_TO_ROK].pfnDecide = PlanCond::MeOrTeammateClosestToRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ROK_VALUE_IN_MY_FAVOR].pfnDecide         = PlanCond::RokValueInMyFavor;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_RED_IS_AVAILABLE].pfnDecide              = PlanCond::RedIsAvailable;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_RATING_MAXED_OUT].pfnDecide              = PlanCond::RatingMaxedOut;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_PASS_OR_RUN_BY_RATINGS].pfnDecide        = PlanCond::PassOrRunByRatings;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_LOOSE_ROK_WHAT_TO_DO].pfnDecide          = PlanCond::LooseRokWhatToDo;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_GO_THROUGH_MULIPLIER].pfnDecide          = PlanCond::GoThroughMuliplier;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_PICK_ATTACK_MOVE].pfnDecide              = PlanCond::PickAttackMove;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OFF_WING_JOB].pfnDecide                  = PlanCond::OffWingJob;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_PICK_FLYING_MOVE].pfnDecide              = PlanCond::PickFlyingMove;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_PICK_HALFPIPE_MOVE].pfnDecide            = PlanCond::PickHalfpipeMove;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_AVOID_SPINNING_TURNSTILE].pfnDecide      = PlanCond::AvoidSpinningTurnstile;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TARGET_TURNSTILE].pfnDecide              = PlanCond::DecideNever;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_USE_KILLBALL].pfnDecide                  = PlanCond::DecideNever;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_USE_RANDOMIZER].pfnDecide                = PlanCond::DecideNever;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SHOOT_SITUATION].pfnDecide               = PlanCond::ShootSituation;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_REGROUP_SITUATION].pfnDecide             = PlanCond::RegroupSituation;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_AGGRESSION_ON_OTHERS].pfnDecide          = PlanCond::AggressionOnOthers;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TEN_OR_MORE_PT_SPREAD].pfnDecide         = PlanCond::TenOrMorePtSpread;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MY_TEAM_LOSING_BY_AT_LEAST].pfnDecide    = PlanCond::MyTeamLosingByAtLeast;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DROMER_NEAR_MINE].pfnDecide              = PlanCond::DromerNearMine;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ATTACK_OR_DEPLOY_WEAPON].pfnDecide       = PlanCond::AttackOrDeployWeapon;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_WITH_RATING].pfnDecide            = PlanCond::DecideWithRating;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_NEAR_OBJECT_BY].pfnDecide                = PlanCond::NearObjectBy;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CHOOSE_TEAM_TUNE].pfnDecide              = PlanCond::ChooseTeamTune;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SOUND_BANK].pfnDecide                    = PlanCond::SoundBank;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_HOME_OR_AWAY_TEAM].pfnDecide             = PlanCond::GetTeamParity;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_WINNING_OR_LOSING].pfnDecide             = PlanCond::WinningOrLosing;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IS_DROMER_KNOCKED_DOWN].pfnDecide        = PlanCond::IsDromerKnockedDown;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_RANDOM_EMPTY_ARMORY].pfnDecide           = PlanCond::RandomEmptyArmory;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_RANDOM_INACTIVE_WEAPON].pfnDecide        = PlanCond::RandomInactiveWeapon;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_WHO_KILLED_ME].pfnDecide                 = PlanCond::WhoKilledMe;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OK_TO_SUBSTITUTE_DROMER].pfnDecide       = PlanAction::SubstitutePlayer;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ANIMATION_SEMAPHORE].pfnDecide           = PlanAction::AnimationSemaphore;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CLOCK_STOPPED_SEMAPHORE].pfnDecide       = PlanAction::ClockStoppedSemaphore;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ATTACHED_TO_SENDER].pfnDecide            = PlanCond::AttachedToSender;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ATTACKED_ANIM].pfnDecide                 = PlanCond::AttackedAnim;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CLOSEST_DROMER_TO_ROK].pfnDecide         = PlanCond::ClosestDromerToRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_ROK_IN_RANGE].pfnDecide                  = PlanCond::RokInRange;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IS_DROMER_INJURED].pfnDecide             = PlanCond::IsDromerInjured;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TEAMMATE_IS_HUMAN_OR_INJURED].pfnDecide  = PlanCond::TeammateIsHumanOrInjured;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DESIRED_SUBSTITUTE].pfnDecide            = PlanAction::DesiredSubstitute;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SUBSTITUTION_SEMAPHORE].pfnDecide        = PlanCond::SubstitutionSemaphore;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OK_TO_SWITCH_TO_TEAMMATE].pfnDecide      = PlanCond::OkToSwitchToTeammate;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OK_TO_PASS_TO_TEAMMATE].pfnDecide        = PlanCond::OkToPassToTeammate;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OK_TO_SUBSTITUTE_TEAMMATE].pfnDecide     = PlanCond::OkToSubstituteTeammate;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_PICK_VALID_SUBSTITUTE].pfnDecide         = PlanCond::PickValidSubstitute;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OK_TO_GET_ROK].pfnDecide                 = PlanCond::OkToGetRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_OK_TO_LAUNCH_ROK].pfnDecide              = PlanCond::OkToLaunchRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_GET_IMMUNITY].pfnDecide                  = PlanCond::GetImmunity;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_INTERCEPTION].pfnDecide                  = PlanCond::Interception;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_SPECIAL_ATTACKS].pfnDecide               = PlanCond::SpecialAttacks;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_IN_RANGE_TO_ATTACK].pfnDecide            = PlanCond::InRangeToAttack;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_JOSTLED_DROP_ROK].pfnDecide              = PlanCond::JostledDropRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_BOUNCED_IN_GOAL].pfnDecide               = CollisionCheck::BouncedInGoal;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CHECK_PASSING_DISTANCE].pfnDecide        = PlanCond::CheckPassingDistance;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_WHICH_OPPN_TO_ATTACK].pfnDecide          = PlanCond::WhichOppnToAttack;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CHECK_RENDER_FLAG].pfnDecide             = PlanCond::CheckRenderFlag;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TEAM_IS_DEAD].pfnDecide                  = PlanCond::TeamIsDead;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TEAM_IS_INJURED].pfnDecide               = PlanCond::TeamIsInjured;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_MY_TEAM_IS_INJURED].pfnDecide            = PlanCond::MyTeamIsInjured;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_WHO_WON_THE_GAME].pfnDecide              = PlanCond::WhoWonTheGame;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_WHO_HAS_ROK].pfnDecide                   = PlanCond::WhoHasRok;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TRY_FOR_KILLBALL].pfnDecide              = PlanCond::TryForKillball;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_TARGET_ACQUIRED].pfnDecide               = PlanCond::TargetAcquired;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_DECIDE_GOALIE_LAUNCH].pfnDecide          = PlanCond::DecideGoalieLaunch;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_NEAREST_SUSPENSE_MULT].pfnDecide         = PlanCond::NearestSuspenseMult;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_BALL_GOT_BEHIND_ME].pfnDecide            = PlanCond::BallGotBehindMe;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_GOALIE_PASSING_TARGET].pfnDecide         = PlanCond::GoaliePassingTarget;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_FORCE_REAR_VIEW].pfnDecide               = PlanCond::ForceRearView;
    g_pfnPlanOpcodeHandlers[PLAN_DECISION_BASE + DCF_CHECK_SCORE_ON_OWN_GOAL].pfnDecide       = PlanCond::CheckScoreOnOwnGoal;

    // Marked in src/graphics/colorpalette.cpp: without /GF each object keeps its own copy.
    // STRING: HYPERX 0x004c4704
    FILE* const pHitFile = fopen(String::Concat(g_gameApp.m_strExeDir.m_str, "HitAnims.lst"), "r");
    // STRING: HYPERX 0x004c46f4
    FILE* const pChkFile = fopen(String::Concat(g_gameApp.m_strExeDir.m_str, "ChkAnims.lst"), "r");
    // STRING: HYPERX 0x004c46e4
    FILE* const pJmpFile = fopen(String::Concat(g_gameApp.m_strExeDir.m_str, "JmpAnims.lst"), "r");
    // STRING: HYPERX 0x004c46d4
    FILE* const pThrFile = fopen(String::Concat(g_gameApp.m_strExeDir.m_str, "ThrAnims.lst"), "r");

    if (pHitFile == 0 || pChkFile == 0 || pJmpFile == 0 || pThrFile == 0)
    {
        // STRING: HYPERX 0x004c46ac
        g_gameApp.Fatal("Error opening Animation List files.\n");
    }

    short* pwIndex               = g_awReactionByDamageAndSector;
    char (*pName)[ANIM_NAME_MAX] = g_aAnimListNames;
    int nLine                    = ANIM_LIST_LINES;

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

        if (nHitRead != ANIM_NAMES_PER_LINE || nChkRead != nHitRead || nJmpRead != nHitRead || nThrRead != nHitRead)
        {
            // STRING: HYPERX 0x004c4670
            g_gameApp.Fatal("Error reading Animation List file.\n");
        }

        char (*pSource)[ANIM_NAME_MAX] = aHitName;
        char (*pDest)[ANIM_NAME_MAX]   = pName;
        short* pwWork                  = pwIndex;
        int    nName                   = ANIM_NAMES_PER_LINE;

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
void __cdecl PlanVM::InitRuntimeOpcodeTable(void)
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
#define PLAN_NAMED_TOKENS        6
#define PLAN_LAST_NUMBERED_TOKEN 50

// Builds the two keyword tables; each ends in an empty-name entry valued minus one.
// FUNCTION: HYPERX 0x0047d340
PlanVM::PlanVM()
{
    char szToken[8];

    m_pszText = 0;
    m_pStream = 0;

    PlanOpcodeRef* pEntry = &m_aOpcodes[0];

    // STRING: HYPERX 0x004c6d0c
    strcpy(pEntry->szName, "SET_COLLISION_INTERACTOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_COLLISION_INTERACTOR | MEMBER_SET_COLLISION_INTERACTOR;

    pEntry = &m_aOpcodes[1];

    // STRING: HYPERX 0x004c6cf4
    strcpy(pEntry->szName, "SET_MESSAGE_INTERACTOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_MESSAGE_INTERACTOR | MEMBER_SET_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[2];

    // STRING: HYPERX 0x004c6cd4
    strcpy(pEntry->szName, "SET_LOC_COLLISION_INTERACTOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_COLLISION_INTERACTOR | MEMBER_SET_LOC_COLLISION_INTERACTOR;

    pEntry = &m_aOpcodes[3];

    // STRING: HYPERX 0x004c6cb4
    strcpy(pEntry->szName, "SET_NET_COLLISION_INTERACTOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_MESSAGE_INTERACTOR | MEMBER_SET_NET_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[4];

    // STRING: HYPERX 0x004c6c98
    strcpy(pEntry->szName, "SET_LOC_MESSAGE_INTERACTOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_MESSAGE_INTERACTOR | MEMBER_SET_LOC_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[5];

    // STRING: HYPERX 0x004c6c7c
    strcpy(pEntry->szName, "SET_NET_MESSAGE_INTERACTOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_MESSAGE_INTERACTOR | MEMBER_SET_NET_MESSAGE_INTERACTOR;

    pEntry = &m_aOpcodes[6];

    // STRING: HYPERX 0x004c6c6c
    strcpy(pEntry->szName, "ADD_AGENDA_ITEM");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_AGENDA | MEMBER_ADD_AGENDA_ITEM;

    pEntry = &m_aOpcodes[7];

    // STRING: HYPERX 0x004c6c58
    strcpy(pEntry->szName, "REMOVE_AGENDA_ITEM");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_AGENDA | MEMBER_REMOVE_AGENDA_ITEM;

    pEntry = &m_aOpcodes[8];

    // STRING: HYPERX 0x004c6c48
    strcpy(pEntry->szName, "SET_TIMEOUTMSG");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_AGENDA | MEMBER_SET_TIMEOUTMSG;

    pEntry = &m_aOpcodes[9];

    // STRING: HYPERX 0x004c6c34
    strcpy(pEntry->szName, "GLOBAL_INTERACTORS");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_GLOBAL_INTERACTORS;

    pEntry = &m_aOpcodes[10];

    // STRING: HYPERX 0x004c6c24
    strcpy(pEntry->szName, "CHANGE_OF_PLAN");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_PLAN_CHANGE | MEMBER_CHANGE_OF_PLAN;

    pEntry = &m_aOpcodes[11];

    // STRING: HYPERX 0x004c6c10
    strcpy(pEntry->szName, "CHANGE_TO_BEHAVIOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_BEHAVIOUR | MEMBER_CHANGE_TO_BEHAVIOR;

    pEntry = &m_aOpcodes[12];

    // STRING: HYPERX 0x004c6bfc
    strcpy(pEntry->szName, "DECLARE_BEHAVIOR");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_BEHAVIOUR | MEMBER_DECLARE_BEHAVIOR;

    pEntry = &m_aOpcodes[13];

    // STRING: HYPERX 0x004c6be8
    strcpy(pEntry->szName, "DECLARE_LOCAL_REALS");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_PLAN_CHANGE | MEMBER_DECLARE_LOCAL_REALS;

    pEntry = &m_aOpcodes[14];

    // STRING: HYPERX 0x004c6bd4
    strcpy(pEntry->szName, "SET_BEHAVIOR_PARAM");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_BEHAVIOUR | MEMBER_SET_BEHAVIOR_PARAM;

    pEntry = &m_aOpcodes[15];

    // STRING: HYPERX 0x004c6bbc
    strcpy(pEntry->szName, "SET_INTERACTOR_PARAM");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_ACTION | MEMBER_SET_INTERACTOR_PARAM;

    pEntry = &m_aOpcodes[16];

    // STRING: HYPERX 0x004c6bb0
    strcpy(pEntry->szName, "DO_ACTION");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_ACTION | MEMBER_DO_ACTION;

    pEntry = &m_aOpcodes[17];

    // STRING: HYPERX 0x004c6ba0
    strcpy(pEntry->szName, "DO_LOCAL_ACTION");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_ACTION | MEMBER_DO_LOCAL_ACTION;

    pEntry = &m_aOpcodes[18];

    // STRING: HYPERX 0x004c6b90
    strcpy(pEntry->szName, "DO_NET_ACTION");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_ACTION | MEMBER_DO_NET_ACTION;

    pEntry = &m_aOpcodes[19];

    // STRING: HYPERX 0x004c6b80
    strcpy(pEntry->szName, "DECIDE_BY_AMONG");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_DECIDE | MEMBER_DECIDE_BY_AMONG;

    pEntry = &m_aOpcodes[20];

    // STRING: HYPERX 0x004c6b68
    strcpy(pEntry->szName, "DECIDE_BY_WITH_AMONG");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_DECIDE | MEMBER_DECIDE_BY_WITH_AMONG;

    pEntry = &m_aOpcodes[21];

    // STRING: HYPERX 0x004c6b5c
    strcpy(pEntry->szName, "SET_DEBUG");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_DEBUG | MEMBER_SET_DEBUG;

    pEntry = &m_aOpcodes[22];

    // STRING: HYPERX 0x004c6b50
    strcpy(pEntry->szName, "SET_TRACE");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_DEBUG | MEMBER_SET_TRACE;

    pEntry = &m_aOpcodes[23];

    // STRING: HYPERX 0x004c6b48
    strcpy(pEntry->szName, "Enable");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_ENABLE;

    pEntry = &m_aOpcodes[24];

    // STRING: HYPERX 0x004c6b40
    strcpy(pEntry->szName, "Block");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_BLOCK;

    pEntry = &m_aOpcodes[25];

    // STRING: HYPERX 0x004c6b34
    strcpy(pEntry->szName, "DataBlock");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_DATA_BLOCK;

    pEntry = &m_aOpcodes[26];

    // STRING: HYPERX 0x004c6b28
    strcpy(pEntry->szName, "END_PLAN");
    pEntry->nMagic = PLAN_MARKER_NODE;
    pEntry->nValue = CATEGORY_END_PLAN;

    pEntry = &m_aOpcodes[27];

    strcpy(pEntry->szName, g_pszPlanVmEmptyName);
    pEntry->nMagic = 0xffffffff;
    pEntry->nValue = PLAN_OPCODE_NONE;

    pEntry = &m_aTokenRefs[0];

    // STRING: HYPERX 0x004c6b1c
    strcpy(pEntry->szName, "_BROADCAST_");
    pEntry->nValue = PLAN_TOKEN_BROADCAST;

    pEntry = &m_aTokenRefs[1];

    // STRING: HYPERX 0x004c6b0c
    strcpy(pEntry->szName, "_STRAIGHTAHEAD_");
    pEntry->nValue = PLAN_TOKEN_STRAIGHTAHEAD;

    pEntry = &m_aTokenRefs[2];

    // STRING: HYPERX 0x004c6afc
    strcpy(pEntry->szName, "_TEMPORARILY_");
    pEntry->nValue = PLAN_TOKEN_TEMPORARILY;

    pEntry = &m_aTokenRefs[3];

    // STRING: HYPERX 0x004c6aec
    strcpy(pEntry->szName, "_MESSAGE_DATA_");
    pEntry->nValue = PLAN_TOKEN_MESSAGE_DATA;

    pEntry = &m_aTokenRefs[4];

    // STRING: HYPERX 0x004c6adc
    strcpy(pEntry->szName, "_NO_BEH_CHANGE_");
    pEntry->nValue = PLAN_TOKEN_NO_BEH_CHANGE;

    short nSlot = PLAN_NAMED_TOKENS;

    pEntry = &m_aTokenRefs[5];

    // STRING: HYPERX 0x004c6acc
    strcpy(pEntry->szName, "_PREVIOUS_BEH_");
    pEntry->nValue = PLAN_TOKEN_PREVIOUS_BEH;

    int nIndex = 0;

    do
    {
        // STRING: HYPERX 0x004c6ac4
        sprintf(szToken, "_%hd_", nIndex);

        pEntry = &m_aTokenRefs[nSlot++];

        strcpy(pEntry->szName, szToken);
        pEntry->nValue = nIndex;

        nIndex++;
    } while ((short)(nSlot - PLAN_NAMED_TOKENS) <= PLAN_LAST_NUMBERED_TOKEN);

    pEntry = &m_aTokenRefs[nSlot];

    strcpy(pEntry->szName, g_pszPlanVmEmptyName);
    pEntry->nValue = -1;

    m_pszText = new char[PLAN_SOURCE_CHARS];

    if (m_pszText != 0)
    {
        m_pszText[0] = 0;
    }

    m_pStream = new PlanWord[PLAN_STREAM_WORDS];
}

// FUNCTION: HYPERX 0x0047dab0
PlanVM::~PlanVM()
{
    if (m_pszText != 0)
    {
        delete[] m_pszText;
        m_pszText = 0;
    }

    if (m_pStream != 0)
    {
        delete[] m_pStream;
        m_pStream = 0;
    }
}

#define PATH_CHARS 0x100

// The sentinel goes out too, and the descriptor is closed twice on the success path.
// FUNCTION: HYPERX 0x0047daf0
char PlanVM::SaveBinary(char* pszName)
{
    OwnedFile file;
    char      szPath[PATH_CHARS];
    int       nWord;

    if (GetModuleFileNameA(0, szPath, PATH_CHARS) > 0)
    {
        char* const pszSeparator = strrchr(szPath, '\\');

        if (pszSeparator != 0)
        {
            *CharNextA(pszSeparator) = 0;
        }
    }

    // STRING: HYPERX 0x004c6d60
    strcat(szPath, "Data\\Plans\\");

    strcat(szPath, pszName);

    // STRING: HYPERX 0x004c6d58
    strcat(szPath, ".pbn");

    if (file.m_file.Open(szPath, FILE_MODE_WRITE) != 0)
    {
        file.m_file.Write((char*)&m_nPos, sizeof(m_nPos));

        int nIndex = 0;

        do
        {
            nWord = m_pStream[nIndex++].n;

            file.m_file.Write((const void*)&nWord, 4);
        } while (nWord != CATEGORY_END_PLAN);

        file.m_file.CloseAndTest();

        return 1;
    }

    // STRING: HYPERX 0x004c6d28
    throw "Cannot open binary plan file. Is it locked?\n";
}

// One short of the buffer, so the terminator fits.
#define PLAN_TEXT_LIMIT 0x1387f

// Only the first byte of each of the four tables is cleared: a leading zero marks an empty slot.
// FUNCTION: HYPERX 0x0047dca0
char PlanVM::ReadAll(FileObj* pFile)
{
    char ch;
    int  nSlot;

    m_nLength = 0;

    if (m_pszText == 0 || m_pStream == 0)
    {
        return 0;
    }

    while (pFile->Read(&ch, 1) != 0)
    {
        m_pszText[m_nLength] = ch;

        m_nLength++;

        if (m_nLength >= PLAN_TEXT_LIMIT)
        {
            // STRING: HYPERX 0x004c6d6c
            throw "This plan file is too large.  Why not break it up into separate plans?\n";
        }
    }

    m_pszText[m_nLength] = 0;

    m_nCursor = 0;
    m_nPos    = 0;

    for (nSlot = 0; nSlot < PLANVM_BEHAVIOR_LIMIT; nSlot++)
    {
        m_aBehaviorNames[nSlot][0]  = 0;
        m_aLocalRealNames[nSlot][0] = 0;

        for (int nRow = 0; nRow < PLANVM_MESSAGES_PER_BEHAVIOR; nRow++)
        {
            m_aMessageNames[nSlot * PLANVM_MESSAGES_PER_BEHAVIOR + nRow][0] = 0;
        }
    }

    for (nSlot = 0; nSlot < PLANVM_INTERACTOR_NAME_LIMIT; nSlot++)
    {
        m_aInteractorNames[nSlot][0] = 0;
    }

    return 1;
}

#define PLAN_PATH_CHARS 0x100

// The path is built beside the executable; a file that will not open throws.
// FUNCTION: HYPERX 0x0047dd80
char PlanVM::LoadText(short nCamera, char* pszName)
{
    char      szPath[PLAN_PATH_CHARS];
    OwnedFile file;

    if (GetModuleFileNameA(0, szPath, PLAN_PATH_CHARS) > 0)
    {
        char* const pSeparator = strrchr(szPath, '\\');

        if (pSeparator != 0)
        {
            *CharNextA(pSeparator) = '\0';
        }
    }

    // STRING: HYPERX 0x004c6d60
    strcat(szPath, "Data\\Plans\\");
    strcat(szPath, pszName);

    // STRING: HYPERX 0x004c6dd4
    strcat(szPath, ".txt");

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
        if (_strcmpi(szToken, "DECLARE_LOCAL_REALS") == 0)
        {
            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken) && !IsNumericChar(szToken))
            {
                // STRING: HYPERX 0x004c6ec0
                throw "Expected an argument enumerator.  First two args are id and string name.\n";
            }

            ParseNumber(szToken, &count);

            for (short nSlot = 0; nSlot < (short)count.n; nSlot++)
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
        if (_strcmpi(szToken, "DECLARE_BEHAVIOR") == 0)
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
        if (_strcmpi(szToken, "SET_MESSAGE_INTERACTOR") == 0)
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

            strcpy(m_aMessageNames[nBehaviors * PLANVM_MESSAGES_PER_BEHAVIOR + nMessages++],
                   szToken);
        }

        // STRING: HYPERX 0x004c6bbc
        if (_strcmpi(szToken, "SET_INTERACTOR_PARAM") == 0)
        {
            ReadNextToken(szToken);

            if (!IsIdentifierPrefix(szToken) && !IsNumericChar(szToken))
            {
                throw "Expected an argument enumerator.\n";
            }

            ReadNextToken(szToken);

            // STRING: HYPERX 0x004c6f0c
            if (_strcmpi(szToken, "id_SET_InteractorName") == 0)
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

    return (unsigned char)bMatch;
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

    return (unsigned char)bMatch;
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

    return (unsigned char)bMatch;
}

// FUNCTION: HYPERX 0x0047e4d0
char PlanVM::EmitArgToken()
{
    PlanOpcodeRef tok;
    char          szToken[TOKEN_BUFFER];

    ReadNextToken(szToken);

    g_wArgsSinceDeclareBehavior++;
    g_wArgsSinceChangeToBehavior++;
    g_wArgsSinceChangeOfPlan++;
    g_wArgsSinceSetInteractorParam++;
    g_wArgsSinceDeclareLocalReals++;

    if (IsIdentifierPrefix(szToken))
    {
        tok = LookupSymbol(szToken);

        if (tok.nValue == PLAN_OPCODE_NONE)
        {
            szToken[TOKEN_MESSAGE_LENGTH]                  = 0;
            g_szBadPlanIdValue[BAD_PLAN_ID_MESSAGE_LENGTH] = 0;
            strcat(g_szBadPlanIdValue, szToken);

            throw g_szBadPlanIdValue;
        }

        if (IsPRN(tok.szName))
        {
            tok.nValue = PlanWord::Pack(PLAN_ARG_PRONOUN, tok.nValue);
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
            szToken[TOKEN_MESSAGE_LENGTH]                  = 0;
            g_szBadNumberInPlan[BAD_NUMBER_MESSAGE_LENGTH] = 0;
            strcat(g_szBadNumberInPlan, szToken);

            throw g_szBadNumberInPlan;
        }
    }

    if (szToken[0] == '"')
    {
        short nIndex = FindLocalRealIndex(szToken);

        if (nIndex >= 0)
        {
            m_pStream[m_nPos].n = PlanWord::Pack(PLAN_ARG_LOCAL_REAL, nIndex);
            m_nPos++;

            return 1;
        }

        nIndex = FindInteractorParamIndex(szToken);

        if (nIndex >= 0)
        {
            m_pStream[m_nPos].n = PlanWord::Pack(PLAN_ARG_INTERACTOR_PARAM, nIndex);
            m_nPos++;

            return 1;
        }
        else if (g_wArgsSinceDeclareBehavior == 3)
        {
            if (EmitStringLiteral(szToken, m_pStream, &m_nPos) == 0)
            {
                g_szInabilityToReadString[UNREADABLE_STRING_MESSAGE_LENGTH] = 0;
                strcat(g_szInabilityToReadString, szToken);

                throw g_szInabilityToReadString;
            }

            return 1;
        }
        else if (g_wArgsSinceChangeOfPlan == 2)
        {
            if (EmitStringLiteral(szToken, m_pStream, &m_nPos) == 0)
            {
                g_szInabilityToReadString[UNREADABLE_STRING_MESSAGE_LENGTH] = 0;
                strcat(g_szInabilityToReadString, szToken);

                throw g_szInabilityToReadString;
            }

            strcpy(tok.szName, PlanVM::UnquoteString(szToken));

            return 1;
        }
        else if (g_wArgsSinceChangeOfPlan == 3)
        {
            PlanVM* const pSub = new PlanVM;

            if (pSub != 0)
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
            nIndex = FindInteractorParamIndexOrThrow(PlanVM::UnquoteString(szToken));

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
                g_szInabilityToReadString[UNREADABLE_STRING_MESSAGE_LENGTH] = 0;
                strcat(g_szInabilityToReadString, szToken);

                throw g_szInabilityToReadString;
            }

            return 1;
        }
    }

    tok = LookupKeyword(szToken);

    switch (tok.nValue)
    {
        case CATEGORY_PLAN_CHANGE | MEMBER_CHANGE_OF_PLAN:
            g_wArgsSinceChangeOfPlan = 0;
            break;

        case CATEGORY_BEHAVIOUR | MEMBER_DECLARE_BEHAVIOR:
            g_wArgsSinceDeclareBehavior = 0;
            break;

        case CATEGORY_BEHAVIOUR | MEMBER_CHANGE_TO_BEHAVIOR:
            g_wArgsSinceChangeToBehavior = 0;
            break;

        case CATEGORY_PLAN_CHANGE | MEMBER_DECLARE_LOCAL_REALS:
            g_wArgsSinceDeclareLocalReals = 0;
            break;

        case CATEGORY_ACTION | MEMBER_SET_INTERACTOR_PARAM:
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
    if (_mbsspnp((const unsigned char*)pszText, (const unsigned char*)"xXABCDEFabcdef0123456789.+-_") == 0)
    {
        const int nLength = strlen(pszText) + 1;

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
            if (strpbrk(pszText, g_pszDecimalPoint) != 0)
            {
                value.fl = (float)atof(pszText);
            }
            else
            {
                value.n = atol(pszText);
            }
        }

        if (pszCopy != 0)
        {
            delete[] pszCopy;
        }

        *pOut = value;

        return 1;
    }

    return 0;
}

// The last character over the limit is already in the stream when the failure is reported.
// FUNCTION: HYPERX 0x0047eb30
char PlanVM::EmitStringLiteral(const char* pszToken, PlanWord* paStream, int* pnPos)
{
    paStream[*pnPos].n = PLAN_MARKER_STRING;

    short iChar = 1;

    char cAt = pszToken[1];

    while (cAt != '"')
    {
        if (cAt == '_')
        {
            cAt = ' ';
        }

        paStream[*pnPos + iChar].n = cAt;

        if (iChar > PLAN_STRING_LIMIT)
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
        cSeparator = (char)IsSeparatorChar();

        while (cSeparator == 0)
        {
            cComment = (char)IsCommentOrGroup();

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

            cSeparator = (char)IsSeparatorChar();

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

    while ((char)IsSeparatorChar())
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

    while (&m_aLocalRealNames[nSlot] != 0 && nSlot < PLANVM_BEHAVIOR_LIMIT)
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
short PlanVM::FindInteractorParamIndex(char* pszName)
{
    char szName[0x28];

    short nSlot = 0;

    strcpy(szName, pszName);
    strcpy(szName, PlanVM::UnquoteString(szName));

    while (&m_aInteractorNames[nSlot] != 0 && nSlot < PLANVM_INTERACTOR_NAME_LIMIT)
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

    while (m_aOpcodes[nSlot].nValue != PLAN_OPCODE_NONE)
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

    while (m_aTokenRefs[nToken].nValue != PLAN_OPCODE_NONE)
    {
        if (strcmp(pszName, m_aTokenRefs[nToken].szName) == 0)
        {
            return m_aTokenRefs[nToken];
        }

        nToken++;
    }

    short nSymbol = 0;

    while (m_aSymbols[nSymbol].nValue != PLAN_OPCODE_NONE)
    {
        if (_strcmpi(pszName, m_aSymbols[nSymbol].szName) == 0)
        {
            return m_aSymbols[nSymbol];
        }

        nSymbol++;
    }

    return m_aSymbols[nSymbol];
}

// A refusal appends the name to a global message and throws it as a char pointer.

// Each base message's own length, which is where the appended name starts.
#define BEHAVIOR_MESSAGE_LENGTH   31
#define INTERACTOR_MESSAGE_LENGTH 33

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
            g_szCouldNotFindBehaviorNamed[BEHAVIOR_MESSAGE_LENGTH] = 0;
            strcat(g_szCouldNotFindBehaviorNamed, pszName);

            throw (char*)g_szCouldNotFindBehaviorNamed;
        }

        nSlot++;
    }

    return nSlot;
}

// FUNCTION: HYPERX 0x0047f1a0
short PlanVM::FindInteractorParamIndexOrThrow(char* pszName)
{
    for (short nSlot = 0; nSlot < PLANVM_INTERACTOR_NAME_LIMIT; nSlot++)
    {
        if (&m_aInteractorNames[nSlot] != 0)
        {
            if (strcmp(m_aInteractorNames[nSlot], pszName) == 0)
            {
                return nSlot;
            }
        }
    }

    g_szCouldNotFindInteractorNamed[INTERACTOR_MESSAGE_LENGTH] = 0;
    strcat(g_szCouldNotFindInteractorNamed, pszName);

    throw (char*)g_szCouldNotFindInteractorNamed;
}

#define SYMBOL_NAME_MAX 40

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
    char      szToken[SYMBOL_NAME_MAX];
    char      szPrefixed[SYMBOL_NAME_MAX];

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

        if ((char)IsAAPrefix(szToken) != 0)
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
        if ((char)IsCommentOrGroup() != 0)
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
        if ((char)IsCommentOrGroup() == 0)
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

    pTable[nSlot].nValue = -1;
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
