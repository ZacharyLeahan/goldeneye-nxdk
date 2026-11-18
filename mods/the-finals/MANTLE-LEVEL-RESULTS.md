# Windows mantle validation across all levels

This earlier survey predates the Facility barrier fix. See the [updated Windows route results](MANTLE-ROUTE-RESULTS.md) for the current build, including Streets.

Tested GE32 e8859fe9 offline in 20 multiplayer practice setups and 20 campaign missions on November 18, 2026. Bond completed a mantle with a released tap and with held Jump in 19 of 20 setups in each mode. All 40 passed a normal-jump control and a solid-wall rejection. Streets had no verified mantle landing in either mode.

## How a landing counted

An offline fixture opened each stage in the Windows executable and placed Bond at sampled coordinates on native map floors. It sent Jump through the A-button adapter and forward through the movement hook. The game had to register one takeoff, select a mantle, pass its path and landing checks, exit the climb, and settle within four vertical units of the expected floor and 35 horizontal units of its own destination. The fixture checked the final grounded state after releasing input. Several trials can belong to the same ledge. Practice and campaign can share geometry.

Tap trials used a one-tick press. Held trials kept Jump down through the grab. The recorded starting position below is a successful tap approach. In the held replays the starting ledge stayed the same and forward input timing could vary. Focused Facility trials separately compare tap and hold at identical approach timing. Test placement made distant areas practical to sample. The survey does not establish a route from spawn to each position or cover online replication.

## Multiplayer practice

| Level | Jump tap | Jump held | Example lower position (X, Y, Z) | Facing | Upper floor Y |
| --- | --- | --- | --- | ---: | ---: |
| Temple | Landed | Landed | 4640.3, 417.1, 1342.2 | 0.0° | 468.8 |
| Complex | Landed | Landed | 961.4, 0.0, 118.0 | -180.0° | 281.1 |
| Caves | Landed | Landed | 18480.1, 1480.7, -5341.2 | -153.7° | 1541.4 |
| Library | Landed | Landed | 639.7, -207.8, 1401.7 | -180.0° | 0.0 |
| Basement | Landed | Landed | 639.7, -207.8, 1401.7 | -180.0° | 0.0 |
| Stack | Landed | Landed | 639.7, -207.8, 1401.7 | -180.0° | 0.0 |
| Facility | Landed | Landed | 8647.5, -478.3, -4756.5 | -180.0° | -238.7 |
| Bunker | Landed | Landed | 798.7, 0.0, -333.3 | 180.0° | 150.2 |
| Archives | Landed | Landed | -2209.6, 0.0, 18.3 | -90.0° | 250.6 |
| Caverns | Landed | Landed | 3979.9, -3097.9, 1091.9 | 136.1° | -3030.8 |
| Egyptian | Landed | Landed | 23.4, 309.7, 2429.5 | 0.0° | 367.1 |
| Statue | Landed | Landed | -8093.8, -62.6, 519.3 | 74.4° | -9.3 |
| Cradle | Landed | Landed | -435.6, -1081.8, -182.4 | 90.0° | -988.5 |
| Runway | Landed | Landed | 7986.2, -658.7, -6289.3 | 0.0° | -457.7 |
| Streets | No clear landing found | — | — | — | — |
| Surface | Landed | Landed | 1282.4, 994.6, -10977.4 | -180.0° | 1144.2 |
| Dam | Landed | Landed | -5838.3, -642.0, -3279.3 | -18.4° | -586.4 |
| Depot | Landed | Landed | 2819.9, -132.7, -6307.3 | -90.0° | -54.9 |
| Jungle | Landed | Landed | 14413.4, 892.5, -11597.3 | 101.4° | 950.7 |
| Frigate | Landed | Landed | 421.2, 315.0, -4963.2 | 0.0° | 404.4 |

## Campaign

| Level | Jump tap | Jump held | Example lower position (X, Y, Z) | Facing | Upper floor Y |
| --- | --- | --- | --- | ---: | ---: |
| Dam | Landed | Landed | -5838.3, -642.0, -3279.3 | -18.4° | -586.4 |
| Facility | Landed | Landed | 6245.4, -26.5, -2702.9 | 90.0° | 53.0 |
| Runway | Landed | Landed | 7171.2, -658.7, -6289.3 | 0.0° | -457.7 |
| Surface I | Landed | Landed | 1282.4, 994.6, -10977.4 | -180.0° | 1144.2 |
| Bunker I | Landed | Landed | 798.7, 0.0, -333.3 | 180.0° | 150.2 |
| Silo | Landed | Landed | 1796.0, -101.6, -1056.5 | 135.0° | -40.2 |
| Frigate | Landed | Landed | -222.3, 315.0, -4832.1 | 0.0° | 391.0 |
| Surface II | Landed | Landed | 1282.4, 994.6, -10977.4 | -180.0° | 1144.2 |
| Bunker II | Landed | Landed | 798.7, 0.0, -333.3 | 180.0° | 150.2 |
| Statue | Landed | Landed | -8093.8, -62.6, 519.3 | 74.4° | -9.3 |
| Archives | Landed | Landed | -2209.6, 0.0, 18.3 | -90.0° | 250.6 |
| Streets | No clear landing found | — | — | — | — |
| Depot | Landed | Landed | 2819.9, -132.7, -6307.3 | -90.0° | -54.9 |
| Train | Landed | Landed | -15021.7, 0.0, -47.0 | 160.3° | 166.4 |
| Jungle | Landed | Landed | 14413.4, 892.5, -11597.3 | 101.4° | 950.7 |
| Control | Landed | Landed | 77.5, 274.6, -1381.3 | 180.0° | 348.8 |
| Caverns | Landed | Landed | 5824.6, -3097.9, 1091.9 | -136.1° | -3030.8 |
| Cradle | Landed | Landed | -435.6, -1081.8, -182.4 | 90.0° | -988.5 |
| Aztec | Landed | Landed | 2794.6, 11.3, -728.2 | 180.0° | 110.5 |
| Egyptian | Landed | Landed | 23.4, 309.7, 2429.5 | 0.0° | 367.1 |

## Geometry and input checks

- **Streets.** In each mode, a denser pass sampled 131,721 edge positions and probed 1,800 combinations of position and approach within the search height range. None passed the full mantle query. Six additional approaches driven in each mode produced no grab. Jump and solid-wall rejection passed. This leaves a playable mantle route on Streets unconfirmed.
- **Complex.** The dedicated practice run completed all six routes: two ramp-side climbs, both ends of the elevated walkway, the raised landing and the pit rim. The open-floor trial stayed a jump.
- **Facility.** A tap, a hold and a jump from 27 units farther back reached the same upper floor. The glancing approach and too-tall wall retained the jump. The low-ceiling fixture raised the test body height under a real ceiling and recorded a detected ledge with failed clearance. The magazine stayed unchanged during the climb. The first shot registered on the landing tick.
- **Jungle.** The successful approach in the tables settled at Y 950.74 in both modes. An earlier nearby approach reached the top and drifted off. Direct placement on that spot also produced motion. The successful replay isolated analog input and incoming shot push, alongside the ordinary jump and wall controls.
- **Steep ground.** Two fixes prevent uphill floor samples from acting as a ceiling or pinning an ascending player. On the sampled Caves incline, the feet rose from 1493.29 to 1514.31 as vertical speed fell from 5.52 to 4.69. The final gameplay DLL also passed eight moving jumps on Dam and eight ceiling-stress jumps on Facility, including rejection under a real low ceiling.

The F8 readout shows the Jump press tick, time since press, ledge detection, clearance and the selected action. The evidence archive contains the native per-stage traces, starting coordinates, facing, expected top, final foot height, grab and landing ticks, plus the focused regressions. The two game installs have the same production DLL, SHA-256 a4d711018e9d1f688f21811ae4c568237b8fc831ab2ed1315942991ea1b74e3a.
