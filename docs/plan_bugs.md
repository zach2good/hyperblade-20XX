# Plan bugs

These are bugs in the shipped plan data and the team ratings files of HYPERX.EXE v1.0.0.1. The
fixes are suggestions only: neither the game data nor this repository applies them. Each finding
survived an adversarial check against the original's code. None has been watched in the game, so
"Confirmed" means the code path is certain and the in-game effect is still an inference.

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

## Summary

| Plan | Bug | Effect | Fix |
|---|---|---|---|
| CARRYROK "PassingPlay" | The re-armed timer has no receiver | After one failed check the carrier tests slot 0 as its receiver | Add `id_PRN_MyTeammate` to `SET_TIMEOUTMSG 50` |
| CARRYROK "GoForGoal" | A collision keeps 8 bits of the decision log, so a second `ShootSituation` branch of 1 replays as 4 | In a network match another player's copy of the AI carrier may briefly enter a stale "PassingPlay" or "Throwing" | Give `ShootSituation` five arms, arm 4 a copy of arm 1 |
| DEFENSE | Six collision interactors name opponents by slot, not by collision object | Attacks never react to dromers 2 and 3 | Use `id_PRN_LeadOppnBodyBub` and `id_PRN_OtherOppnBodyBub`, as "ManOnMan" does |
| MULTPLR "Arming", "ScoreArming" | Timer and `Initiate` carry data 0 where the hitter's slot is read | Spins take their sign from dromer 0, and dromer 0 can be bounced from anywhere | Add `_MESSAGE_DATA_` to both timers; send `Extra0` from the `YerHit` handler |
| TURNSTIL | The same data-0 pattern | Extra spin impulse from slot 0, and slot 0 can be bounced | Drop the `Initiate` `ReallySendMsg`; add `_MESSAGE_DATA_` to the timer |
| GOALIE "Flying" | The timer sends `id_MSG_TimeOutMessage`, which "Flying" does not handle | An airborne goalie without the rok is not kicked down | Send `id_MSG_Extra1`, as "SaveFromGoal" does |
| GRANDOM "Inactivate" | `ModifyOpposTeamFriction` copies one cached Speed to both slots of a pair | All four skaters get slot 0's Speed 150 ms into each period, after each goal and each green expiry | Engine fix only: write each slot's own cached value |
| GRANDOM "Inactivate" | Blanket resets of long pass, long shot and immunity for slots 0 to 3 | Seattle's built-in long pass is lost every period | Remove the resets; send the plan's unused "End..." messages from timers |
| RRANDOM "Active" | `REMOVE_AGENDA_ITEM` kills the 45 s `RedIsUp` timer | The 2.0 speed boost lasts until a green randomizer resets it | Delete that `REMOVE_AGENDA_ITEM` |
| ROK "BeingCarried" | A refused throw's `_NO_BEH_CHANGE_` sticks | Later throws that period leave the rok hanging in the air | Add `CHANGE_TO_BEHAVIOR 3 "Flying"` to the throw arm |
| NGOAL, SGOAL | `_NO_BEH_CHANGE_` in the own-goal arm sticks | Later goals into that net skip the post-goal sequence | Use `CHANGE_TO_BEHAVIOR 1 "PostScoreWait4Drag"` (8 places) |
| Team files | Ratings parser stops a line at the first unknown keyword | JAMCA and NEM players with `id_RAT_RokReceiving` or `id_RAT_Stealing` lose RokHandling and drop the rok on about 99% of jostles | Rename to `MultiPointSpread` and `RokHandling` |
| Team files | `SetSpecialRatings` is not a command | JAMCA's special easy goalie ratings are ignored | Rename to `SetSpecialGoalieRatings` |
