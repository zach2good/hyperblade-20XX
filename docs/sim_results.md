# Simulated matches

`-sim` (see the [README](../README.md#simulating-matches)) played every team against every other
team on three builds of the AI:

- **Shipped:** the plans as shipped.
- **First fixes:** the fixes from the first review in [plan_bugs.md](plan_bugs.md).
- **Fixed:** those plus the second review's fixes, as the port now plays.

All three are compared with the team ratings the menus show.

## Method

- Every ordered pair of the 14 teams, home and away, so 182 matchups. Each match uses the home
  team's drome.
- Shipped and Fixed played 5 matches per matchup, 910 matches each. First fixes played 50 per
  matchup, 9,100 matches.
- The `booie.ini` defaults for difficulty and period length.
- Every run includes the fix for the rok's glide speed cap. Before it, `CGlideOnSurface` read the
  rok's speed rating from past the end of the rok, and some matches ended in "BAD SURFACE NORMAL".
- Points % counts a win as 1 and a draw as a half.
- "Card" is the average of the team's ratings on the menu cards, and "card rank" orders the
  teams by it.

With 130 matches per team, a points % in the 910-match runs moves by about 4 between runs, so
read small differences as noise.

## How matches change

| | Shipped | First fixes | Fixed |
| --- | --- | --- | --- |
| Points scored per match, both teams | 6.5 | 13.5 | 11.7 |
| Draws | 13.2% | 4.5% | 6.3% |
| Matches won by 10 or more | 5.4% | 36.2% | 27.8% |
| Shots per team per match | 0 | 11.5 to 23.4 | 11.9 to 24.2 |
| Matches won by the team with more possession | 67.1% | 79.4% | 76.4% |
| Home wins against away wins | 418 to 372 | 4,242 to 4,446 | 398 to 455 |

On the shipped plans no AI skater takes a shot. The shots stat stays at 0 in every match, and
every point comes from something other than a shot, as [plan_bugs.md](plan_bugs.md) predicts.
The first fixes double the scoring and turn a third of matches into blowouts. The second review
takes some of that back. The likely causes are the bounce clamp, which stops bodies rising thirty
times too fast when they change plan, red randomiser boosts no longer outliving a goal, and the
free rok no longer restarting every AI skater while a dromer lies on it. No run separates them.

## Ranking

| Team | Card | Card rank | Shipped pts% | Rank | First fixes pts% | Rank | Fixed pts% | Rank |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| BLAM! Nemesis | 4.78 | 1 | 90.4 | 1 | 98.8 | 1 | 98.1 | 1 |
| The DredNoks (Jamaica) | 3.84 | 2 | 63.8 | 3 | 74.1 | 3 | 73.5 | 2 |
| The Knights (Leeds) | 2.88 | 8 | 54.6 | 5 | 75.0 | 2 | 73.1 | 3 |
| The Machine (Chicago) | 2.53 | 14 | 63.8 | 2 | 69.5 | 4 | 70.4 | 4 |
| The Ambush (Warsaw) | 2.91 | 6 | 58.1 | 4 | 66.3 | 5 | 56.5 | 5 |
| The Fury (Seattle) | 2.75 | 12 | 44.2 | 8 | 44.4 | 7 | 45.4 | 6 |
| The Crusade (Toronto) | 2.72 | 13 | 42.3 | 11 | 45.7 | 6 | 42.3 | 7 |
| The T-34s (Moscow) | 2.75 | 11 | 48.5 | 6 | 31.3 | 11 | 41.5 | 8 |
| The Ronins (Tokyo) | 2.94 | 5 | 42.3 | 10 | 35.9 | 10 | 40.8 | 9 |
| The Shockwave (Los Angeles) | 3.16 | 3 | 36.2 | 12 | 26.0 | 13 | 37.3 | 10 |
| The LoneStars (Houston) | 2.88 | 7 | 45.4 | 7 | 39.0 | 9 | 35.0 | 11 |
| The Glory (New York) | 2.78 | 10 | 43.1 | 9 | 41.6 | 8 | 31.5 | 12 |
| The BlackJack (Las Vegas) | 2.81 | 9 | 32.3 | 14 | 30.5 | 12 | 31.2 | 13 |
| The Sturmnacht (Munich) | 2.94 | 4 | 35.0 | 13 | 21.9 | 14 | 23.5 | 14 |

- All three agree on the shape. Spearman's rank correlation between Fixed and First fixes is
  0.90, and between Fixed and Shipped 0.86. The same five teams fill the top five on every
  build, and Munich finishes last or second last. The middle eight sit within a few points of
  each other and reshuffle from run to run.
- The menu cards predict none of them. Card rank against results is 0.09 shipped, 0.00 with the
  first fixes and 0.10 fixed. The two starters' cards alone give 0.35, 0.20 and 0.26.
- The Knights gain the most from the fixes, from 5th to 3rd. They score through the 4x
  multiplier, which only pays once the AI shoots.
- The Machine has the worst card and finishes 2nd to 4th on every build, so its strength does
  not come from the fixes.
- BLAM! Nemesis is far ahead on every build, and loses 2 of 130 matches with the fixes.

## Which ratings matter

Rank correlation of each team rating with points %:

| Rating | Shipped | First fixes | Fixed |
| --- | --- | --- | --- |
| Strength | 0.60 | 0.53 | 0.50 |
| Weight | 0.42 | 0.41 | 0.37 |
| Stamina | 0.26 | 0.27 | 0.49 |
| Rok handling | 0.32 | 0.45 | 0.26 |
| Shot power | 0.13 | 0.24 | 0.23 |
| Acceleration | 0.13 | 0.12 | 0.21 |
| Jumping | 0.09 | -0.03 | 0.03 |
| Speed | -0.01 | -0.06 | 0.00 |

Strength and weight decide matches on every build. Speed and jumping never do, but they count as
much as the others on the cards, which explains most of the gap between cards and results.

## Multiplier goals

With the fixes the top teams score through the 4x multiplier:

| Team | 1x goals | 2x goals | 4x goals | Points per shot |
| --- | --- | --- | --- | --- |
| BLAM! Nemesis | 0.9 | 0.4 | 3.4 | 0.71 |
| The Machine | 0.6 | 0.2 | 2.0 | 0.76 |
| The Knights | 0.6 | 0.2 | 2.0 | 0.64 |
| The DredNoks | 1.4 | 0.6 | 0.8 | 0.27 |
| The Shockwave | 1.1 | 0.3 | 0.3 | 0.24 |

Goals are per team per match, from the Fixed run. The DredNoks take the most shots in the
league, 24.2 a match, but few of them go through the multiplier.

## Period length

The options menu offers periods of 3, 5 and 7 minutes. Each length played its own 910-match
round robin on the fixed plans, all on the same build.

| | 3 minutes | 5 minutes | 7 minutes |
| --- | --- | --- | --- |
| Points scored per match, both teams | 5.2 | 11.5 | 16.8 |
| Points per period | 1.7 | 3.8 | 5.6 |
| Draws | 16.4% | 5.4% | 3.2% |
| Matches won by 10 or more | 5.9% | 28.9% | 49.3% |
| Matches won by the team with more possession | 66.6% | 76.1% | 84.4% |
| BLAM! Nemesis points % | 90.8 | 97.7 | 99.6 |
| Card rank against results, starters' cards | 0.51 | 0.33 | 0.19 |

Scoring grows faster than the clock. A straight line through the points per period reaches nought
at about 1.2 minutes, so roughly the first minute of every period produces no points, whatever its
length. Short periods are therefore far more even: a third of 3-minute matches end drawn or by one
point, and nearly half of 7-minute matches are won by 10 or more.

| Team | 3 min pts% | Rank | 5 min pts% | Rank | 7 min pts% | Rank |
| --- | --- | --- | --- | --- | --- | --- |
| BLAM! Nemesis | 90.8 | 1 | 97.7 | 1 | 99.6 | 1 |
| The DredNoks (Jamaica) | 81.5 | 2 | 75.0 | 3 | 64.6 | 5 |
| The Ambush (Warsaw) | 63.5 | 3 | 63.8 | 5 | 65.4 | 4 |
| The Machine (Chicago) | 51.9 | 4 | 71.5 | 4 | 74.2 | 3 |
| The Knights (Leeds) | 50.0 | 5 | 78.1 | 2 | 81.9 | 2 |
| The T-34s (Moscow) | 49.2 | 6 | 40.4 | 7 | 28.5 | 12 |
| The Crusade (Toronto) | 45.8 | 7 | 46.2 | 6 | 54.2 | 6 |
| The BlackJack (Las Vegas) | 45.0 | 8 | 28.5 | 12 | 31.2 | 11 |
| The LoneStars (Houston) | 40.8 | 9 | 38.1 | 8 | 40.8 | 8 |
| The Glory (New York) | 40.4 | 10 | 38.1 | 9 | 40.4 | 9 |
| The Shockwave (Los Angeles) | 38.5 | 11 | 28.1 | 13 | 23.1 | 13 |
| The Ronins (Tokyo) | 38.5 | 12 | 37.3 | 10 | 35.0 | 10 |
| The Sturmnacht (Munich) | 35.0 | 13 | 21.9 | 14 | 17.7 | 14 |
| The Fury (Seattle) | 29.2 | 14 | 35.4 | 11 | 43.5 | 7 |

- Neighbouring lengths agree at 0.89 (Spearman); 3 and 7 minutes agree at only 0.72.
- Defence wins short matches. The DredNoks concede 0.6 a match at 3 minutes and finish 2nd, but
  end 7-minute matches with the most injured squad among the top five (health 27) and drop to
  5th.
- Scoring wins long ones. The Knights and The Machine, who score through the 4x multiplier,
  climb from 5th and 4th to 2nd and 3rd. Seattle climbs from last to 7th.
- The T-34s and The BlackJack do best in short matches and fade as the minutes add up.
- Strength matters at every length. Rok handling and jumping matter most at 3 minutes, and weight
  at 7.

## Home advantage

Away teams win slightly more often in the First fixes and Fixed runs, but each match is played
in the home team's drome, so arena and home side are mixed up. To separate them, each team played
itself 100 times in Toronto's drome, the arena with the fewest multipliers and randomisers, on the
first fixes. Home won 639 and away won 651, with 110 drawn: 49.5% of decided matches, p = 0.74.
Being the home team makes no difference, so the gap comes from the dromes.

## Match stats

In the shipped game, 26 of the per-player match stats are 0 in every simulated row. No plan or code
writes 25 of them: completed passes, interceptions, checks given, drags won, power play and
short-handed stats, 2-point stats, rail time and hang time. The 26th, freestyle moves, is written
only by the Human plan's rail slide, and no AI plan slides a rail.

Two stats that are written are wrong. Assists equal goals in 99.7% of rows: the plans credit the
previous rok carrier, and that pronoun answers the current one, the scorer. One-timers counted 7 of
8 goals, because the test asked whether the scorer released the rok within a second of the goal.

The port records or corrects these. Scores and every other stat are unchanged.

| Stat | The port's rule | Seeded run |
| --- | --- | --- |
| Passes completed, interceptions | The next possession within 3 seconds of a pass: a teammate's completes it, an opponent's intercepts it | DredNoks against Knights, 5 matches: 66% of 335 passes completed, 6% intercepted |
| Assists | The teammate whose completed pass last reached the scorer, within 15 seconds | Nemesis against Sturmnacht, 10 matches: 15 of 30 goals |
| One-timers | The scorer, when the shot left within a second of receiving that pass | Same run: 3 of 30 goals |
| Checks given | Every attack that reaches an opponent | Same run: Sturmnacht 753, Nemesis 647 |
| Time on rail | Each frame a dromer spends on a rail | 0 with no person playing |
| Hang time | Each frame a dromer spends flying: jumps and being thrown | Nemesis against Sturmnacht: 9.7 s a dromer a match, 40.9 s at most |
| Drags won | The first dromer to the rok after a drag starts: each period's start and after each goal | Same run: 56 drags in 10 matches, 27 to Nemesis, 29 to Sturmnacht |

The Sturmnacht gave more checks than Nemesis but caused 1 injury or fatality to Nemesis's 21.

The Injuries and Fatalities columns count harm a player inflicts, not harm taken: the hurt dromer's
plan credits its attacker, or the owner of the mine, rok or turnstile that hit it.

## Balance proposal: BLAM! Nemesis

BLAM! Nemesis wins about 9 of 10 matches against every other team. As the Gauntlet's last
opponent it should stay on top, but be beatable. A newer run than the tables above, with the port's
AI, 3-minute periods, seed 1 and 10 matches per matchup, measured its points % against the other 13
teams, home and away.

Setting one Nemesis rating at a time to The Machine's median moves it by under 4 points. Checking
does most, from 92.1 to 88.3, then weight class to 89.4. Its goalie ratings (`id_GRT_*`) and its
goalie's motion speed (120, against 80 for most teams) do nothing. No single rating carries the
lead.

So the proposal pulls every skater rating toward The Machine's median for the same difficulty by
one factor k: `median + k * (nemesis - median)`, rounded.

| k | Nemesis pts% |
| --- | --- |
| 1 (shipped) | 92.1 |
| 0.5 | 82.7 |
| 0.25 | 73.1 |
| 0 | 56.7 |

k = 0.5 puts Nemesis at The DredNoks' level and keeps it first. On medium, its median checking goes
from 12.5 to 9, and stamina from 11 to 7. With 260 matches a point, each figure is good to about 2
points.

At k = 0 Nemesis lands near The Machine's 53.7%, and The DredNoks given the same treatment take
57.5%. The whole lead is in the ratings; models, goalies and dromes add nothing measurable.

The two teams are also the only ones whose files use `id_RAT_Stealing` and `id_RAT_RokReceiving`.
The original stopped reading a line at either name. The port reads them as `id_RAT_RokHandling`
and `id_RAT_MultiPointSpread`, so any shrink must change those lines too. Reading them as the
original did, cut at the name, leaves Nemesis where it is (94.6%) and drops The DredNoks from
83.1% to 77.5%, a lift for The DredNoks just inside the noise of two such runs.

The port ships no game data, so this stays a proposal.

### In the Gauntlet

The Gauntlet barely meets Nemesis. Its bracket is 13 rounds, and a loss, or a draw with any
opponent still standing, ends the season. Over 20 simulated seasons each, The DredNoks, The
Knights, The Machine and The Ambush averaged 2.3 to 4.8 matches a season, and only one season in
80 reached Nemesis, and 35 of the 80 ended on a draw, 13 of The DredNoks' 20. Draws are 15% of
these 3-minute-period matches, against 6.3% in the Fixed round robin's 5-minute ones. With
Nemesis shrunk, The DredNoks won one title in 20; no other team's seasons changed. At a 70% win rate, 13 straight wins come once in a hundred seasons, so the format, not
Nemesis, is what keeps AI teams from titles. A person playing the home team is another matter, and
untested.

## Balance proposal: the bottom teams

The same measure (the port's AI, 3-minute periods, seed 1, 10 matches per matchup against the other
13 teams, home and away) puts The Sturmnacht at 33.7% and The LoneStars at 38.1%.

Pulling their ratings toward The Machine's per-rating medians makes them worse, down to 25.6% and
25.0%: four identical median dromers are a weaker team than The Machine's real roster. Copying The
Machine's rating lines row for row gives 47.5% and 47.3%, near The Machine's own 53.7%. So the
proposal blends each rating line toward its counterpart in The Machine's file:
`machine + k * (team - machine)`, rounded.

| k | The Sturmnacht pts% | The LoneStars pts% |
| --- | --- | --- |
| 1 (shipped) | 33.7 | 38.1 |
| 0.5 | 30.8 | 43.8 |
| 0.375 | 47.5 | |
| 0.25 | 47.9 | 46.9 |
| 0 | 47.5 | 47.3 |

The LoneStars need nothing: 38.1% is within noise of the middle of the table. The Sturmnacht take
k = 0.375, which puts them mid-table. Their share rises steeply between k = 0.5 and 0.375 and
is flat below. No single rating carries the rise: of the 39 that change between those two blends,
any half lifts them only part of the way (29.6% to 42.9% from 30.8%), so it builds up across many.

The models, the goalies and the drome don't explain the gap. With The Machine's models The Sturmnacht
take 27.1% at the median ratings, their goalie is The Machine's size, and they lose at home (22.7%)
as well as away (28.5%).

## Same matches everywhere

A seed now plays the same matches, byte for byte, with MSVC on Windows, GCC and Clang on Linux,
and in the browser build. Clang builds used to differ: one shipped animation clip holds a frame
whose X and Z angles are NaN, and packing that frame kept the NaN's sign, which each compiler
picks differently. The rok jumped for that frame, a different distance per compiler. The frame
now holds the previous frame's angles, so the jump is gone and every build agrees.

Re-measured after the animation fix, the proposals hold within noise: BLAM! Nemesis 93.5%
shipped and 83.1% at k = 0.5, The Sturmnacht 30.8% shipped and 46.7% at k = 0.375.

## Not covered

- These runs predate exact replay. A seed now replays the same matches, so later runs can repeat a
  single match, but the figures here would shift slightly if rerun.
- Only the default difficulty was played. The sections other than "Period length" use 5-minute
  periods.
- The Shipped and Fixed runs are small. A full 9,100-match run of each would firm up the middle
  of the table.
- The mirror matches were not repeated after the second review.
