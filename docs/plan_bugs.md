# Plan bugs

These are bugs in the shipped plan data and the team ratings files of HYPERX.EXE v1.0.0.1. Each
finding survived an adversarial check against the original's code. None has been watched in the
game, so "Confirmed" means the code path is certain and the in-game effect is still an inference.

The port fixes them, and the debug menu's Plans tab (F2) turns the fixes off and on:

- Each fix is a unified diff in `res/plans` against the plan's source as
  [libadlib](https://github.com/vomitselfie/libadlib) decompiles it. The diffs go into the pak.
  When a plan loads, the engine reads it from the disc, decompiles it, checks that it recompiles
  to the same words, applies the diff and compiles the result in memory. A plan whose diff does
  not apply runs as shipped. No plan source is committed beyond the diffs' context lines.
- Every object compiles both sets of plans at load. Switching restarts each object's current
  behaviour from the other set. A network session cannot switch.
- `PlanFixes::On()` gates the engine faults: animation events on glide behaviours, the speed
  restore, the power-up resets, three-leaf avoidance, `SelectSubstitute`, the bounce clamps and speed cap, the goalie's `LaunchAt`, `CSteerToward`'s double step, a computer team sent to Human, and the ratings parser. The team file fixes apply when the next
  match loads its teams.

The AI runs from the plan files in `Data/PLANS`, scripts that the plan VM in `src/planvm/`
interprets (see the [glossary](README.md#glossary)). `tools/plan_dump.py` prints them as readable
pseudo-source, with the developers' own names from `ENUMIDS.H`:

```sh
python tools/plan_dump.py CARRYROK --data <path to Data>
```

Add `--kinds` to have the dumper comment every timer that carries data 0 and every slot pronoun
used as a collision object, and `--decisions` to walk each handler's DECIDE branches.

| Document | Covers |
|---|---|
| [plan_bugs_skaters.md](plan_bugs_skaters.md) | OFFWING, DEFENSE, CARRYROK, GOALIE, HUMAN, INPUT, SIDELN, LISTEN, MULTPLR, MINIMAL and CRSHDUM0 |
| [plan_bugs_objects.md](plan_bugs_objects.md) | The object plans, the decision log, and the team ratings files |

Ratings used in both:

- **Confirmed**: the data flow is certain from the plan bytes and the VM code.
- **Probable**: the data flow is certain, and the intent is clear enough from sibling code.
- **Odd**: the plan does something unusual that may be deliberate.

To write a new fix, decompile the plans, edit a copy and diff it:

```sh
planpatch_check <Data folder> res/plans <output folder>
diff -U2 --label a/DEFENSE --label b/DEFENSE <output folder>/DEFENSE.txt DEFENSE.txt > res/plans/DEFENSE.diff
```

`planpatch_check` writes every plan's decompiled source and every fixed plan to the output
folder. Add the diff to `HYPERX_PAK_FILES` in `CMakeLists.txt`. The author writes every child
count in ADLIB source, and the compiler does not check them, so a dump of the fixed `.PBN` with
`plan_dump.py` is the way to see that a fix landed where it should.

## Plan VM rules

These rules decide most of the findings, and each is easy to miss in a dump. The findings refer
to them by number.

1. **A behaviour change inside a handler is sticky.** `MsgInteractor::Setup` (0x0045dfc0) and
   `CollInteractor::Setup` (0x0045dd00) set the node's `m_nBehaviour` once, from the
   `CHANGE_TO_BEHAVIOR` that sits before `Enable`. A `CHANGE_TO_BEHAVIOR` inside the `Enable`
   block overwrites that field (`Interactor::ExecuteScript`, 0x0045d860), and `Dispatch` and
   `Process` never reset it (0x0045e0d0, 0x0045ddf0). Nodes live as long as the plan instance,
   which is one period (`GameEngine::RestartMatch` reinstantiates every plan). So an arm that
   writes `CHANGE_TO_BEHAVIOR _NO_BEH_CHANGE_` cancels the header's change for that firing _and
   every later one_ in the period.
2. **Entering a non-digressive behaviour cancels every pending timer.** Timers belong to the
   object, not the behaviour. `GameBehaviorObject::TickAndSortMessages` (0x0041c380) detaches
   every agenda item that is ready or not persistent, unless the new behaviour sets
   `id_SET_Digressive`, and `CTimerActivity::GetReadyFlag` (0x00413ef0) always answers one. Only
   a digressive target, such as "CompleatAnimation", inherits a timer, and handles it if it fires
   there. `CHANGE_OF_PLAN` kills them too: `InstantiateAIPlan` (0x00471da0) calls
   `PlanState::Teardown` (0x0047b900), which releases every agenda item.
3. **`REMOVE_AGENDA_ITEM id_AGD_CTimerActivity` cancels all of the object's timers**, not only
   the one that just fired (`GameBehaviorObject::RemoveAgendaItem`, 0x0041c5f0).
4. **A behaviour with no interactors passes control to the next one**
   (`GameBehaviorObject::CanPassOn`). This is how COACH reaches "CoachTheTeam".
5. **A static pronoun is resolved once.** A pronoun such as `id_PRN_MyTeammate` becomes a plain
   number when the plan is instantiated for an object, through `g_apPlanArgFns` in
   `PlanState::ParseNodeStream` (0x0047b430). `id_PRN_MyLeadOpponent` becomes slot `~me & 1` and
   `id_PRN_MyOtherOpponent` becomes `(~me & 1) | 2` (`PlanArg::GetMyLeadOpponent` 0x0047c7d0,
   `GetMyOtherOpponent` 0x0047c7f0).
6. **Message data is 0 unless a node supplies it.** `SET_TIMEOUTMSG` stores data 0 unless the
   node has a fourth leaf (`CTimerActivity::BindWithNode`, 0x00411a40). `SendMessage` and
   `ReallySendMsg` forward the incoming data unless they have their own fourth leaf
   (`PlanAction::SendMsg` 0x0045e6a0, `ReallySendMsg` 0x0045e5d0). `id_MSG_Initiate` always
   carries data 0 (`GameBehaviorObject::SendTimedMessage`, 0x0041c320).
7. **A collision interactor's two leaves are collision object ids.** `CollInteractor::Setup`
   (0x0045dd00) stores them, and `GameEngine::DispatchCollisionToRule` (0x004868f0) indexes
   `m_ppColObjects` with them. Dromer `d` owns objects `3d` (`id_COB_Dromer_d`), `3d + 1`
   (`id_COB_BatonEzone_d`) and `3d + 2` (`id_COB_DromerBody_d`).

## A compiler fault

**The compiler's argument counters carry over from one compile to the next.**
`PlanVM::EmitArgToken` (0x0047e4d0) finds a behaviour name, a plan name or an interactor name by
counting the tokens since `DECLARE_BEHAVIOR`, `CHANGE_TO_BEHAVIOR`, `CHANGE_OF_PLAN`,
`SET_INTERACTOR_PARAM` and `DECLARE_LOCAL_REALS`. The five counters are globals (0x004c6ab8 to
0x004c6ac0) that start at 100 and go back to 0 only at their own keyword; no compile resets them.
So after a source that ends within three tokens of one of those keywords, the next source compiled
in the same run reads its first tokens as that keyword's arguments. `PlanEditorDialog::OnCompileAll`
(0x0047c1a0) is the one path that compiles many sources in a run. The game ships its 32 plans
compiled, with no `.txt` sources, so the fault cannot fire in a retail install. The nested compile
for `CHANGE_OF_PLAN` only prescans the target plan, which does not touch the counters.

## The pattern

**AI carriers pass instead of shooting.** In `CARRYROK.PBN`, the "GoForGoal" behaviour arms its
shot timer with three arguments:

```text
SET_TIMEOUTMSG 15, id_MSG_TimeOutMessage, id_PRN_Me
```

The missing fourth argument reaches the timer as 0, and `SendMessage(id_PRN_Me, id_MSG_Extra2)`
passes it on. `RecvrInRangeNView` then reads it as the pass receiver's slot, so the carrier tests
slot 0, the human player, in place of its teammate. When the human is close, the carrier passes
where it would shoot. This hits the two opposing carriers at every difficulty. The same plan's
"PassingPlay" sets the receiver correctly:

```text
SET_TIMEOUTMSG 50, id_MSG_Extra2, id_PRN_Me, id_PRN_MyTeammate
```

Fix: add `id_PRN_MyTeammate` as the fourth argument of GoForGoal's `SET_TIMEOUTMSG 15`. This adds
one word to the node, so the node's argument count goes from 3 to 4. Adding it as a third argument
to `SendMessage(id_PRN_Me, id_MSG_Extra2)` works too. PassingPlay repeats the slip once, in its
re-armed `SET_TIMEOUTMSG 50, id_MSG_TimeOutMessage, id_PRN_Me`.

Fixed: `res/plans/CARRYROK.diff` adds the fourth leaf to both timers.

## Summary

| Plan | Bug | Effect | What the port does |
|---|---|---|---|
| CARRYROK "Throwing", "PassingPlayPass" | Only `BehObj_Fly`, `BehObj_FlyLcl` and `BehObj_OpenAndClose` bind `id_SET_AnimationEvent` when they start. These two are `id_BEH_Glide`, and the throw animation's own trigger, "release", names no message | AI carriers wind up, release nothing and change plan still holding the rok, so they pump fake and never pass or shoot. HUMAN's "Throwing" works because it also binds the event with `DO_ACTION id_ACF_SetAnimationEvent` | Engine: `GameBehaviorObject::SendTimedMessage` binds the event after starting the animation, as those three do |
| CARRYROK "GoForGoal", "PassingPlay" | Two receiver timers carry data 0 | AI carriers test slot 0 as their receiver and pass where they should shoot | Adds `id_PRN_MyTeammate` to both timers |
| CARRYROK "GoForGoal" | A collision keeps 8 bits of the decision log, so a second `ShootSituation` branch of 1 replays as 4 | In a network match another player's copy of the AI carrier may briefly enter a stale "PassingPlay" or "Throwing" | Gives `ShootSituation` five arms, 2 to 4 copies of arm 1 |
| DEFENSE | Six collision interactors name opponents by slot, not by collision object | Attacks never react to dromers 2 and 3 | Uses `id_PRN_LeadOppnBodyBub` and `id_PRN_OtherOppnBodyBub`, as "ManOnMan" does |
| DEFENSE | `id_MSG_SubstituteDromer` only runs with the clock going | A face-off substitute key never benches a healthy AI teammate | Swaps the `ClockStoppedSemaphore` arms |
| DEFENSE, OFFWING, CARRYROK | Three-leaf `CAvoidObjects` has no range, and its second target in DEFENSE and OFFWING is the camera | AI skaters never swerve round teammates or opponents | Rewrites the 17 DEFENSE and OFFWING items to `id_PRN_MyTeammate, 0, 80.0, -6.5`; the engine gives CARRYROK's three-leaf items `Bind`'s range and strength |
| MULTPLR "Arming", "ScoreArming" | Timer and `Initiate` carry data 0 where the hitter's slot is read | Spins take their sign from dromer 0, and dromer 0 can be bounced from anywhere | Adds `_MESSAGE_DATA_` to both timers; sends `Extra0` from the `YerHit` handler |
| TURNSTIL | The same data-0 pattern | Extra spin impulse from slot 0, and slot 0 can be bounced | Drops the `Initiate` `ReallySendMsg`; adds `_MESSAGE_DATA_` to the timer |
| GOALIE "Flying" | The timer sends `id_MSG_TimeOutMessage`, which "Flying" does not handle | An airborne goalie without the rok is not kicked down | Sends `id_MSG_Extra1`, as "SaveFromGoal" does |
| GRANDOM "Inactivate" | `ModifyOpposTeamFriction` copies one cached Speed to both slots of a pair | All four skaters get slot 0's Speed 150 ms into each period, after each goal and each green expiry | Engine: a slow-down saves each opponent's speed and the restore puts it back |
| GRANDOM "Inactivate" | Blanket resets of long pass, long shot and immunity for slots 0 to 3 | Seattle's built-in long pass is lost every period | Engine: a power-up saves the value it raises and a reset restores only that. End timers would not work: "Inactive" is not digressive, so entering it kills them (rule 2) |
| RRANDOM "Active" | `REMOVE_AGENDA_ITEM` kills the 45 s `RedIsUp` timer | The 2.0 speed boost lasts until a green randomizer resets it | Deletes that `REMOVE_AGENDA_ITEM` |
| ROK "BeingCarried" | A refused throw's `_NO_BEH_CHANGE_` sticks | Later throws that period leave the rok hanging in the air | Adds `CHANGE_TO_BEHAVIOR 3 "Flying"` to the throw arm |
| NGOAL, SGOAL | `_NO_BEH_CHANGE_` in the own-goal arm sticks | Later goals into that net skip the post-goal sequence | Uses `CHANGE_TO_BEHAVIOR 1 "PostScoreWait4Drag"` (8 places) |
| ARMORY | Display arms rely on the header after a sticky "Closed" | The armory can stay "Closed" for up to 5 s per weapon cycle | Adds `CHANGE_TO_BEHAVIOR 1 "Open"` to arms 0 and 1 |
| SWARF | The timer tells armory 52 to deploy over armory 53 | Armory 52 polls early for nothing | Sends to 53 |
| CAMERA "ViewSelfandOther" | `_NO_BEH_CHANGE_` in two arms sticks | The camera can stay in pass view | Names `CHANGE_TO_BEHAVIOR 2 "BasicOverTheShoulderView"`, which also undoes a sticky "PressCam" |
| Coach | `SelectSubstitute` reads the first candidate's `SubOutDamage` for both | The second candidate's limit is wrong | Engine: reads the second candidate's row |
| Team files | Ratings parser stops a line at the first unknown keyword | JAMCA and NEM players with `id_RAT_RokReceiving` or `id_RAT_Stealing` lose RokHandling and drop the rok on about 99% of jostles | Reads `RokReceiving`, `Stealing`, `d_RAT_SubInDamage` and `id_RAT_Pass` as `MultiPointSpread`, `RokHandling`, `SubInDamage` and `Pass2` |
| Team files | `SetSpecialRatings` is not a command | JAMCA's special easy goalie ratings are ignored | Reads it as `SetSpecialGoalieRatings` |
| Team files | NEM's easy blocks leave out `SubInDamage` and `MinSubIn` | Both read as 0 | An easy block without them gets 50 and 40, as every other team's has |

Not fixed:

- `RokScoreValue` cannot return 4, so a carried-in rok scores. Arm 4 is a sticky `_NO_BEH_CHANGE_`,
  so making it reachable would also break the post-goal sequence. The intent is unclear.
- LOSAN's `id_RAT_Speedburst4b 0 1`: the intended value is unclear.
- LIMBOPO's sends to pole 40, the dead handlers, and the "Looks odd" findings: no effect, or
  maybe deliberate.

## Second review

A later pass over every plan and the plan VM found these. Each was checked against the code; none
has been watched in the game. The fixes follow the toggle, except the crash guards and the port's
autopilot code, which always apply.

| Where | Bug | Effect | Fix |
| --- | --- | --- | --- |
| `CProjectAndBounce::Bind` | A rise faster than eighty a second is clamped to eighty divided by the frame delta (`fdivr` at 0x0040d3fb) | A body changing plan while rising jumps thirty times too fast; the rok's spikes come from here | Engine: multiplies by the delta |
| `CProjectAndBounce` | The speed cap is twice the forward speed, and SWARF and MINE define none | A launched swarf stops dead and sinks | Engine: no cap for a body with no forward speed |
| `ClosestTargetToProjectile` | With no carrier it uses the address of its result as a slot | A read far outside the slot table | Engine, always: answers the goalie, the arm that launches plainly |
| GOALIE `LaunchAt` | Divides by the held rok's speed, which is nought while the goalie stands still | The launch has no speed and the rok drops at the goalie's feet | Engine: uses the goalie's forward speed |
| `CSteerToward` | Runs before `LaunchToward` sets its target when the rok has no carrier | Null pointer | Engine, always: does nothing until it has a target |
| `CSteerToward` | Steps the next agenda item twice on arrival | The rok's and swarf's animation steps twice that frame | Engine: steps it once |
| DEFENSE | Finding 4's gate, swapped in the first pass, blocked the coach's substitutions in play | A damaged AI defender stays on until the next drag | Drops the gate |
| RRANDOM | A goal or drag during a red boost kills the timer that undoes it | Maximum speed stays at 2.0 until another red expires, all match in arenas without green randomisers | "Setup" restores 0.7 |
| ROK "GlidingFree", "Flying" | The knocked-down arm of each dromer collision re-enters the behaviour | A downed dromer on the rok rebroadcasts `RokIsFree` about every tenth of a second and restarts every AI skater | `_NO_BEH_CHANGE_` |
| ROK "BeingCarried" | Deploying an uncharged rok re-enters "BeingCarried" | Its exit turns the head camera off and the carry timers reset | `_NO_BEH_CHANGE_` for the uncharged arms |
| ROK | The goal-launch fallback and the throw count no shot on goal | A player can score with no shots and an infinite shooting percentage | Both count stat 40 |
| ARMORY "Open" | The awareness timer re-enters "Open" | The deploy poll restarts whenever a skater passes within range | The timer changes nothing |
| CARRYROK "GoForGoal" | The timeout's first arm has no behaviour change | Later shoot decisions run `Initiate` twice and play its sound twice | `_NO_BEH_CHANGE_` |
| OFFWING | The global `id_MSG_PassReceived` gives the receiver the keys when its team mate is hurt or due off | Right for the player's team, but a computer team's receiver reads the keyboard | Engine: a computer team's dromer sent to Human runs Defense, as autopilot does |
| Autopilot | Turning it on or off skips `kMsgExitBehavior` | A dromer moved mid-flight keeps its flying flag and skips the surface snap | Port: sends the exit first |
| Autopilot | Turning it off with both dromers benched hands Human a hidden dromer | The player steers an invisible dromer | Port: the AI keeps them |
