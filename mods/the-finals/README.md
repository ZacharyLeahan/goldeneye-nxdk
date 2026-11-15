# GE32 extension

Source for the tested Finals-inspired controls and equipment wheel. See [DOWNSTREAM.md](../../DOWNSTREAM.md) for the feature summary and upstream relationship.

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
| LB held | Open equipment wheel: guns, Grenades (right), Jump Pad (bottom) |
| Right stick with wheel open | Select equipment; return to center to clear selection |
| Release LB | Equip the highlighted item; neutral center keeps current equipment |
| B with wheel open | Cancel selection |
| LT | Aim |
| B during gameplay | Toggle crouch; cancel when holding the pad |
| A | Jump; clear crouch first; one jump per press |
| Y | Interact / open doors |
| Left-stick click while moving forward | Toggle sprint; stop/aim/fire/crouch/wheel/slap cancels |
| Right-stick click | One native slap, then return to the gun |
| X | Reload |
| RT while holding Grenades | Native grenade throw; three per life |
| RT while holding Jump Pad | Place on valid floor, then return to gun |
| Walk onto a placed pad | Launch upward; steer with movement stick |

The wheel does not pause gameplay. Fire and look input are suppressed while selecting. Engine inventory/equipment functions execute only on the local player's gameplay thread; drawing uses a lock-protected snapshot.

## Files

- weapon-wheel.c: game-thread hook, inventory snapshot/queue, controller handling, regression tests.
- combat.h: sprint ramp, input edges, temporary slapper state, ammunition-safe gun restoration.
- combat-test.h: offline sprint/slap integration checks.
- jump.h: grounded takeoff, native vertical integration, ceiling checks, input/physics tests.
- jump-pad.h: carried gadget, floor validation, native launch, upper-floor traversal, input edges and render snapshot.
- jump-pad-render.h: procedural pad, hologram preview and carried equipment visual.
- jump-pad-test.h: native Temple equipment-to-upper-floor acceptance tests.
- health-bars.h: confirmed native-hit observer, true health/armor reads, target lifecycle and locked render snapshots.
- health-bars-render.h: world-anchored segmented bars and compact names.
- health-bars-test.h: native gunfire, timing, armor, lifecycle, sight and mission-guard tests.
- retro-ui.h: original 5x7 pixel lettering shared by the gadget and target gauges.
- grenades.h: once-per-life native grenade loadout and total ammunition.
- grenades-test.h: native equip/throw/depletion and equipment round-trip acceptance.
- wheel-layout.h: fixed right/bottom gadget quadrants, upper/left gun sectors.
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

Before ascent, 17 upward probes across the body footprint check loaded room geometry and occupied-room dynamic doors/scenery. Low standing clearance rejects takeoff; a ceiling contact stops upward velocity. These are conservative sampled clearance checks, not a new continuous capsule collision engine or a mantling system. The wheel, aim profile and B crouch remain intact.

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

## Deployable jump pad

Jump Pad is always available in the LB equipment wheel beside the guns currently owned. Select it with the right stick and release LB. Aim down at a level, clear surface; a muted green floor preview marks a valid placement and red marks an invalid one. RT deploys the pad and returns to the previous gun. The placement press cannot discharge the gun, even if RT remains held. B cancels the gadget, and selecting a gun from LB also stows it. RB is unchanged.

Walk onto the deployed pad to launch immediately on contact. Native gravity, momentum, air movement and sampled ceiling clearance remain active. The 26-unit impulse clears Temple’s lower-to-upper opening; steer toward the ledge after clearing its height. A pad is reusable, and placing another replaces the previous local pad. A low ceiling stops ascent. Player/stage changes clear pad state. Placement requires a supported, nearly level footprint and rejects airborne, obstructed or missing-floor positions.

The pad uses original procedural geometry, a translucent preview and a carried-device drawing; no game/Embark assets are bundled. Characters use movement bounds, never render-hit caches. Native room collision bounds are prepared lazily on the simulation thread during high flight. Physics and projection are published to the drawing thread through a locked snapshot.

Pads currently belong to the local player on this computer. Shared pad objects and activation across separate PCs are not networked yet; only the existing native player movement updates remain in use.

Offline acceptance: set GE32_PAD_TEST=1 and GE32_REGRESSION_PRACTICE=1. Add GE32_PAD_VISUAL_TEST=1 to write local renderer BMP readbacks and inspect the preview longer. These flags supply test guns, move the test player and exit; leave them unset for normal play.

## Neutral equipment-wheel center

Opening LB highlights the equipped item if it is represented in the wheel. A deliberate gentle stick nudge or returning from a sector enters the empty center. Releasing LB from center keeps the current item; recentering and releasing on the same frame also cancels. Move outward again to select an item. Opening and releasing an unchanged item does not enqueue a redundant weapon change. The wheel uses icons for every item and shows only the highlighted item’s name beside its icon. The center stays transparent, with no text panel or repeated equipped labels.

## Temple edge collision and movement stress coverage

Ledge and added-jump traversal retains a shared support/collision context until the full body has a stable landing. Returning to the upper floor restores its native navigation tile before ordinary collision runs. Briefly touching the lip no longer ends traversal while the body still straddles the opening.

A 16-point circular footprint plus the center covers the body’s front, back and sides. Horizontal sweeps and vertical destination probes include the toe region, and collision geometry is prepared for intersected rooms independently of camera visibility. Gravity, movement input and landing clamps remain native. These are conservative sampled checks, not a continuous capsule collision engine.

Set GE32_TEMPLE_STRESS=1 with GE32_REGRESSION_PRACTICE=1 for the offline 64-route perimeter suite: four edges, corners, retreat timings, strafing, sprint, crouch, jumps and eight pad launch directions. GE32_TEMPLE_DEFERRED_AUDIT=1 checks the recorded paths against background geometry after each route; GE32_TEMPLE_GEOMETRY_AUDIT=1 checks it during movement. The deferred version avoids preparing collision caches ahead of the tested movement. GE32_TEMPLE_VISUAL_TEST=1 writes a local renderer readback for a representative room transition. GE32_TEMPLE_PROBE=1 writes local floor/room diagnostic grids and exits. All flags are offline-only and leave normal gameplay unchanged when unset.

## Confirmed-hit health bars

A damaging native bullet/melee hit reveals the target’s current health above their head. Repeated damaging hits restart the timer. The supplied roughly 60 fps reference recording showed approximately 3.25 seconds at full visibility after the last hit, followed by a 0.20-second fade; these are the extension’s named timing constants. This matches the supplied footage rather than claiming a universal timing specification for every version or mode of THE FINALS. The fade eases out, and the main bar updates immediately with a brief lighter damage trail. Armor uses a thin secondary strip. Only the actual player/bot name (or GUARD) is shown; no numeric damage counter or character outline is added.

Misses, invulnerable hits and other shooters’ damage do not open the display. Each target has its own timer. Native health changes update an already-visible bar without extending its timeout. Dead targets fade promptly; removed identities, new lives and stage/local-player changes clear old records. The bar is culled behind solid background/doors, outside the viewport or behind the camera, and hidden while gameplay is unavailable. Pausing freezes the simulation-time timeout. Drawing receives only a lock-protected value snapshot; it never follows native character pointers or changes damage.

The hit callback is signature-checked for the supported GE32 release. The game’s damage/armor/health behavior is unchanged. Cross-PC authoritative-health updates, all weapons and every map are not exhaustively verified. A remote hit awaiting replicated HP is kept briefly pending and cannot invent health or show a bar before an observed decrease.

Offline tests: GE32_HEALTH_TEST=1, GE32_REGRESSION_PRACTICE=1 and GE32_PAD_LIVE_TEST=1 launch the native Temple fixture with actual targets, exercise native gunfire and then exit. GE32_HEALTH_VISUAL_TEST=1 also writes local hit/fade BMP readbacks. The fixture freezes bots, places the player and target, invokes native trigger/strike paths, and supplies eligibility/health fixtures for armor and lifecycle cases. GE32_HEALTH_GUARD_TEST=1 with -level_34 validates native mission-guard damage and armor semantics. These flags are for offline tests only and must be unset for ordinary play.

## GoldenEye equipment presentation and enlarged pad

The jump pad now has a 156-unit diameter (previously 96): about 63% wider, with placement support and activation matched to the octagonal deck. Sixteen floor samples and radial clearance traces reject unsupported rims and obstructed placement. The old deployment arming delay is removed. Reuse cooldown, native velocity/gravity, ceiling checks and upper-floor traversal remain active.

The supplied jump-pad clip demonstrates equipment selection, carrying, floor preview, placement and returning to the gun. It ends before a complete launch arc, so it is not used to invent an airtime measurement. The tested 26-unit impulse is retained for Temple’s lower-to-upper route. An approximately 0.18-second carry/stow motion and a short deck-compression response provide equipment feedback; the gun model returns after the held device has lowered.

The pad uses an original low-poly olive housing, beveled rim, fasteners, a coarse generated ribbed-rubber material and worn identification paint. Perspective-correct UV interpolation keeps the material attached to its projected geometry. The equipment-wheel icon matches the new octagonal device. The preview and interaction hints use subdued game-era colors and original pixel lettering.

Target health uses a dark gunmetal bezel, eight green health cells (amber/red at lower health), a blue armor strip and small uppercase pixel labels. The 3.25-second hold, 0.20-second fade, hit detection and target lifecycle are unchanged. Names and equipment hints share original asset-free 5x7 glyphs; they do not copy or ship the game’s font.

## Fixed gadget directions and grenades

Jump Pad stays at the bottom of LB and native GoldenEye hand grenades stay on the right. Guns share the upper/left half; a single gun appears at the top. The right slot shows remaining grenades and remains visible, dimmed at zero. Empty grenades cannot be equipped; native ammo pickups re-enable them. Each playable local life starts with three, awarded once after gameplay becomes available; throwing or changing equipment never tops them up. Native throw animation, fuse, projectile and damage are retained. This is hand-grenade item 26; the virtual pad uses a separate ID outside native weapons.

Run GE32_GRENADE_TEST=1 with GE32_REGRESSION_PRACTICE=1 only in an offline test installation. It exercises wheel selection, three real native throws, depletion, empty-slot rejection, switching back through the pad/guns, and a native-inventory ammo-pickup fixture. Cross-PC grenade replication remains unverified.
