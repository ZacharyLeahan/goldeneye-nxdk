# Windows controller mantle tests

November 18, 2026. All 20 multiplayer maps and 20 campaign missions ran in the native GE32 Windows executable. Multiplayer practice had zero bots. Campaign runs kept the native mission actors.

On each setup a virtual SDL Xbox controller pressed A, released it and pressed A again while the player was airborne. The verified replay for each row has one takeoff, a distinct second press, a grab carrying that press tick, a later native landing, and a supported player on the intended top at the end of the trial. These replays position Bond on the measured approach before applying controller input.

The from-spawn runs used forward, turning, use and A without writing player position. The controller reached and landed on a ledge in 18 of 40 setups. “Unconfirmed” means the walkthrough did not establish a successful mantle. Some approaches ended at solid walls, below a platform, or beyond the tested reach. A positioned success establishes one usable ledge on that setup.

Times below are 60 Hz simulation ticks. The first interval is between the two A presses, the second is from the airborne A to the grab, and the third is from the grab to landing.

## Multiplayer

| Level | Walked from spawn | A to A | Second A to grab | Grab to landing |
|---|---|---:|---:|---:|
| Temple | Unconfirmed | 21 | 2 | 31 |
| Complex | Landed | 21 | 1 | 31 |
| Caves | Landed | 26 | 2 | 17 |
| Library | Landed | 22 | 6 | 31 |
| Basement | Unconfirmed | 22 | 6 | 31 |
| Stack | Landed | 22 | 6 | 31 |
| Facility | Landed | 22 | 1 | 31 |
| Bunker | Landed | 22 | 6 | 31 |
| Archives | Unconfirmed | 21 | 7 | 31 |
| Caverns | Landed | 22 | 2 | 31 |
| Egyptian | Landed | 33 | 1 | 17 |
| Statue | Unconfirmed | 25 | 1 | 31 |
| Cradle | Unconfirmed | 21 | 2 | 31 |
| Runway | Landed | 22 | 6 | 31 |
| Streets | Landed | 36 | 4 | 17 |
| Surface | Landed | 22 | 6 | 31 |
| Dam | Landed | 22 | 8 | 17 |
| Depot | Landed | 23 | 2 | 31 |
| Jungle | Unconfirmed | 28 | 11 | 17 |
| Frigate | Landed | 22 | 9 | 31 |

## Campaign

| Level | Walked from spawn | A to A | Second A to grab | Grab to landing |
|---|---|---:|---:|---:|
| Dam | Unconfirmed | 22 | 8 | 17 |
| Facility | Unconfirmed | 22 | 1 | 31 |
| Runway | Landed | 22 | 6 | 31 |
| Surface I | Unconfirmed | 22 | 6 | 31 |
| Bunker I | Unconfirmed | 21 | 4 | 31 |
| Silo | Unconfirmed | 22 | 5 | 17 |
| Frigate | Unconfirmed | 22 | 6 | 31 |
| Surface II | Unconfirmed | 22 | 6 | 31 |
| Bunker II | Unconfirmed | 22 | 6 | 31 |
| Statue | Unconfirmed | 25 | 1 | 31 |
| Archives | Unconfirmed | 21 | 7 | 31 |
| Streets | Landed | 36 | 4 | 17 |
| Depot | Unconfirmed | 22 | 3 | 31 |
| Train | Unconfirmed | 22 | 1 | 31 |
| Jungle | Unconfirmed | 28 | 11 | 17 |
| Control | Unconfirmed | 22 | 6 | 31 |
| Caverns | Unconfirmed | 22 | 2 | 31 |
| Cradle | Unconfirmed | 22 | 8 | 31 |
| Aztec | Landed | 21 | 1 | 31 |
| Egyptian | Landed | 25 | 5 | 17 |

## Reproduced cases

- Stack had two stable landings in its first-seat walkthrough, each following an airborne second A. One approach recorded 23 ticks between presses, 3 ticks from the second press to the grab and another 31 ticks to land.
- On Jungle, the original turret-side approach produced a grab and native landing followed by displacement. The alternate ledge near X 7113, Z −9426 passed the full stability check in both modes with the turret active.
- Aztec had stable landings from the original approach on the first press. A longer approach verified a grab attributed to the second press.
- The right-stick melee regression passed all five Complex cases on the production co-op build, including the click after a mantle.

The earlier fixed-input survey covers tap, hold, glancing approaches, overhead clearance, movement and firing. Its full case descriptions are in [MANTLE-ROUTE-RESULTS.md](MANTLE-ROUTE-RESULTS.md). The local test archive `build/real-ledge-ODDb3p/controller-evidence.zip` contains the native logs and per-level landing screenshots.
