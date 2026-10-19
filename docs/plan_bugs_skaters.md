# Plan bugs in the skater and goalie plans

The shipped AI plans in `Data/PLANS`, read against the plan VM in `src/planvm/`: OFFWING,
DEFENSE, CARRYROK, GOALIE, HUMAN, INPUT, SIDELN, LISTEN, MULTPLR, MINIMAL and CRSHDUM0.
[plan_bugs.md](plan_bugs.md) gives the CARRYROK "GoForGoal" timer bug, which is the pattern and is
not repeated here, the ratings, and the [plan VM rules](plan_bugs.md#plan-vm-rules) the findings
rest on. Plan lines are quoted as `python tools/plan_dump.py <PLAN> --data <Data>` prints them.

## 1. CARRYROK "PassingPlay": the re-armed timeout tests slot 0

**Confirmed.** This is the known bug's pattern in a second behaviour.

    DECLARE_BEHAVIOR 7 "PassingPlay" id_BEH_GlideToObject
        SET_MESSAGE_INTERACTOR id_MSG_Initiate
            ...
                    SET_TIMEOUTMSG 100, id_MSG_TimeOutMessage, id_PRN_Me, id_PRN_MyTeammate
        SET_MESSAGE_INTERACTOR id_MSG_TimeOutMessage
            Enable
                DECIDE_BY_AMONG id_DCF_RecvrInRangeNView()
                    [0] DO_ACTION id_ACF_ReallySendMsg(id_PRN_Me, id_MSG_Extra0, id_PRN_MyTeammate)
                    [1] SET_TIMEOUTMSG 50, id_MSG_TimeOutMessage, id_PRN_Me

The first timeout carries the teammate's slot. When the receiver is out of range or view, arm [1]
re-arms the timer with no fourth leaf, so every later timeout carries data 0.
`PlanCond::RecvrInRangeNView` (0x00466b70) reads the message data as the receiver's slot
(`src/planvm/plancond.cpp`). From the second check on it tests slot 0. That is the right
receiver only when the carrier is slot 2. Slot 0 tests itself, and slots 1 and 3 test an
opponent.

"GoForGoal"'s `SET_TIMEOUTMSG 15, id_MSG_TimeOutMessage, id_PRN_Me` (the known bug) does not reach
this handler: "PassingPlay" is not digressive, so entering it detaches the timer.

In game: after one failed check, the AI carrier decides whether to pass by the position of slot 0.
It can pass when its teammate is covered, or keep skating when the teammate is open.

Fix: give the re-arm a fourth leaf, `SET_TIMEOUTMSG 50, id_MSG_TimeOutMessage, id_PRN_Me,
id_PRN_MyTeammate` (add the word 0xfedc0001 and raise the node's count from 3 to 4). The port does
this (see [plan_bugs.md](plan_bugs.md#summary)).

Related dead code: "PassingPlay" also handles `id_MSG_Extra2` and re-arms it with the teammate's
slot, but nothing in "PassingPlay" sends the first `id_MSG_Extra2`. Only a GoForGoal `Extra2`
still queued across the behaviour change reaches it, and that one carries data 0.

## 2. DEFENSE: slot pronouns used as collision objects

**Confirmed.** Six collision interactors name opponents by slot where every other skater and
goalie plan names a collision object:

    DECLARE_BEHAVIOR 2 "AttackRokCarrier"
        SET_COLLISION_INTERACTOR id_PRN_MyLeadOpponent, id_PRN_MyAwarenessBub
        SET_COLLISION_INTERACTOR id_PRN_MyOtherOpponent, id_PRN_MyAwarenessBub
    DECLARE_BEHAVIOR 3 "AttackOpponent"
        (the same two)
    DECLARE_BEHAVIOR 12 "LooseRokAttackOpponent"
        (the same two)

Compare "ManOnMan" in the same plan, which uses `id_PRN_LeadOppnBodyBub` and
`id_PRN_OtherOppnBodyBub` with the same awareness bubble.

The pronouns resolve to slot numbers 0 to 3, and the engine reads those as collision objects 0 to
3. Those all belong to dromers 0 and 1:

| Actor slot | Lead pronoun gives | Should be | Other pronoun gives | Should be |
|---|---|---|---|---|
| 0 | 1, its own baton zone | dromer 1 | 3, dromer 1 | dromer 3 |
| 1 | 0, dromer 0 | dromer 0 | 2, dromer 0's body | dromer 2 |
| 2 | 1, teammate 0's baton zone | dromer 1 | 3, dromer 1 | dromer 3 |
| 3 | 0, dromer 0 | dromer 0 | 2, dromer 0's body | dromer 2 |

In game: these three attacking behaviours never react when dromer 2 or 3 comes near. Dromers 1 and
3 react only to dromer 0, often the human. Dromers 0 and 2 react to dromer 1 through object 3, and
dromer 2 also to its teammate's baton zone. When dromer 0 is an AI defender, its "lead" pair is its
own baton zone inside its own awareness bubble, so that interactor fires whenever the engine tests
it, if the zone's radius puts it inside the bubble (not checked against the data).
`DispatchCollisionToRule` stops at the first overlapping rule, so such a pair could also hide
later rules in the same behaviour. In "AttackRokCarrier" that means a fresh `AggressionOnOthers`
decision each time, which either clears its timers and attacks or re-enters the behaviour.

Fix: replace the six leaves. `id_PRN_MyLeadOpponent` (0xfedc0002) becomes
`id_PRN_LeadOppnBodyBub` (0xfedc0021), and `id_PRN_MyOtherOpponent` (0xfedc0003) becomes
`id_PRN_OtherOppnBodyBub` (0xfedc0022), matching "ManOnMan".

## 3. MULTPLR "Arming" and "ScoreArming": spins driven by dromer 0

**Confirmed.** The same missing-data pattern, in the multiplier.

`PlanAction::SpinTurnstile` (0x004671c0, `src/planvm/planaction.cpp`), `PlanCond::SpinUp`
(0x004672d0) and, in a message handler, `PlanAction::ReflectOffModel` (0x00405920,
`src/planvm/planactionrecoil.cpp`) all read the message data as the slot of the dromer that hit the
multiplier. "Suspense" supplies it: each collision sends `id_MSG_YerHit` with 0, 1, 2 or 3.

Two paths lose it:

    DECLARE_BEHAVIOR 4 "Arming"
        SET_MESSAGE_INTERACTOR id_MSG_YerHit
            Enable
                DO_ACTION id_ACF_SpinTurnstile()
                SET_TIMEOUTMSG 20, id_MSG_TimeOutMessage, id_PRN_Me
                ...
        SET_MESSAGE_INTERACTOR id_MSG_TimeOutMessage
            Enable
                DO_ACTION id_ACF_SendMessage(id_PRN_Me, id_MSG_YerHit)
                CHANGE_TO_BEHAVIOR 6 "Spin"

The timer carries data 0, the forwarded `id_MSG_YerHit` inherits it, and "Spin" runs
`SpinTurnstile` for dromer 0. "ScoreArming" has the same timeout handler.

    DECLARE_BEHAVIOR 5 "ScoreArming"
        SET_MESSAGE_INTERACTOR id_MSG_Initiate
            Enable
                DO_ACTION id_ACF_ReallySendMsg(id_PRN_Me, id_MSG_Extra0)
        SET_MESSAGE_INTERACTOR id_MSG_Extra0
            Enable
                DO_ACTION id_ACF_SpinTurnstile()
                SET_TIMEOUTMSG 20, id_MSG_TimeOutMessage, id_PRN_Me
                DECIDE_BY_AMONG id_DCF_SpinUp()
                    [0] DO_ACTION id_ACF_ReflectOffModel()

`id_MSG_Extra0` inherits the data of `id_MSG_Initiate`, which is always 0. The hitter's slot
arrives later in `id_MSG_YerHit`, which "ScoreArming" uses only for scoring.

In game: the multiplier's second spin impulse takes its sign from dromer 0's motion relative to the
multiplier (the cross product of its offset and velocity, 0x00467266 to 0x0046727d), so it can kick
the opposite way. When a rok carrier other than dromer 0 arms the multiplier, the spin is
computed from dromer 0. If dromer 0 happens to be heading along a line through the multiplier,
`SpinUp` passes and `ReflectOffModel` acts on dromer 0 wherever it is. Above speed 20 it bounces
dromer 0 with a hit animation and `id_MSG_YerHit`. Below that it nudges dromer 0 by 0.5.

Fix: in both timers add `_MESSAGE_DATA_` as a fourth leaf (0xabcdabce). In "ScoreArming", move the
`ReallySendMsg(id_PRN_Me, id_MSG_Extra0)` from `id_MSG_Initiate` into the `id_MSG_YerHit`
handler, where it inherits the hitter's slot.

TURNSTIL, outside this scope, has the same timeout path in "Arming", and its "ClockStoppedSpin"
sends itself a data-0 `id_MSG_YerHit` from `id_MSG_Initiate`.

## 4. DEFENSE: a forwarded teammate substitution is dropped while the clock is stopped

**Partly confirmed.** The gate is real, but "always" is wrong: see the correction below.

    GLOBAL_INTERACTORS
        SET_MESSAGE_INTERACTOR id_MSG_SubstituteDromer
            Enable
                DECIDE_BY_AMONG id_DCF_ClockStoppedSemaphore()
                    [0]
                        DECIDE_BY_AMONG id_DCF_OkToSubstituteDromer()
                            [0] CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_
                            [1]
                                DO_ACTION id_ACF_DesiredSubstitute(_MESSAGE_DATA_)
                                CHANGE_OF_PLAN "Sideln" 8 "SkateToLockerRoom"
                    [1] CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_

`ClockStoppedSemaphore` (0x00463e00) answers 1 while the actor's clock-stopped flag is set. Only
one path sends this message to an AI skater: HUMAN "GettingThere", the face-off behaviour, forwards
the substitute keys to an AI teammate with `SendMessage(id_PRN_MyTeammate,
id_MSG_SubstituteDromer, _MESSAGE_DATA_)`. At a face-off every AI skater is in DEFENSE "SkateToFaceOff"
or "GettingThere", and every plan's `id_MSG_GoalScored` and `id_MSG_StartDrag` handler has set the
flag to 1 on the way there. So the request always takes arm [1] and does nothing. The OFFWING,
CARRYROK and HUMAN handlers have no such gate, and play sound 126 when they refuse.

Correction: not every AI skater is in DEFENSE at a face-off. At a drag, DEFENSE's
`id_MSG_StartDrag` sends an injured AI skater to SIDELN 9 "InjSubstitute", whose
`id_MSG_SubstituteDromer` handler has no clock gate and applies `DesiredSubstitute(_MESSAGE_DATA_)`.
HUMAN's `OkToSubstituteTeammate` (0x004641b0) checks only the roster, so the forward lands when the
teammate is injured.

In game: pressing a substitute key at a face-off to bench a healthy AI teammate does nothing and
gives no refusal sound. An injured teammate is substituted.

Fix: swap the two arms of the `ClockStoppedSemaphore` decision, or drop the gate so the handler
matches OFFWING's.

## 5. GOALIE "Flying": the timer sends a message nothing in "Flying" handles

**Probable.**

    DECLARE_BEHAVIOR 6 "Flying" id_BEH_Fly
        SET_MESSAGE_INTERACTOR id_MSG_Extra0
            Enable
                SET_TIMEOUTMSG 100, id_MSG_TimeOutMessage, id_PRN_Me
                ...
        SET_MESSAGE_INTERACTOR id_MSG_Extra1
            Enable
                DO_ACTION id_ACF_KickDownToSurface()

"Flying" has no `id_MSG_TimeOutMessage` handler and GOALIE has no global one. Nothing in "Flying"
sends `id_MSG_Extra1`. "SaveFromGoal"'s `SET_TIMEOUTMSG 30, id_MSG_Extra1`, the only `Extra1`
sender in any plan, cannot reach it: "Flying" is not digressive, so the change into it detaches
every pending timer. That the authors meant `Extra1` is inference. "SaveFromGoal" shows the
intended shape: its `id_MSG_Extra0` arms `SET_TIMEOUTMSG 30, id_MSG_Extra1` to kick the goalie
down.

In game: a goalie knocked into the air without the rok is never kicked down after a second. The
unhandled timeout dies when the goalie leaves "Flying" for a non-digressive behaviour such as
"LandThenPassItOff"; only a digressive one such as "CompleatFlying" inherits it.

Fix: change the timer's message leaf from `id_MSG_TimeOutMessage` (69) to `id_MSG_Extra1` (124).

## 6. Two-target `CAvoidObjects` items steer nothing

**Probable.** Plan data and VM together.

    ADD_AGENDA_ITEM id_AGD_CAvoidObjects, id_PRN_MyTeammate, 9                 (17 in DEFENSE and OFFWING)
    ADD_AGENDA_ITEM id_AGD_CAvoidObjects, id_PRN_MyLeadOpponent, id_PRN_MyOtherOpponent  (6 in CARRYROK)

`CAvoidObjects::BindWithNode` (0x0040f890, `src/planvm/cavoidobjects.cpp`) sets only the
two targets for a three-leaf node. It leaves range and strength as they were. A recycled
agenda item has both at 0 from `OnListChanged` (0x0040f970). A never-recycled one has whatever the
heap held: the pool's `operator new(0x30)` (0x00413660, 0x004136f0) sets only +4 to +0xc and the
vtable, so range and strength are uninitialised, though fresh heap pages are very likely zero. The
one-leaf `Bind` (0x0040f830) sets 80 and -6.5, and the five-leaf form sets them from the node. With a range of 0 the gate
`|dx| + |dy| < range` never passes, so no avoidance force is added. The item still raises
`id_MSG_Blocked` and `id_MSG_Breakaway`, which the plans do listen for.

The second leaf `9` in DEFENSE and OFFWING is slot 9, the main camera. Nothing suggests the
authors meant to avoid the camera. It reads like a distance written where the engine expects a
slot.

In game: AI skaters never swerve round their teammate or, as carriers, round the two opponents.
Only the `_MESSAGE_DATA_, 1, 85.0, -10.0` items for arena hazards steer.

Fix: data alone cannot give the three-leaf form a range. Use one five-leaf item per target, for
example `id_PRN_MyTeammate, 0, 80.0, -6.5`, with 0 so the item is not released on range.

## Looks odd, may be intended

- **GOALIE "SaveFromGoal"** handles a second `id_MSG_ShotOnGoal` with `LaunchAt(id_PRN_MyGoal,
  2.0)` and then "Save". Every other `id_MSG_ShotOnGoal` handler in the plan launches at slot 6,
  the rok. The goalie leaps back toward its own goal instead of at the shot.
- **DEFENSE "LooseRokAttackOpponent"** arms `SET_TIMEOUTMSG 200, id_MSG_TimeOutMessage` in
  `id_MSG_SendRandomTimeout` but has no handler for it. Its siblings "AttackRokCarrier",
  "AttackOpponent" and "PursueRok" all act on their timeouts. Its own `id_MSG_ExitBehavior` clears
  the timer, so nothing leaks, but the defender never re-decides by timeout.
- **SIDELN "HumanInjured"** sends itself `id_MSG_TimeOutMessage` from `id_MSG_Extra5` arm [2], and
  nothing in the behaviour or the plan handles it. "Injured" runs its death sequence on that
  message. "HumanInjured" runs the same sequence from `id_MSG_Extra2` instead, so the send looks
  like a leftover.
- **HUMAN "Stopped2"** attacks without moving to "CompleatAnimation": its `id_MSG_ChooseVictim`
  arms lack the `CHANGE_TO_BEHAVIOR 2` that "GeneralGliding" and "GlideToStop2" have. The attack
  then runs without `AnimationSemaphore(2)`.
- **OFFWING "AttackOppnClosestToRok" and "AttackDefOpponent"** give both arms of
  `TeammateIsHumanOrInjured` the same action, and unlike the other OFFWING `id_MSG_PassTo`
  handlers never ask the carrier for the pass.
- **OFFWING "GoOutForward"** re-arms `id_MSG_CheckPassingDistance` only on arm [0]. After the first
  pass request it stops checking. "RunPattern", "ReceivePass" and "PassingPlayRecv" re-arm first.
- **`id_MSG_Incoming`** carries 0 from INPUT and 1 from CARRYROK "PassingPlay", but the OFFWING
  handler ignores the data and always goes to "GoOutForward". OFFWING "SetUpToReceive",
  "ReceivePass" and "PassingPlayRecv" are never entered. They may be an abandoned receiver design.
- **GOALIE `ADD_AGENDA_ITEM id_AGD_CBungeeToward, id_PRN_MyGoal, 3.5, -1.0, 0, 100`**: the sixth
  leaf is read only as "lead the target when it equals 1" (`src/planvm/cbungeetoward.cpp`), so
  100 does nothing. A target goal does not move, so leading it would not matter either.
- **CRSHDUM0** calls `id_ACF_CrashTestSetup` (action 14), which has no handler in the table
  `src/planvm/planvm.cpp` fills. An `id_MSG_Test` to a crash dummy would call a null pointer. This
  is a developers' test plan.
- **INPUT** calls `id_ACF_SetInputMode`, which is `Stub::Assert`, an empty function, in v1.0.0.1.

## Dead behaviours

No plan transition targets these, and the engine starts plans at behaviour 0:

- DEFENSE 9 "Halted".
- OFFWING 5 "SetUpToReceive", 6 "ReceivePass", 7 "PassingPlayRecv".
- HUMAN 11 "GlideToStop" and its pair 12 "GlideToStop2", 17 "GoalScoredDrag". 1 "GeneralGliding"
  and 14 "Stopped2" are reached as the keyboard behaviours' standby partners.
- SIDELN 4 "InjStart", 6 "HumanInjStart".
- CARRYROK 0 "RegroupOnsides" is entered from SIDELN, but its `id_MSG_TimeOutMessage` handler only
  runs on a timer armed inside CARRYROK. Entered through `CHANGE_OF_PLAN`, it inherits no timers.

## Checked and consistent

- Every `ChooseVictim` path sends the attack index 0 to 7. `TakeDamage`, `AttackedAnim` and the
  anim event that sends `id_MSG_YerAttacked` agree on it.
- HUMAN's `id_MSG_Attack` and `id_MSG_SpeedBurst` data come from INPUT and match the arms of
  `AttackOrDeployWeapon` and `IHaveRok`.
- `id_MSG_OkToGetRok` carries 120 from ROK to every `RokInRange` caller.
- GOALIE `IHaveRok` arms [2] and [3] in `id_MSG_Initiate` are unreachable, since that message
  carries 0. They duplicate arms [0] and [1], so this is harmless.
- GOALIE `DecideWithRating(14)` reads `id_GRT_Aggression` in the goalie's own rating table, not
  `id_RAT_ArmoryRange`. The dumper names it from the goalie table for GOALIE.
- HUMAN's global `id_MSG_GoalScored` arms `id_MSG_InDrag` after 10 s. STARTGUN's "Spin" sends
  `id_MSG_InDrag` to all four skaters at the same moment, and play resumes no sooner than 16 s after
  the goal (NGOAL "PostScoreWait4Drag"), so the backup cannot fire into live play.
- Face-off spots, drag-start spots and celebration targets use one table per slot and agree
  between HUMAN, DEFENSE and SIDELN.
