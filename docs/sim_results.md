# Simulated matches

`-sim` (see the [README](../README.md#simulating-matches)) played every team against every other
team on both plan sets. This page compares the plans as shipped with our fixed plans
([plan_bugs.md](plan_bugs.md)), and both against the team ratings the menus show.

## Method

- Every ordered pair of the 14 teams, home and away, so 182 matchups. Each match uses the home
  team's drome.
- Fixed plans: 50 matches per matchup, 9,100 matches. Shipped plans: 5 per matchup, 910 matches.
- The `booie.ini` defaults for difficulty and period length.
- Both runs include the fix for the rok's glide speed cap. Before it, `CGlideOnSurface` read the
  rok's speed rating from past the end of the rok, and some matches ended in "BAD SURFACE NORMAL".
  The fix changed the fixed-plan ranking by at most one place per team.
- Points % counts a win as 1 and a draw as a half.
- "Card" is the average of the team's ratings on the menu cards, and "card rank" orders the
  teams by it.

The shipped run is a tenth of the size, so read small differences in its column as noise.

## How matches change

| | Shipped | Fixed |
| --- | --- | --- |
| Points scored per match, both teams | 6.5 | 13.5 |
| Draws | 13.2% | 4.5% |
| Matches won by 10 or more | 5.4% | 36.2% |
| Shots per team per match | 0 | 11.5 to 23.4 |
| Matches won by the team with more possession | 67.1% | 79.4% |
| Home wins against away wins | 418 to 372 | 4,242 to 4,446 |

On the shipped plans no AI skater takes a shot. The shots stat stays at 0 in every match, and
every point comes from something other than a shot, as [plan_bugs.md](plan_bugs.md) predicts.
The fixes double the scoring and turn a third of matches into blowouts.

## Ranking

| Team | Card | Card rank | Shipped pts% | Shipped rank | Fixed pts% | Fixed rank |
| --- | --- | --- | --- | --- | --- | --- |
| BLAM! Nemesis | 4.78 | 1 | 90.4 | 1 | 98.8 | 1 |
| The Knights (Leeds) | 2.88 | 8 | 54.6 | 5 | 75.0 | 2 |
| The DredNoks (Jamaica) | 3.84 | 2 | 63.8 | 3 | 74.1 | 3 |
| The Machine (Chicago) | 2.53 | 14 | 63.8 | 2 | 69.5 | 4 |
| The Ambush (Warsaw) | 2.91 | 6 | 58.1 | 4 | 66.3 | 5 |
| The Crusade (Toronto) | 2.72 | 13 | 42.3 | 11 | 45.7 | 6 |
| The Fury (Seattle) | 2.75 | 12 | 44.2 | 8 | 44.4 | 7 |
| The Glory (New York) | 2.78 | 10 | 43.1 | 9 | 41.6 | 8 |
| The LoneStars (Houston) | 2.88 | 7 | 45.4 | 7 | 39.0 | 9 |
| The Ronins (Tokyo) | 2.94 | 5 | 42.3 | 10 | 35.9 | 10 |
| The T-34s (Moscow) | 2.75 | 11 | 48.5 | 6 | 31.3 | 11 |
| The BlackJack (Las Vegas) | 2.81 | 9 | 32.3 | 14 | 30.5 | 12 |
| The Shockwave (Los Angeles) | 3.16 | 3 | 36.2 | 12 | 26.0 | 13 |
| The Sturmnacht (Munich) | 2.94 | 4 | 35.0 | 13 | 21.9 | 14 |

- The two plan sets agree on the order: Spearman's rank correlation between them is 0.83.
- The menu cards predict neither. Card rank against results is 0.09 on the shipped plans and
  0.00 on the fixed ones. Using the two starters' cards alone gives 0.35 and 0.20.
- The Knights gain the most from the fixes, from 5th to 2nd. They score through the 4x
  multiplier, which only pays once the AI shoots.
- The Machine has the worst card and finishes 2nd and 4th, so its strength does not come from
  the fixes.
- Spread widens with the fixes: BLAM! Nemesis loses 13 of 1,300 fixed-plan matches.

## Which ratings matter

Rank correlation of each team rating with points %:

| Rating | Shipped | Fixed |
| --- | --- | --- |
| Strength | 0.60 | 0.53 |
| Weight | 0.42 | 0.41 |
| Rok handling | 0.32 | 0.45 |
| Stamina | 0.26 | 0.27 |
| Shot power | 0.13 | 0.24 |
| Acceleration | 0.13 | 0.12 |
| Jumping | 0.09 | -0.03 |
| Speed | -0.01 | -0.06 |

Strength and weight decide matches on both plan sets. Speed and jumping do not, but they count
as much as the others on the cards, which explains most of the gap between cards and results.

## Multiplier goals

On the fixed plans the top teams score through the 4x multiplier:

| Team | 1x goals | 2x goals | 4x goals | Points per shot |
| --- | --- | --- | --- | --- |
| BLAM! Nemesis | 0.8 | 0.3 | 3.5 | 0.77 |
| The Knights | 0.5 | 0.1 | 2.4 | 0.81 |
| The Machine | 0.6 | 0.1 | 2.2 | 0.84 |
| The DredNoks | 1.4 | 0.5 | 1.1 | 0.32 |
| The Shockwave | 0.9 | 0.2 | 0.4 | 0.29 |

Goals are per team per match. The DredNoks take the most shots in the league, 23.4 a match, but
few of them go through the multiplier.

## Home advantage

The fixed round robin has away teams winning slightly more often, but each match is played in
the home team's drome, so arena and home side are mixed up. To separate them, each team played
itself 100 times in Toronto's drome, the arena with the fewest multipliers and randomisers. Home
won 639 and away won 651, with 110 drawn: 49.5% of decided matches, p = 0.74. Being the home team
makes no difference, so the gap comes from the dromes.

## Stats the game never records

Of the per-player match stats, 26 are 0 in all 72,800 player rows of the fixed run. Among them:
completed passes, checks and injuries given, interceptions, drags won, power play and
short-handed stats, 2-point stats, rail time, hang time and freestyle moves. Passes attempted
and injuries taken are counted, so the game either never wired the others up or the port loses
the calls. Assists equal goals in 99.7% of rows, which points to the scorer getting the assist.
Treat both as unverified.

## Not covered

- A seed does not yet replay a match exactly, so a single match cannot be repeated.
- Only the default difficulty and period length were played.
- The shipped run is small. A full 9,100-match run would firm up its column.
