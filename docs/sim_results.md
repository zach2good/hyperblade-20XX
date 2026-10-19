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

## Stats the game never records

Of the per-player match stats, 26 are 0 in all 72,800 player rows of the First fixes run. Among
them: completed passes, checks and injuries given, interceptions, drags won, power play and
short-handed stats, 2-point stats, rail time, hang time and freestyle moves. Passes attempted
and injuries taken are counted, so the game either never wired the others up or the port loses
the calls. Assists equal goals in 99.7% of rows, which points to the scorer getting the assist.
Treat both as unverified.

## Not covered

- A seed does not yet replay a match exactly, so a single match cannot be repeated.
- Only the default difficulty was played. The sections other than "Period length" use 5-minute
  periods.
- The Shipped and Fixed runs are small. A full 9,100-match run of each would firm up the middle
  of the table.
- The mirror matches were not repeated after the second review.
