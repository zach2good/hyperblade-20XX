#ifndef HYPERBLADE_PLANCOND_H
#define HYPERBLADE_PLANCOND_H

#include "common/compat.h"

class Interactor;
class EngineObject;
class AgendaItem;
struct PlanNode;

// DATA/ENUMIDS.H's ID_ATK: the attack a YerAttacked message carries.
enum AttackType
{
    ATTACK_PUNCH,
    ATTACK_HIP_CHECK,
    ATTACK_CLOTHES_LINE_L,
    ATTACK_CLOTHES_LINE_R,
    ATTACK_SHOULDER_CHECK_L,
    ATTACK_SHOULDER_CHECK_R,
    ATTACK_LEG_SWEEP_L,
    ATTACK_LEG_SWEEP_R
};

namespace PlanCond
{

// The linker folded these empty bodies onto Stub::Assert, which carries the marker.
inline void AnalyzePadC(Interactor*, PlanNode*, short*, int)
{
}

inline void TryForKillball(Interactor*, PlanNode*, short*, int)
{
}

inline void TargetAcquired(Interactor*, PlanNode*, short*, int)
{
}

void __cdecl DecideGoalieLaunch(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl Decide_Attachment(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         ForceRearView(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl OkToLaunchRok(Interactor* pContext, PlanNode* pArgs, short* pOutValid, int nArg);
void         SetInteractorName(EngineObject* pActor, PlanNode* pNode, class GameBehaviorObject* pBehaviour, Interactor* pInteractor);
void __cdecl SetActiveState(EngineObject* pActor, PlanNode* pNode, class GameBehaviorObject* pBehaviour, Interactor* pInteractor);
void         Decide_Celebration(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl EngageWhichOpponent(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void         TeammateHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl GagTheAnnouncer(Interactor* pContext, PlanNode* pArgs, short* pnOut, int nArg);
void         VictimCamIsOn(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         IsScanningModeOn(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl AttackerIsInRear(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         AnalyzePadA(Interactor* pContext, PlanNode* pArgs, short* pResult, int nSelector);
void __cdecl AnalyzePadB(Interactor* pContext, PlanNode* pArgs, short* pResult, int nKind);
void __cdecl InRangeToAttack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl Decide_BlockOrGoOut(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         SpeedBurstIsRelaxed(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl WhichOppnIsClosestToRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl BallGotBehindMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl GoaliePassingTarget(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl WhichOppnToAttack(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void __cdecl WhichOppnIsClosestToMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void __cdecl ClosestTargetToProjectile(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl ClosestUncoveredOppn(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl NearestSuspenseMult(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl OffenseOrWRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         HostIsViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         RokInWhichTeamsPoss(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl WhoHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         IsDoubleScore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl BungeeVictimLorR(Interactor* pContext, PlanNode* pArgs, short* pResult, int nFlags);
void __cdecl Decide_PassOrRun(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl Decide_HPSpin_Rail_orNot(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         PickACell(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         Random(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl ShotPower(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void __cdecl IfInFaceOff(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void         TeamIsDead(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         TeamIsInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         MyTeamIsInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         WhoWonTheGame(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         IsSenderTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         AmMainDromer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         MyID(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);

// A message's data word packs one field in each nibble.
inline short MessageNibble(unsigned int nData, int nShift)
{
    return (short)((nData >> nShift) & 0xf);
}

void         MessageDataNibl0(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         MessageDataNibl1(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         MessageDataNibl4(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         MsgData4Nibl1X2Nibl0(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         SideKeysAreDown(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl AmViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl TeammateIsViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void __cdecl TeammateIsHumanOrInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl OkToSwitchToTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl OkToPassToTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl DecidePosition(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl CollisionDetail(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         OwnerIsViewOwner(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl ViewOwnersMateInFront(Interactor* pContext, PlanNode* pArgs, short* pOutFacing, int nArg);
void __cdecl IHaveRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nFlag);
void __cdecl DromerHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void         SetResultDirect(Interactor* pContext, PlanNode* pArgs, short* pResult, int nValue);
void __cdecl IsRokInMyZone(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         SubstitutionSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl PickValidSubstitute(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl OkToSubstituteTeammate(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void         GetImmunity(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         AttachedToSender(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl AmIKnockedLoose(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         RokScoreValue(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         CatchBlockOrMiss(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl Jumping(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl RandomInactiveWeapon(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl RandomEmptyArmory(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         WhoKilledMe(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl TakeDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack);
void __cdecl AttackedAnim(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack);
void __cdecl SpecialAttacks(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAnim);
void __cdecl MeOrTeammateClosestToRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         TenOrMorePtSpread(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl RedIsAvailable(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl RatingMaxedOut(Interactor* pContext, PlanNode* pNode, short* pnResult, int nArg);
void __cdecl ChooseTeamTune(Interactor* pContext, PlanNode* pArgs, short* pResult, int nTeam);
void __cdecl MyTeamLosingByAtLeast(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         DecideWithRating(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         AttackOrDeployWeapon(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl DromerNearMine(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         RokValueInMyFavor(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl CheckPassingDistance(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl RecvrInRangeNView(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl ChoosePassingPlay(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         Ducking(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         RokIsCharged(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         RokIsChargedAgainst(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl SpinUp(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl PassOrRunByRatings(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl LooseRokWhatToDo(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl JostledDropRok(Interactor* pContext, PlanNode* pNode, short* pnResult, int nArg);
void __cdecl GoThroughMuliplier(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl PickAttackMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl OffWingJob(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl PickFlyingMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         PickHalfpipeMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         AvoidSpinningTurnstile(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         DecideNever(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl ShootSituation(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl RegroupSituation(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl AggressionOnOthers(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl NearObjectBy(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         OkToGetRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam);
void         CheckScoreOnOwnGoal(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         Interception(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl RokInRange(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl ClosestDromerToRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         SoundBank(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         GetTeamParity(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void         WinningOrLosing(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg);
void __cdecl IsDromerKnockedDown(Interactor* pContext, PlanNode* pArgs, short* pResult, int nDefault);
void __cdecl IsDromerInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);
void __cdecl CheckRenderFlag(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot);

} // namespace PlanCond

#endif
