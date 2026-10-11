# Bugs in the object plans and the team ratings files

HYPERX.EXE v1.0.0.1's shipped object plans and team ratings files, decoded with
`tools/plan_dump.py` and checked against `src/planvm/` and, where it mattered, Ghidra. [plan_bugs.md](plan_bugs.md) covers the confidence ratings and the
[plan VM rules](plan_bugs.md#plan-vm-rules), and [plan_bugs_skaters.md](plan_bugs_skaters.md) the
skater and goalie plans. "Rule N" below is rule N there.

## The decision log

`Interactor::ExecuteScript` (0x0045d860) records each DECIDE branch in one `short`, the
`DecisionLog` in `src/planvm/decisionlog.h`, so that a network copy takes the same branches as
the local object. The local caller passes -1: `GameEngine::DispatchCollisionToRule` for a collision and
`EngineObject::DispatchSceneMessage` (0x00472110) for a message. While bit 15 is set, each DECIDE
calls its selector and pushes the branch (0x0045d8f0 to 0x0045d970); otherwise it pops the oldest
branch and runs that (0x0045d972 to 0x0045d9d3). Branches replay oldest first.

Bits 12 and 13 count the branches, and each count packs them at its own widths:

| Count | Oldest | Second | Third |
|---|---|---|---|
| 1 | 0-11 | | |
| 2 | 0-6 | 7-12 | |
| 3 | 0-4 | 5-8 | 9-11 |

The word goes out after the firing. `MsgEvent::PackScene` keeps the low 14 bits, so a message
replay sees the count with bit 15 clear. `MsgEvent::Pack` keeps the low 8 bits, so a collision
replay sees a count of nought and takes the three-branch arm: five bits of the first branch.

Faults, all in the original:

- The third push masks the branch with 0xe00 where it should shift it left by 9 (0x0045d963), so a
  replay takes branch nought for any third branch under 0x200.
- A second branch of 32 or more sets bit 12 and turns the count to three.
- A fourth push takes the three-branch arm again and repacks the word it already packed.

The `| 0xffffff40) << 7` of the second push carries no meaning. The constants 0x9000, 0xa000 and
0xb000 are negative shorts, VC6 sign-extends them, and it folds 0xffffa000 through the shift. The
typed `(nBranch << 7) | ... | 0xa000` compiles to the same bytes.

### Which shipped handlers the faults reach

`python tools/plan_dump.py --decisions` walks each handler as `ExecuteScript` does. A DECIDE
runs one arm and a Block runs its children in order; no other node runs its children. For each
local path the tool packs the word, keeps the bits the network keeps and replays it.
`--selftest` checks the emulation against the cases above.

Verified, from the code and the 33 plans:

- All 578 DECIDEs sit in a handler's Enable block. No path holds more than two, except one in
  INPUT. No path holds four.
- INPUT "GamePlay" `id_MSG_NewInput` holds the only three-DECIDE path:
  `MessageData[25] > AnalyzePadA[3] > TeammateIsHumanOrInjured`. A third branch of 1 replays as
  nought. INPUT never loads: all fourteen arena ENVs comment out the keyboard object that sets it
  (`#SetPlans input`), and no network slot drives such an object.
- No second-position DECIDE has an arm above 22, so no branch that runs an arm spills into the
  count.
- 99 collision rules in 14 plans contain a DECIDE. Every branch with an arm in them is under 32,
  so a one-DECIDE rule replays its branch unchanged.
- CARRYROK "GoForGoal" has the only two-DECIDE collision rules: one for each opponent's bubble,
  `PassOrRunByRatings` then `ShootSituation`. Both selectors answer 0 or 1. The low byte holds the
  first branch in bits 0-6 and bit 0 of the second in bit 7. The replay reads the first from bits
  0-4 and the second from bits 5-7, so a second branch of 1 replays as 4. `ShootSituation` has two
  arms, so the copy runs neither. The local carrier runs arm 1: a 50-tick `id_MSG_Attack` timer
  and `CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_`.
- The network copy does run `ExecuteScript` on the received word.
  `GameEngine::ProcessInputCommandQueue` (0x00487140) splits a collision word with `Unpack`
  (0x0045d7d0) and calls `CollInteractor::Process` (0x0045ddf0) in mode 1 with the low byte. A
  scene word goes through `UnpackScene` (0x0045d790) to `DispatchSceneMessage` with `bFromInput`
  set. `Pack` (0x0045d730) masks its low field with 0xff, `PackScene` (0x0045d760) with 0x3fff.

Inferred, not watched in a match:

- The missing timer changes nothing. A network copy runs a message only when it comes from the
  queue (`DispatchSceneMessage`, 0x00472110), so the timer's message would be dropped anyway.
- The missing `_NO_BEH_CHANGE_` matters when the rule's target still holds 7 "PassingPlay" or 4
  "Throwing" from an earlier firing of the same node (rule 1). The copy then enters that
  behaviour while the carrier stays in "GoForGoal". The next movement record that disagrees
  re-instantiates the copy's plan (`ProcessReceivedPacket`, 0x004711c0), so other players might
  see the AI carrier start a pass or a throw for one network update.
- Every skater slot has a `CPlayer` in a network game (`g_anNetworkSlotObject`), so the carrier's
  collision does go out.
- The tool follows only the branches that have arms. A selector that answers a value with no arm
  still pushes it. The second-position selectors read so far (`Random`, `TakeDamage`, `MyID`,
  `ShootSituation`, `PassOrRunByRatings`) stay below their arm counts. Not every selector's range
  is decoded.

Plan-side fix: give GoForGoal's `ShootSituation` five arms in both rules, with arm 4 a copy of
arm 1. The selector never answers 2 to 4, and the copy's replay never reads 2 or 3, so their
filler never runs.

## Findings in the object plans

### 1. GRANDOM "Inactivate" gives every skater player 0's speed rating

Confirmed bug. Every arena except NEMDRO and TORDRO has green randomizers.

```
GLOBAL_INTERACTORS
    SET_MESSAGE_INTERACTOR id_MSG_GoalScored
        Enable
            SET_TIMEOUTMSG 15, id_MSG_Inactivate, id_PRN_Me
    SET_MESSAGE_INTERACTOR id_MSG_StartDrag
        Enable
            SET_TIMEOUTMSG 15, id_MSG_Inactivate, id_PRN_Me
    SET_MESSAGE_INTERACTOR id_MSG_Inactivate
        ...
            DO_ACTION id_ACF_ModifyOpposTeamFriction(1, -90, 0)
            DO_ACTION id_ACF_ModifyOpposTeamFriction(1, -90, 1)
```

With a non-zero gate, `ModifyOpposTeamFriction` (`planaction.cpp`, Ghidra 0x00464750)
"restores" both opponents of the target to `g_aOpposSpeedCache[target]`, the _target's_ cached
speed, not their own. It also refills the cache first whenever the stale flag is set, and sets
the flag after every restore. Trace of the two calls:

- Call 1 (target 0): the cache refills to `[s0, s1, s2, s3]`; players 1 and 3 get `s0`.
- Call 2 (target 1): the cache refills to `[s0, s0, s2, s0]`; players 0 and 2 get `cache[1] = s0`.

All four skaters end with player 0's Speed rating. `CGlideOnSurface` reads that rating for
top speed (`CGlideOnSurface::UpdateTerrainPhysics`). `BeginMatchPlay` restores the ratings at
period start, but `StartDrag` follows it at once, so the collapse happens 150 ms into every period
and again 150 ms after every goal. Every green power-up's expiry runs `Inactivate` too, because
"Activate" arms a 1500-tick timer for it. Each arena's green randomizers repeat it: LND has 3,
MUN 6, CHI, HOU, LOS, MOS, NEW and TOK 4, and JAM, SEA, VEG and WAR 8. NEM and TOR have none.
In single player slot 0 is usually the human's skater (not traced for every mode), so the team
files' Speed ratings are mostly ignored on these arenas.

Smallest fix: none in data restores per-player speeds, because the handler can only copy one
cached value to a pair. The real fix is in the handler: write `cache[nFlip]` and
`cache[nFlip + 2]`. A data-only mitigation is to delete both restore lines; a green slow-down
then lasts until the next period start instead of 15 seconds.

### 2. GRANDOM "Inactivate" wipes Seattle's long-pass ability every period

Confirmed bug (checked adversarially). Same handler:

```
            DO_ACTION id_ACF_ModifyRating(11, -11, 0)   ... for slots 0-3
            DO_ACTION id_ACF_ModifyRating(9, -11, 0)    ... for slots 0-3
            DO_ACTION id_ACF_SetImmunity(0, 0)          ... for slots 0-3
```

Rating 11 is `LongPassEnable`, 9 is `LongShotEnable`. `ModifyRating` treats both as flags and
zeroes them for any non-positive amount (`planaction.cpp`). SEATTLE sets
`id_RAT_LongPassEnable = 9` for S._Vanacher and Cowboy_Bob at all levels and Kiem at hard. The
rating lifts the pass-range limit on the receiver (`PlanCond::CheckPassingDistance` and
`RecvrInRangeNView`). After the first `Inactivate`, on-ice Seattle players lose it for the rest of the period. The immunity
reset also cancels the immunity cheat.

The plan already has per-player undo handlers that nothing sends: `EndImmunity`,
`EndSlowDown`, `EndLongShot`, `EndLongPass` (see 11). Smallest fix: delete the blanket resets
from `Inactivate` and add a timer to each power-up arm in "Active", for example
`SET_TIMEOUTMSG 1500, id_MSG_EndLongPass, id_PRN_Me, _MESSAGE_DATA_`. That still clears
Seattle's flag for a player who takes the power-up, but not for everyone.

### 3. RRANDOM: the red power-up almost never ends after 45 seconds

Confirmed in code; almost certainly unintended.

```
DECLARE_BEHAVIOR 2 "Active" id_BEH_SetFlatInPlace
    SET_BEHAVIOR_PARAM id_SET_Digressive, 1
    SET_COLLISION_INTERACTOR id_PRN_ThisRedRandomizerCOB, 41 ... 44
        Enable
            DO_ACTION id_ACF_SendMessage(0, id_MSG_ChangeWatchObject, id_PRN_Me)
            SET_TIMEOUTMSG 150, id_MSG_TimeOutMessage, id_PRN_Me
    SET_MESSAGE_INTERACTOR id_MSG_TimeOutMessage
        Enable
            REMOVE_AGENDA_ITEM id_AGD_CTimerActivity
            CHANGE_TO_BEHAVIOR 2 "Active"
```

`RedIsDown` arms `SET_TIMEOUTMSG 4500, id_MSG_RedIsUp` and sets `SetMaxSpeed(2.0)`. COBs 41-44
are the skaters' awareness bubbles, radius 50, so one of them overlaps the randomizer within a
poll of the hit. 1.5 seconds later the `REMOVE_AGENDA_ITEM` cancels every timer, including
`RedIsUp` (rule 3). The global speed scale then stays at 2.0 until a green randomizer's
`Inactivate` sets 0.7: at a goal, a period start, or any green power-up's expiry. Green arm [9]
sets 1.2. HOUDRO and JAMDRO have no red randomizer. On NEMDRO and TORDRO, which have no green
randomizers, nothing resets it, so it can last across periods. GRANDOM's matching handler has
no `REMOVE_AGENDA_ITEM`, which points to a slip.

Smallest fix: delete the `REMOVE_AGENDA_ITEM` line from RRANDOM "Active"'s
`TimeOutMessage` handler. The stacked 150 timers are harmless, as GRANDOM shows.

Side note, looks odd: `g_flMaxSpeed` is only ever written by config load and these plans. The
default keyboard config sets 1.0; GRANDOM forces 0.7 at every period start. Skaters on NEMDRO
and TORDRO therefore run at the config value, about 40% faster than on the other arenas.

### 4. ROK "BeingCarried": one refused throw breaks every later throw

Probable bug. Rare trigger, severe effect.

```
DECLARE_BEHAVIOR 1 "BeingCarried" id_BEH_UpdateWhileAttached
    SET_MESSAGE_INTERACTOR id_MSG_Throw
        CHANGE_TO_BEHAVIOR 3 "Flying"
        Enable
            DECIDE_BY_AMONG id_DCF_OkToLaunchRok()
                [0] CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_
                [1]
                    ... DO_ACTION id_ACF_Throw()
                    DO_ACTION id_ACF_Unset_AttachPak()
```

`OkToLaunchRok` (`plancond.cpp`) answers 0 when the sender is not the carrier or runs
OffWing, Defense or Sideln. That happens when a Throw arrives late from an animation event
(HUMAN "ChooseVictim", GOALIE "PassItOff") after another player has taken the rok. Arm [0]
then makes `_NO_BEH_CHANGE_` stick (rule 1). Every later legal throw that period runs
`Throw()` and `Unset_AttachPak()` but leaves the rok in "BeingCarried". With no attach,
`CBeAttached::Process` neither moves it nor gives it velocity, and that behaviour has
no pickup or goal collisions. The rok should hang in the air until the period ends, unless
`EndOfPeriod` or a `Deke` into "Decapitation" moves it out of "BeingCarried" first (who sends
`Deke` is not traced). The
sibling handlers (`Shoot`, `PassTo`, `ShootToKill`) all name their target explicitly.

Smallest fix: add `CHANGE_TO_BEHAVIOR 3 "Flying"` to arm [1].

### 5. NGOAL and SGOAL: one own goal turns off the post-goal sequence for the period

Probable bug, moderate effect.

```
    SET_MESSAGE_INTERACTOR id_MSG_NGoalScored
        CHANGE_TO_BEHAVIOR 1 "PostScoreWait4Drag"
        Enable
            DECIDE_BY_AMONG id_DCF_RokScoreValue()
                [0]
                    DECIDE_BY_AMONG id_DCF_CheckScoreOnOwnGoal()
                        [0] ... id_STA_r1xGoal ...
                        [1] CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_
```

The same `[1]` arm appears in each of the four score arms, in both goals.
`CheckScoreOnOwnGoal` (`plancond.cpp`) answers 1 for an own goal, or when the last
carrier's slot is not 0 to 3: a goalie's pass, or no carrier at all. The trigger is wider than an
own goal. The authors use `CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_` as a do-nothing
filler, but here it cancels the header and sticks (rule 1). From then on, goals into that net
leave it in "BasicWaitState". It skips `HeadCamOff`, the start-gun `DropRok`, the
`IfInFaceOff` check that ends the period when a team is injured, and its own `Shoot` restart
at about 16 s. The rok's "RokDrag2" restarts play at 20 s instead. The score still counts.

Smallest fix: replace each inner `[1] CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_` with
`[1] CHANGE_TO_BEHAVIOR 1 "PostScoreWait4Drag"` (8 places across the two files).

Engine note: `RokScoreValue` writes 4 when the rok is attached and then overwrites it in every
path (`plancond.cpp`), so arm [4] is dead and a carried-in rok scores.

### 6. TURNSTIL spins and bounces from slot 0's motion

Probable bug, the known missing-data pattern.

```
DECLARE_BEHAVIOR 2 "ClockStoppedSpin" id_BEH_SetInPlace
    SET_MESSAGE_INTERACTOR id_MSG_Initiate
        Enable
            DO_ACTION id_ACF_ReallySendMsg(id_PRN_Me, id_MSG_YerHit)
    SET_MESSAGE_INTERACTOR id_MSG_YerHit
        Enable
            DO_ACTION id_ACF_SpinTurnstile()
            ...
            DECIDE_BY_AMONG id_DCF_SpinUp()
                [0] DO_ACTION id_ACF_ReflectOffModel()

DECLARE_BEHAVIOR 3 "Arming"
    SET_MESSAGE_INTERACTOR id_MSG_YerHit
        Enable
            ...
            SET_TIMEOUTMSG 5, id_MSG_TimeOutMessage, id_PRN_Me
    SET_MESSAGE_INTERACTOR id_MSG_TimeOutMessage
        Enable
            DO_ACTION id_ACF_SendMessage(id_PRN_Me, id_MSG_YerHit)
```

`YerHit` data is the skater's slot. `SpinTurnstile`, `SpinUp` and `ReflectOffModel` all read
it as a slot (`planaction.cpp`, `plancond.cpp`, `planactionrecoil.cpp`). `Initiate`
carries 0, and the 3-argument timer carries 0, so each hit adds a second `YerHit` about slot
0. The extra `SpinTurnstile` adds an impulse whose direction comes from slot 0's motion, so the
turnstile can spin against the push. In "Arming", the timer's `YerHit` is queued and handled after
the change to "Spin", so there it costs only that extra impulse. In "ClockStoppedSpin", when slot
0's line of travel passes within 1.5 units of the turnstile, `ReflectOffModel` bounces slot 0 or cuts its speed
to 3, wherever it is on the rink.

Smallest fix: delete the `ReallySendMsg` from "ClockStoppedSpin" `Initiate`, because the
collision already sends `YerHit` with the slot. In "Arming", give the timer the slot:
`SET_TIMEOUTMSG 5, id_MSG_TimeOutMessage, id_PRN_Me, _MESSAGE_DATA_`.

### 7. ARMORY: an empty poll makes the next display land in "Closed"

Confirmed mechanism, low effect.

```
DECLARE_BEHAVIOR 0 "DeployWeapon"
    SET_MESSAGE_INTERACTOR id_MSG_Initiate
        CHANGE_TO_BEHAVIOR 1 "Open"
        Enable
            DECIDE_BY_AMONG id_DCF_RandomInactiveWeapon()
                [0] DO_ACTION id_ACF_SendMessage(54, id_MSG_Display)
                [1] DO_ACTION id_ACF_SendMessage(55, id_MSG_Display)
                [2] CHANGE_TO_BEHAVIOR 1 "Open"
                [3] CHANGE_TO_BEHAVIOR 2 "Closed"
```

Arms [0] and [1] rely on the header. After arm [3] (nothing at the armory) the node holds
"Closed" (rule 1), so the next display of a mine or swarf leaves the armory "Closed". Only
"Open" tells skaters `WatchOutArmory`, which sends DEFENSE to "RunThroughArmory" and plays the
human's cue. The next 5 s poll answers [2] and reopens it, so the delay is up to 5 s per
weapon cycle. Smallest fix: add `CHANGE_TO_BEHAVIOR 1 "Open"` to arms [0] and [1].

### 8. SWARF tells the wrong armory to deploy

Probable bug (copy slip), low effect. Affects only arenas with two armories (HOU, LND, LOS,
MOS, NEM, SEA, TOK, VEG).

```
SWARF "SitInArmory":
                [2]
                    DO_ACTION id_ACF_HoverAboveArmory(4, 53)
                    SET_TIMEOUTMSG 200, id_MSG_DeployWeapon, 52
MINE "SitInArmory":
                [2]
                    DO_ACTION id_ACF_HoverAboveArmory(6, 53)
                    SET_TIMEOUTMSG 200, id_MSG_DeployWeapon, 53
```

The swarf waits over armory 53 until 53's own 5 s poll, and armory 52 polls early for
nothing. Smallest fix: change the target to 53.

Related, looks odd: MINE and SWARF "DisplayOverArmory" always send `Close` to 52, even over 53.
Armory 52 can close while it still shows the other weapon, and 53 stays "Open" until its next
poll. No pronoun names "the armory I am over", so there is no one-word fix.

### 9. CAMERA "ViewSelfandOther" can stay in pass view

Looks odd, low effect.

```
DECLARE_BEHAVIOR 3 "ViewSelfandOther" id_BEH_TrackPassPrep
    SET_MESSAGE_INTERACTOR id_MSG_PassReceived
        CHANGE_TO_BEHAVIOR 2 "BasicOverTheShoulderView"
        Enable
            DECIDE_BY_WITH_AMONG id_DCF_CheckRenderFlag(id_PRN_ViewOwner)
                [1] ... CHANGE_TO_BEHAVIOR 5 "PressCam"
                [2] ... CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_
```

Arm [2] is copied from "BasicOverTheShoulderView", where `_NO_BEH_CHANGE_` is right. Here it
cancels the header, and the cancel sticks. `TimeOutMessage` in the same behaviour has the same
shape. `EndPassMode` from HUMAN and INPUT usually rescues the camera. Smallest fix: drop the
`CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_` from both [2] arms.

### 10. LIMBOPO sends every pole's duck to pole 40

Looks odd, harmless. All six poles send `SendMessage(40, id_MSG_DuckingUnder, k)` instead of
`id_PRN_Me`. The handler only reads the slot and sets the rok's kill state, and the first
pole always takes slot 40, so the result is the same.

### 11. Dead handlers and behaviours

Looks odd, no effect today.

- GRANDOM `EndImmunity`, `EndSlowDown`, `EndLongShot`, `EndLongPass`: no plan and no engine
  path sends them. They look like the intended per-effect expiry (see 2).
- NGOAL/SGOAL `DoubleScore`: the engine sends 0x49 only to the sound man (slot 24).
- EXPLODE `ExplodeUniform`: never sent.
- HELMCAM "RearShutOff" and CAMERA "StadiumCam": no `CHANGE_TO_BEHAVIOR` reaches them, and
  the behaviour before each has interactors, so rule 4 does not reach them either.
- RRANDOM "Inactive" `TimeOutMessage`: the awareness collisions re-enter "Inactive", which is
  not digressive, so the 150 timer dies at once (rule 2).

### Checked and not bugs

- NGOAL vs SGOAL: every difference mirrors the side (`PlayCrowdResponse` 3/4/5 vs 0/1/2,
  `IncRokValue` 8 vs 7, timer targets 7 vs 8).
- GRANDOM vs RRANDOM: the plans differ by design; RRANDOM's `ModifyPitch` arms are
  consistent.
- INSCRTBL sends `YerAttacked` data 4 for both goalies. The data is the damage kind; the
  attacker comes from the sender. Slot 23 maps to the opposing goalie by parity in
  `TakeDamage` (Ghidra 0x004657b0).
- COACH "CoachTheTeam" is reached through rule 4, so the coach AI runs.
- Collision interactor argument order does not matter; the test is a sphere overlap.

## Findings in the team ratings files

### How the parser reads a line

`GameEngine::ParseObjectDefinition` (its command table is `s_apszCommands`, 0x004c7078) maps
`SetRatings1/2/3` to level slots 0/1/2 and `SetGoalieRatings1/2/3` and
`SetSpecialGoalieRatings` to goalie slots 0-3. An unknown command skips the whole line.
`Cmd_SetRatings` (`gameengine.cpp`) loops over `keyword = value` pairs and stops the
line (`break`) when:

- the keyword is not in the table, or
- the next non-blank character after the keyword is not `=` (`ParseUtil::SkipToChar` gives up
  at the first other character).

Keyword matching ignores case. Level slot 0 is easy (`DIFFICULTY_EASY = 0`), 1 medium, 2 hard.
Network games always use slot 0 (`GameEngine::LoadTeamFromFile`). A new player row starts at
0.0, so a lost rating reads as 0, not the `InitDefaults` 5.

I ran a simulation of these rules over all 14 `.HTM` files.

### Lost ratings

| File:line | Player | Level | Bad text | Ratings lost (intended value) |
|---|---|---|---|---|
| JAMCA:113 | VooDoo | easy | `id_RAT_RokReceiving` | RokHandling (10), ArmoryRange (5); MultiPointSpread never set |
| JAMCA:142 | VooDoo | medium | `id_RAT_RokReceiving` | RokHandling (10), ArmoryRange (50); MultiPointSpread never set |
| JAMCA:350 | Oaxaca | easy | `id_RAT_RokReceiving` | RokHandling (7), ArmoryRange (50); MultiPointSpread never set |
| JAMCA:379 | Oaxaca | medium | `id_RAT_RokReceiving` | RokHandling (7), ArmoryRange (50); MultiPointSpread never set |
| NEM:13 | Ravage | easy | `id_RAT_Stealing` | ArmoryRange (50); RokHandling never set |
| NEM:109 | Ruckus | easy | `id_RAT_RokReceiving`, `id_RAT_Stealing` | ArmoryRange (50); MultiPointSpread and RokHandling never set |
| NEM:137 | Ruckus | medium | `id_RAT_RokReceiving` | RokHandling (10), ArmoryRange (50); MultiPointSpread never set |
| NEM:250 | Rage | easy | `id_RAT_Stealing` | ArmoryRange (9); RokHandling never set |
| NEM:345 | Riot | easy | `id_RAT_RokReceiving`, `id_RAT_Stealing` | ArmoryRange (9); MultiPointSpread and RokHandling never set |
| LOSAN:79 | J._Vanacher | hard | `id_RAT_Speedburst4b 0 1` | Speedburst4b (unclear, 0 or 1) |
| LOSAN:110 | Martinez | easy | `d_RAT_SubInDamage` | SubInDamage (50) |
| SEATTLE:178 | Kariotis | hard | `id_RAT_Pass` | Pass2 (100) |
| JAMCA:205-210 | Hedley_Salesse (goalie) | easy, special | command `SetSpecialRatings` | the whole special goalie set |

`RokReceiving` and `Stealing` are old names. They sit where the other files put
`MultiPointSpread` and `RokHandling`, and the values match the same players' hard rows
(VooDoo 7, Oaxaca 4, Ravage 10). Smallest fix: rename `id_RAT_RokReceiving` to
`id_RAT_MultiPointSpread` and `id_RAT_Stealing` to `id_RAT_RokHandling`, which also restores
the ratings after them on the line.

### Omitted lines (no typo)

NEM's easy blocks use an older template. Ravage, Ruckus and Riot have no `SubOutDamage`,
`SubInDamage`, `MultiRange` or `MinSubIn`; Rage lacks `SubOutDamage` and `SubInDamage`. All
read as 0. SubOutDamage 0 matches LOSAN's and SEATTLE's explicit easy value, so only
SubInDamage and MinSubIn (0 against 50 and 40 elsewhere) differ. Fix: add the two lines from
any other team's easy block.

### Effect in game

- **RokHandling 0: noticeable.** `JostledDropRok` (`plancond.cpp`) drops the rok when
  `rand() % 100 > (damage * base / 100 + base) / 2`, with `base = weighted RokHandling * 10`.
  At handling 10 and full form the threshold is at least 50, and 100 at full health, so the
  carrier rarely drops it. At 0 the threshold is 0, so the carrier drops it on about 99% of
  jostles. It hits JAMCA's VooDoo and Oaxaca on easy and medium, NEM's Ruckus on easy and
  medium, and all four NEM skaters on easy. It also applies in every network game. Handling
  also scales `RokInRange` (0.65x instead of about 1.15x at 10).
- **ArmoryRange 0: moderate.** The coach's weapon check (`EngineObject::CoachUpdate` in
  `coachai.cpp`) needs the distance to the swarf carrier to be within ArmoryRange, so these players never react to an
  opponent with the swarf.
- **MultiPointSpread 0: mild.** `CheckHandicapCondition` (`coachai.cpp`) sends a
  carrier to a multiplier when its team trails by more than the spread. At 0, any deficit
  triggers it, against 4-10 for teammates.
- **Pass2 0 (Kariotis, hard): mild.** In `PassOrRunByRatings` (`plancond.cpp`) the "a
  defender is within 10 units of the receiver" test never fires for him.
- **SubInDamage 0 (Martinez, easy): little.** A benched Martinez is fit to return at once.
  On easy LOSAN never subs out by damage, so it only matters after an injury.
- **Speedburst4b (J._Vanacher, hard): none to slight.** Two of three teammates also have 0.
- **JAMCA special goalie: mild.** At easy, offline, the home goalie (slot 4) uses the special
  set when it exists (`PlayerObject::PlayerObject`, 0x00474150). JAMCA's goalie keeps its easy
  set instead: Intelligence 40 instead of 20, CatchingAbility 4 instead of 8. Fix: rename the six lines to
  `SetSpecialGoalieRatings`, as in the other 13 files.

### Other oddities

- TOKYO defines two skaters named `Hashimoto` (lines 3 and 100). No ratings are lost; the
  roster shows the name twice.
- MSCOW:80 sets Kovalick's hard `MultiRange` twice, with the same value. Harmless.

## Engine faults seen on the way

These are in the engine, not the data. They're recorded so nobody has to find them again:

- `ModifyOpposTeamFriction` restores from the target's cache entry (finding 1).
- `SelectSubstitute` reads the first candidate's `SubOutDamage` for both candidates
  (`coachai.cpp`; the source comment already says so).
- `RokScoreValue` cannot return 4 (finding 5).
