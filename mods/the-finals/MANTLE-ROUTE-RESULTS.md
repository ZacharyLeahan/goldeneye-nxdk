# Windows mantle results, November 18, 2026

For the later Stack atrium fix, controller footage and rerun totals, see the [Stack upper walkway replay](VALIDATION.md#stack-upper-walkway-controller-replay). The tables below describe the earlier build.

The striped Facility barrier passed all ten approaches on the installed gameplay build. Tap, held Jump, and movement and firing after landing were each verified on at least one route in all 20 multiplayer setups and all 20 campaign missions. These modes cover 26 distinct stage IDs. The survey uses the actual Windows executable and stage collision. Multiplayer runs used offline, single-view local practice with zero bots. Campaign runs retained native mission actors.

## The Facility barrier

The top of the striped divider is approximately 159 world units above the near floor. The top exists in the collision mesh, while the navigation floor continues underneath it. The updated detector finds the cap and checks that Bond can clear it and stand on the floor beyond. On the stair-side approach, the climb now accommodates an existing toe overlap with a connected stair tread. The standing body and destination still need clearance.

The ten approaches included a released tap, held A, a farther jump and second A, a flush start, angled approaches, the stair side, the reverse crossing, and movement and fire after landing. On the near approach both tap and hold grabbed 8 simulation ticks after the press (about 133 ms) and landed 31 ticks after the grab (about 517 ms). The magazine stayed unchanged during the climb and the first shot registered on the landing tick. Four additional trials with an SDL virtual controller exercised Windows controller polling, including full forward plus A, a released tap, a hold, a second A after the first window expired and a flush start.

The reference clips show a released tap before visible hand contact. The first clip gives roughly 0.17–0.22 seconds from press to the reaching hand. The awning sequence gives approximately 0.43–0.63 seconds from hand reach to apparent landing. The clips leave the first accepted shot, full steering handoff and any same-ledge hold advantage uncertain. GE32 uses a 300 ms input window and returns firing and steering at landing. [Frame observations](VALIDATION.md)

## Survey results

The [complete trial ledger](MANTLE-ROUTE-TRIALS.csv) includes 1,033 survey, follow-up and repeated attempts. The standard survey recorded **461 passes, 64 misses and 32 skips across 557 trials**. The misses include 23 farther-start attempts, 20 additional geometry samples and seven movement/firing trials. A move/fire trial can fail because Bond never grabbed the ledge. The raw result distinguishes that from an early shot or a bad landing. Seventeen trials had no native floor at the proposed start. Fifteen had no measured roof over a verified landing. The 25 roof passes used an actual overhead surface and an enlarged test body.

For each setup, the second-A column records a separate fresh-game replay. The verified press gaps spanned 21–36 simulation ticks (350–600 ms). A pass requires a takeoff after the first press, no intervening native landing, a second press after the first 18-tick buffer expired, and a grab attributed to that new press followed by a stable landing. 40 of 40 setups passed this exact check.

### Multiplayer practice

| Level | Stage | Pass | Miss | Skip | Second A on a fresh load | Survey routes that missed | Skipped routes |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| Temple | 38 | 8 | 1 | 3 | 21 ticks after first A | offset | close-tap, close-hold, roof |
| Complex | 31 | 15 | 0 | 0 | 21 ticks after first A | — | — |
| Caves | 50 | 8 | 1 | 3 | 25 ticks after first A | far-repress | offset, close-tap, close-hold |
| Library | 48 | 13 | 1 | 0 | 22 ticks after first A | far-repress | — |
| Basement | 45 | 13 | 1 | 0 | 22 ticks after first A | far-repress | — |
| Stack | 46 | 13 | 1 | 0 | 22 ticks after first A | far-repress | — |
| Facility | 34 | 18 | 2 | 0 | 22 ticks after first A | sample-2, sample-3 | — |
| Bunker | 27 | 13 | 1 | 0 | 22 ticks after first A | sample-2 | — |
| Archives | 24 | 9 | 6 | 0 | 21 ticks after first A | hold, far-repress, sample-1, sample-2, sample-3, resume | — |
| Caverns | 39 | 12 | 3 | 0 | 22 ticks after first A | far-repress, sample-1, far-moving | — |
| Egyptian | 32 | 9 | 1 | 2 | 32 ticks after first A | far-repress | close-tap, close-hold |
| Statue | 22 | 7 | 2 | 3 | 25 ticks after first A | hold, resume | close-tap, close-hold, roof |
| Cradle | 41 | 12 | 0 | 0 | 21 ticks after first A | — | — |
| Runway | 35 | 14 | 0 | 1 | 22 ticks after first A | — | roof |
| Streets | 29 | 10 | 1 | 1 | 36 ticks after first A | resume | roof |
| Surface | 36 | 14 | 0 | 1 | 22 ticks after first A | — | roof |
| Dam | 33 | 15 | 0 | 0 | 22 ticks after first A | — | — |
| Depot | 30 | 13 | 1 | 1 | 22 ticks after first A | sample-2 | roof |
| Jungle | 37 | 7 | 2 | 3 | 32 ticks after first A | far-repress, far-moving | close-tap, close-hold, roof |
| Frigate | 26 | 10 | 3 | 0 | 22 ticks after first A | sample-1, close-tap, close-hold | — |

### Campaign missions

| Level | Stage | Pass | Miss | Skip | Second A on a fresh load | Survey routes that missed | Skipped routes |
| --- | ---: | ---: | ---: | ---: | --- | --- | --- |
| Dam | 33 | 15 | 0 | 0 | 22 ticks after first A | — | — |
| Facility | 34 | 18 | 2 | 0 | 22 ticks after first A | sample-2, sample-3 | — |
| Runway | 35 | 14 | 0 | 1 | 22 ticks after first A | — | roof |
| Surface I | 36 | 14 | 0 | 1 | 22 ticks after first A | — | roof |
| Bunker I | 9 | 9 | 5 | 0 | 21 ticks after first A | hold, sample-1, sample-2, close-hold, resume | — |
| Silo | 20 | 9 | 3 | 0 | 22 ticks after first A | far-repress, offset, far-moving | — |
| Frigate | 26 | 12 | 1 | 0 | 22 ticks after first A | sample-1 | — |
| Surface II | 43 | 14 | 0 | 1 | 22 ticks after first A | — | roof |
| Bunker II | 27 | 12 | 2 | 0 | 22 ticks after first A | far-repress, sample-2 | — |
| Statue | 22 | 7 | 2 | 3 | 25 ticks after first A | hold, resume | close-tap, close-hold, roof |
| Archives | 24 | 9 | 6 | 0 | 21 ticks after first A | hold, far-repress, sample-1, sample-2, sample-3, resume | — |
| Streets | 29 | 9 | 2 | 1 | 36 ticks after first A | far-moving, resume | roof |
| Depot | 30 | 13 | 1 | 1 | 22 ticks after first A | sample-2 | roof |
| Train | 25 | 9 | 3 | 0 | 22 ticks after first A | far-repress, offset, far-moving | — |
| Jungle | 37 | 7 | 2 | 3 | 32 ticks after first A | far-repress, far-moving | close-tap, close-hold, roof |
| Control | 23 | 15 | 0 | 0 | 22 ticks after first A | — | — |
| Caverns | 39 | 10 | 4 | 0 | 22 ticks after first A | wall, far-repress, sample-1, far-moving | — |
| Cradle | 41 | 10 | 2 | 0 | 22 ticks after first A | other-edge, far-repress | — |
| Aztec | 28 | 13 | 1 | 1 | 32 ticks after first A | tap | roof |
| Egyptian | 32 | 9 | 1 | 2 | 25 ticks after first A | far-repress | close-tap, close-hold |

## What the trials measured

The fixture placed Bond on native geometry at recorded coordinates, then used the game’s A-button adapter and movement update. A positive mantle required a grab, a completed climb and supported landing near the intended destination. On the farther-back traversal checks, an ordinary jump also passed if it actually cleared the edge and settled on the expected upper floor. The separate second-A check always required a mantle.

The survey tested a quick tap and a hold on the same ledge, two farther starts, a glancing angle, a tall wall, movement/fire recovery, and headroom where geometry permitted. Additional openings and obstacles were exercised at recorded coordinates. The CSV keeps the unsuccessful attempts, including trials where a wider start never reached the lip. This gives sampled coverage of each level.

All 40 open-floor jumps and corrected glancing checks passed. The campaign Caverns tall-wall trial missed its combined jump-and-recovery assertion on the survey run and passed from a fresh load. In Control, the original 83-degree turn faced a different wall only 7 degrees from its normal. Turning along the original wall in the opposite direction preserved the normal jump. The corrected survey exercises both the glancing rejection and the valid corner grab.

Jungle practice twice stopped advancing the multi-case replay. Diagnostic ticks continued with player control disabled. Every Jungle survey case was then run in a fresh game process, including the two longer approaches that missed. In Complex, 45 consecutive cases completed in one match. A separate regression covered ten Complex approaches with a jump pad deployed. The SDL right-stick regression passed five melee cases, and the ordinary-jump regression completed eight moving jumps without an airborne double jump.

## Replaying the tests

Run from the mod directory with a prepared, isolated GE32 2026.11.07 e8859fe9 runtime. The tool writes its input and logs in that game copy. It returns a failure status when a route misses or a test ends early. The default run covers all 40 setups.

```powershell
python run-mantle-routes.py C:\GE32-test C:\mantle-results
python run-mantle-routes.py C:\GE32-test C:\mantle-results --setup Barrier-7
python run-mantle-routes.py C:\GE32-test C:\mantle-results --second-press
python run-mantle-routes.py C:\GE32-test C:\mantle-results --setup MP-2 --case far-repress
```

The [trial ledger](MANTLE-ROUTE-TRIALS.csv) records exact inputs, expected outcome, grab and landing ticks, shot tick, verdict and diagnostic output. The [second-press routes](mantle-second-press.txt) rerun the airborne sequence on every level. The [route manifest](mantle-routes.txt) contains the standard suite and the ten Facility barrier cases. Native traces for this run are in `build/all-level-mantle-mantle-levels-MmqhGD/`. Gameplay DLL SHA-256 in both Windows installs is `6f4a1aeed03f59eee83ddaf26ee8ed7ba8a687717c411776631b8518d0ed7657`.
