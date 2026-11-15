# GE32 extension

Source for the tested Finals-inspired controls and weapon wheel. See [DOWNSTREAM.md](../../DOWNSTREAM.md) for the feature summary and upstream relationship.

## Compatibility

Windows 64-bit only. Requires the official GE32 fan release **2026.11.07 e8859fe9 win64**, downloaded separately from [the author](https://popnoodler.itch.io/ge32). The extension uses build-specific executable addresses. The patch scripts verify SHA-256 fingerprints and reject unsupported executables.

The base PC-port source in this repository does not contain GE32 multiplayer. Building the upstream port does not build the extension automatically.

## Build and install

Install Python 3 and Zig 0.13.0 separately. Extract the supported GE32 Windows release, supply your own US ROM, launch it once to initialize settings, and close the game.

From PowerShell:

```powershell
.\mods\the-finals\build-and-install.ps1 -GameDirectory '<GE32 folder>' -Zig '<zig executable>' -Python '<python executable>'
```

The installer builds the extension, applies B-back and loader patches, copies the DLL into the supplied GE32 folder, and applies the controller profile. It does not download or copy ROMs.

The profile preserves unrelated settings and makes an INI backup. It uses Custom controller presets so B crouch is honored. Invert flags match the locally tested GE32 configuration; adjust them in-game if your device/save already reverses pitch.

## Controls

| Input | Behavior |
| --- | --- |
| LB held | Open weapon wheel |
| Right stick with wheel open | Select carried gun |
| Release LB | Queue selected gun on the gameplay thread |
| B with wheel open | Cancel selection |
| LT | Aim |
| B during gameplay | Toggle crouch |
| A | Jump; clear crouch first; one jump per press |
| Y | Interact / open doors |
| Left-stick click while moving forward | Toggle sprint; stop/aim/fire/crouch/wheel/slap cancels |
| Right-stick click | One native slap, then return to the gun |
| X | Reload |
| LB wheel | Weapon selection |

The wheel does not pause gameplay. Fire and look input are suppressed while selecting. Engine inventory/equipment functions execute only on the local player's gameplay thread; drawing uses a lock-protected snapshot.

## Files

- weapon-wheel.c: game-thread hook, inventory snapshot/queue, controller handling, regression tests.
- combat.h: sprint ramp, input edges, temporary slapper state, ammunition-safe gun restoration.
- combat-test.h: offline sprint/slap integration checks.
- jump.h: grounded takeoff, native vertical integration, ceiling checks, input/physics tests.
- wheel-render.h: production renderer, font atlas, vector silhouettes, translucent layout.
- preview.c: hidden-window visual test using the production renderer.
- patch-character-back.py: reversible standard character-menu B-back patch.
- install-wheel.py: reversible extension loader patch.
- apply-controls.py: portable configuration profile.
- build-and-install.ps1: build and setup entry point.
- VALIDATION.md: recorded test coverage and limits.

## Validation commands

Run GE32 from its installation directory. Setting GE32_WHEEL_TEST=1 runs an offline test session that adds test guns/ammunition, exercises switching and inventory changes, and exits. Setting GE32_STAND_TEST=1 tests A press/hold/release behavior and exits before starting gameplay. Do not set either variable for normal play.

Build preview.c with the same compiler/libraries as the DLL, as an executable instead of a shared library. Place the resulting executable beside SDL2.dll and pass an output directory for BMP previews. Outputs are local test artifacts and are ignored by Git.

## Rollback

With GE32 closed, restore GE32.before-wheel.exe as GE32.exe to remove the wheel while retaining the B-back patch, or GE32.original.exe to remove both executable patches. Restore data/ge007.before-the-finals.ini to undo the controller profile. These backups stay only in your GE32 installation folder.

## Licensing and contents

Original extension code is provided under the repository's MIT license. Upstream attribution, NOTICE and LICENSE remain intact. No third-party runtime/compiler code or game assets are vendored with this extension. No game binaries or ROMs are distributed. THE FINALS is used only as a description of the control inspiration; this project is unofficial and unaffiliated with Embark Studios.

## Jump implementation

A presses enqueue a request from the input adapter. The simulation-thread vertical-movement hook accepts takeoff only when grounded, alive, unpaused, outside menus/cinematics/tanks, and not frozen by a match countdown. Native vertical velocity receives an impulse of 5.8; the engine retains gravity, horizontal movement, floor following, landing, and collision/body-position updates. There is no extra jump while airborne or automatic repeat while A is held.

Before ascent, nine upward probes across the body footprint check loaded room geometry and occupied-room dynamic doors/scenery. Low standing clearance rejects takeoff; a ceiling contact stops upward velocity. These are conservative sampled clearance checks, not a new continuous capsule collision engine or a mantling system. The wheel, aim profile and B crouch remain intact.

Jump tests use GE32_JUMP_TEST=1. Add GE32_JUMP_MOVING_TEST=1 for scripted forward/back movement. GE32_JUMP_CEILING_TEST=1 temporarily increases the test impulse to reach a real ceiling; it must never be used for normal play. None of these test flags should be set for ordinary gameplay.

Cross-PC replication is not yet verified. All physics work stays in the local simulation context and existing position/body updates, without changing the network protocol.

## Sprint and quick slap

Click the left stick while moving forward to toggle a smooth ramp up to 1.35 times the engine's current movement speed. This scales the native forward/strafe movement scalars on the simulation thread; it does not teleport the player or replace collision handling. Stopping, backing up, aiming, firing, crouching, opening the wheel, or starting a slap cancels sprint. Sprint is unavailable in menus, cinematics, tank mode, match freezes, or death. There is no stamina system or slide mechanic.

Click the right stick for a single native slapper attack. The simulation temporarily selects the palm model and fist item, runs the engine's existing slap animation/strike dispatch, and restores both hands, partially loaded magazines, reserve ammunition, and previous-weapon history. Sniper ownership does not substitute a rifle-butt animation. A wheel selection made during the slap is deferred until the slap ends. Holding the stick button does not repeat attacks.

The slap is accepted from ready/fire/recoil states; a weapon transition or reload can temporarily prevent starting it. Pause freezes the action phase; death/tank/stage changes clear the temporary action state. Cross-PC replication and real-target damage still need a multiplayer/playtest pass; automated checks verify native strike dispatch without a spawned target.

Set GE32_COMBAT_TEST=1 only for the offline sprint/slap integration harness. It supplies test weapons and ammunition, runs movement and strike cases, then exits. Never enable it for normal play. GE32_STAND_TEST=1 also checks left/right-stick button press/hold/release edges before the game starts.

## Ledge drops

Walk forward off an open ledge, or press A while moving off it. The extension permits a blocked native step only when a lower walkable floor and sampled landing footprint are verified. Loaded background geometry and dynamic props are traced across the body; a downward trace rejects a solid deck between the ledge and landing. Native movement remains the first choice, and native gravity and landing clamps perform the fall. Health is not overridden.

The support footprint retains the upper floor until the player clears its edge. All drop state and physics writes stay on the local simulation thread. This is sampled clearance, not a continuous swept capsule or a climbing/mantling system. Walls, doors, arbitrary map edges, and cross-PC replication still warrant broader playtesting.

Set GE32_DROP_TEST=1 only for the offline Temple integration fixture. It launches practice, isolates the local player from practice-bot damage, repositions the test player at the upper opening, tests walking/jumping/crouched drops and geometry traces, then exits. GE32_DROP_BASELINE=1 disables the fallback for diagnostic comparison. Neither variable should be set for ordinary play.

For deterministic wheel regression, set GE32_REGRESSION_PRACTICE=1 alongside GE32_WHEEL_TEST=1. This starts an isolated Temple practice session rather than a solo mission with hostile guards. It is a test-only flag.

Character clearance uses native movement bounds rather than character bullet-hit caches. Jump eligibility follows the native controllable camera state (normal camera, or first-person camera with its timer enabled), together with the existing pause/menu/cinematic and match-freeze gates. Pressing or holding A during the intro does not queue a jump for the control handoff; release and press A after play begins.

GE32_DROP_LIVE_TEST=1 alongside GE32_DROP_TEST=1 retains three bots and adds actor-bound collision fixtures, including temporarily missing render-hit caches restored within the simulation-thread fixture. Damage from bots remains enabled. GE32_INTRO_TEST=1 alongside GE32_JUMP_TEST=1 attempts jumps throughout the intro, verifies unchanged jump physics and an empty queue at gameplay handoff, then runs the ordinary jump suite. These flags are only for offline tests.
