# Validation

## Gameplay and input coverage

The local GE32 build passed 20 alternating completed weapon switches and 100 rapid wheel reopen/cancel cycles. Checks also covered local-versus-foreign-player inventory snapshot isolation, new pickups, removal of carried weapons, explicit empty inventory, separate gameplay/render threads, and drawing without OpenGL errors.

A-to-stand tests passed for clearing toggle crouch, consuming a held A press, resetting on release, preserving standing interaction, and crouching/standing repeatedly. The crouch binding was enabled through Custom controller presets.

The user has playtested the weapon wheel and smoother aiming. Physical-controller feel is user-verified; automated tests do not replace that feedback.

## Visual coverage

The production wheel renderer was exercised in a hidden test window with 1, 2, 3 and 8 owned-item layouts, including changing selection. Visual inspection corrected text alignment and crowded labels. OpenGL error checks passed. The later transparency change preserved text/icon opacity while reducing panel opacity and background dimming.

## Publication checks

- Extension source compiles with Zig 0.13.0 and -Wall -Wextra -Werror.
- Included patch scripts reproduce the running patched executable byte-for-byte from the pristine supported release.
- Apply-controls fixture confirms unrelated player/server/game settings are preserved.
- Git ignore checks cover lowercase/uppercase z64, n64, v64 and rom files, saves, and local runtime/build outputs.
- Only source, configuration logic, patch scripts and documentation are committed; no ROM, ROM-derived assets, runtime binaries or local-machine credentials are included.

## Limits

The extension is pinned to GE32 Windows 2026.11.07 e8859fe9 and is not a portable multiplayer source implementation. Automated gameplay tests used an offline session; cross-PC multiplayer regression testing remains outstanding. Jumping has offline coverage as detailed below. Sliding is not implemented.

## Jump validation

- A-input edge tests: a press queues one request and clears crouch; holding A does not queue repeated requests; release rearms; menu A is preserved.
- Dam: 8 stationary native-physics jumps, consistent 60.60-unit peak and exact floor landing.
- Facility: 8 indoor native-physics jumps and floor landings.
- Dam moving test: 8 jumps while forwarding/backing with native movement inputs; maximum observed horizontal excursion 74.45 units.
- Facility ceiling stress: 8 test-only stronger jumps contacted a real ceiling, stopped ascending, and landed; 8 ceiling stops. Normal impulse remains 5.8, not the test impulse.
- A simulation-thread low-clearance fixture under that real ceiling rejected takeoff and restored the test state before further native movement.
- Airborne A did not produce another jump. Holding A through landing did not bounce again. No non-finite vertical states were observed.
- After integrating jumping, the full 20-switch / 100-reopen wheel regression passed, including inventory isolation, pickup/removal, cancellation, empty state and OpenGL checks.

These tests verify local native physics and clearance handling. They do not verify cross-PC replication, every map edge/prop arrangement, or final physical-controller feel. Clearance uses a sampled footprint rather than a continuous swept capsule, so unusual edge geometry remains a playtest area.

## Sprint and quick-slap validation

- Controller-edge checks passed for one event per press, no repeat while held, rearming on release, and menu passthrough.
- Native movement test passed: forward sprint ramps up, and aiming, firing, crouching, wheel opening, backward movement, and stopping cancel it. Native horizontal displacement was observed.
- Six slap cases passed: single pistol, dual pistols, owned sniper with palm override, rifle, deferred wheel choice, and rejecting another active-slap request.
- Each case emitted exactly one native fist strike event and returned to the saved gun(s). Partially loaded magazines, reserve totals and previous-weapon history were preserved.
- Test guns were supplied only by GE32_COMBAT_TEST; ordinary launches do not add inventory or ammunition.
- The profile fixture preserved unrelated settings and verified Y interact, no Y weapon cycling, B crouch and LT aim for all four controller sections.

No live target was spawned for the automated slap tests. They verify the native slapper animation/strike dispatch rather than independently measuring target damage. Cross-PC sprint/slap behavior remains unverified.

After sprint/slap integration, 8 moving native-physics jumps and landings passed, followed by the full 20-switch / 100-reopen wheel suite, inventory isolation, pickup/removal, empty-state and OpenGL checks.

## Ledge-drop validation

- Temple baseline reproduced the invisible ledge stop above the lower floor.
- Three consecutive native-engine cases passed: walk off, A jump while moving off, and crouched movement off the opening.
- Each case landed on the lower floor at -468.79 units from an initial upper floor of 468.79, with full health preserved (1.000).
- The jump case emitted a native takeoff and landing.
- Background tracing rejected a downward path through the solid upper deck and accepted the adjacent opening. Infinite background rays are filtered to the tested segment.
- Practice bots are excluded only by the test fixture; normal play retains them. No health override or global damage suppression was added.

Coverage is local/offline and specific to the Temple opening plus the existing regression suites. Sampled body/landing checks do not prove every map, door, thin obstacle, or cross-PC interaction.

After drop integration, controller-edge checks, six native slap cases, sprint cancellation, and eight moving jumps passed. The wheel suite passed all 20 switches, 100 rapid reopen/cancels, ownership isolation, inventory changes, empty state and GL checks in isolated practice. A prior solo-mission run failed a late switch assertion; the deterministic practice fixture excludes hostile interference and explicitly supplies both test weapons.

## Character-clearance crash and intro lockout correction

The user reported a real Temple jump crash after the earlier isolated suite passed. Its stack entered projectileFindCollidingProp -> character detailed-hit testing -> a null ModelHitEntry chain. The previous fixture disabled practice bots and therefore did not cover this failure. Character clearance now uses native movement polygons with conservative bounds, while object/door traces exclude the detailed character-hit path.

The intro gate also follows native controllable-camera semantics, including first-person timer readiness and match freezing. The input-edge test checks that holding A across the handoff does not queue a jump and that releasing and pressing again rearms it. The native intro test consumed 1,134 queued jump attempts without changing jump physics or retaining a request, then passed eight normal jumps and landings after control began.

The bot-present fixture retains three active bots and checks blocking movement bounds while each tested actor temporarily has no render-hit cache; it restores that pointer immediately on the simulation thread before native gameplay resumes. Ordinary damage remains enabled in this live-bot test, so health loss from bots is permitted. The isolated no-fall-health-loss coverage above remains separate.

Final acceptance: all three bot-present gap cases landed successfully with full health in the final run. Nine explicit missing-render-cache actor fixtures blocked correctly; the deck/opening traces passed. Strict -Wall -Wextra -Werror compilation and controller-edge checks also passed.

## Deployable jump pad

- LB wheel lists every owned gun plus Jump Pad, including when no guns are owned. A distinct original silhouette and equipment labels identify the gadget.
- Native Temple acceptance selected the pad through the real wheel, equipped it, deployed with an RT edge and returned to the previous gun without changing either hand or magazine. Holding the placement trigger did not fire the gun.
- The launch used native vertical velocity 26, gravity and horizontal momentum. The tested arc peaked at 748.04 from a -468.79 lower floor, then landed on the 468.79 upper floor with full health.
- The same placed pad launched the player again without a second deployment.
- Airborne and missing-floor placement attempts were rejected. B cancellation and selecting a gun from LB preserved ammunition totals.
- A real Temple ceiling at 311.44 stopped pad ascent; the player landed normally. Owner/context reset cleared placed/flight/equipped state.
- Trigger-edge checks covered held-trigger suppression, fresh-press rearming, menu/intro suppression and preventing gun fire until RT release.
- Renderer readbacks were visually reviewed for the LB wheel, carried pad, blue hologram and placed pad. GL error checks passed during the native acceptance fixture. World projection uses native camera matrices and viewport parameters; visibility uses simulation-thread geometry/prop traces.
- The equipment wheel regression passed 20 alternating gun switches and 100 rapid reopen/cancel cycles, foreign-player isolation, pickup/removal, and retaining the pad after all guns were removed.
- Strict DLL compilation passed with -Wall -Wextra -Werror.

Native room geometry bounds are lazy; the high arc exposed an absent-bound cache being interpreted as a blocking ceiling. The extension now requests the engine’s room load/bounds preparation on the simulation thread before the occupied-room ceiling query. It still stops if preparation fails.

Pads are local gadget state and do not yet replicate as shared objects or activate on other PCs. Character/ceiling clearance is sampled and conservative, not a new continuous collision engine. All-map, tilted-surface and cross-PC behavior are not covered. No Embark assets or game content are distributed.

The full gadget acceptance suite also passed with three active practice bots, including three explicit missing-render-hit-cache character fixtures. Upper-floor landing, reuse, invalid placement, cancellation, gun return/ammunition checks, ceiling contact and reset all completed without a crash.

After the room-cache change, Facility’s indoor ceiling stress regression passed all eight native jumps/ceiling contacts and exact floor landings, including rejecting takeoff under a real low-clearance fixture.

## Neutral equipment-wheel center

Native wheel regression passed opening on the equipped item, leaving it unchanged without a redundant equip request, a gentle 18% stick nudge entering neutral, outer selection -> center -> release, recenter/release on the same frame, leaving center to reselect, and B cancellation. The full 20-switch / 100-reopen suite passed, including pickup/removal, foreign-player isolation, GL rendering and keeping Jump Pad available with no guns. Opening when the equipped item is absent from the wheel stays neutral.

Nine production-renderer previews passed GL checks (one/two/three/eight items, gun selection, gadget selection and neutral). Highlighted and neutral layouts were visually reviewed on dark and bright backgrounds; subtle icon/name shadows retain contrast. Slots show icons only; only the highlighted item has a name above the wheel. The central disk and repeated labels/instructions were removed.

After the neutral-center update, the full native jump-pad acceptance suite passed again: LB selection, RT deployment, upper-floor landing, reuse, invalid placements, B cancellation, LB gun selection/ammo totals, ceiling contact and context reset.

## Temple opening movement-clipping correction

The bug was reproduced without a jump pad: approaching the north edge briefly and backing away left feet at 468.79 on the upper floor while the native tile still referenced the -468.79 lower floor (room 4 instead of room 11). A second independent geometry check caught the body entering the floor lip while falling and reversing. Corner-jump cases also exposed a partial landing clearing traversal protection too early.

The fix restores the supporting native floor layer before normal collision, keeps traversal protection through partial landings and added jumps, and ends it only at stable full-footprint support. Front/back and side sweeps use a circumscribed 16-point footprint plus center. Toe-level and vertical destination probes protect thin slabs; intersected room geometry is prepared independently of what the camera has cached.

Final native movement run: 64 completed routes, 10,240 movement steps, zero wrong-floor settled states, zero detected solid-geometry crossings and zero escaped-floor failures. Coverage includes all four sides, six approach/retreat timings per side, eight corner approaches with strafe/sprint/jump/crouch variants, and eight pad launch directions reaching the upper floor. A deferred audit replays recorded movement against background triangles after each route, avoiding ahead-of-movement cache warming. Settled native anchors must contain the player. A representative room-transition renderer readback was also visually reviewed.

The normal Facility jump regression passed eight ceiling contacts/landings, low-headroom takeoff rejection and intro request suppression. Live-bot pad validation includes missing-render-cache actor fixtures and an independent background-geometry check during movement. Bot-driven knockback is allowed to change the landing destination; the isolated pad test retains the upper-floor destination requirement. Bots are disabled for the final deterministic low-ceiling fixture.

This is extensive local coverage of Temple’s opening, not an exhaustive guarantee for every map, dynamic object or cross-PC interaction. All test traces and renderer captures remain local/ignored.

Final live-bot run passed the independent movement geometry audit, three missing-render-hit-cache actor fixtures, deployment, upper-floor landing, reuse under bot damage, invalid placements, rapid B cancel/LB reselect, ammunition preservation and the isolated ceiling/reset fixture. The scripted UI actions now wait for the published snapshot. An explicit wheel selection is compared with the current item at release, allowing rapid reselect after cancellation without redundant equips from an untouched wheel.

The final neutral-wheel regression also passed its center/reselection checks, 20 alternating switches, 100 reopen/cancel cycles, ownership isolation, inventory changes and GL checks after the rapid-reselect correction.

## Confirmed-hit health display

The supplied 1280x720 MP4 contains 2,652 frames over approximately 44.18 seconds, with frame timestamps near 60 fps. Frame-level review of the first stationary burst and the final moving-target sequence showed a full-opacity hold of approximately 3.25 seconds after the final hit and an approximately 0.20-second fade. In the stationary sequence, the final damage update occurs near 20.89 seconds, full visibility lasts to about 24.14 seconds, and the bar becomes invisible near 24.32 seconds. Compression/frame sampling limit exact fade-shape recovery. The implementation uses a quick quadratic ease-out approximation and named 3.25/0.20 timing constants. Original footage and derived frames remain local and ignored.

The native Temple test passed two confirmed real-gunfire hits and timer refresh, full opacity through the measured hold, intermediate fade opacity and clean expiration. Recorded wall time from the refreshed hit to expiration was approximately 3.45 seconds (within one simulation tick). Misses, invulnerable hits, armor-only damage, foreign damage, independently damaged targets, lethal fade, removed prop identity, pause/time freezing, new-life cleanup, context reset, behind-camera rejection, and solid-deck versus open-atrium sight checks passed. Native gunfire is dispatched through the engine trigger path after normal input processing, so the test does not depend on a physical controller. Frozen-bot fixtures clear the native damage-display cooldown before synthetic strike cases; ordinary play never does.

Facility mission-guard tests exercise native damage and compare actual remaining-HP ratios, guard armor stored as negative damage, and damage detection above the visible armor-strip capacity. Health/render state is published across the simulation/render boundary without sharing engine pointers. Production-renderer hit/fade readbacks are visually reviewed, and the native health run checks OpenGL errors.

Coverage is local offline native gameplay and explicit lifecycle fixtures. Separate-PC attribution/health replication and every weapon/map combination still require broader playtesting; no networking protocol or damage behavior was changed.

After the health overlay was added, the live-bot Jump Pad suite passed deployment, upper-floor landing, reuse under bot damage, three missing-render-cache actor fixtures, independent movement-geometry checks, invalid placement, rapid cancellation/reselect, gun/ammo preservation and ceiling/owner reset. The complete neutral-wheel regression also passed its 20 alternating switches, 100 rapid reopen/cancels, ownership isolation, inventory changes and rendering checks.

Facility’s jump/ceiling regression passed eight takeoffs, ceiling stops and landings, low-headroom rejection, and 557 suppressed intro requests with an empty jump queue at gameplay handoff. The final DLL built with -Wall -Wextra -Werror.

## Larger pad and GoldenEye-style presentation

The supplied jump-pad reference has 509 frames at approximately 60 fps over 8.47 seconds. It shows selection, carry, floor preview, placement and gun return; it ends before a complete launch arc. The extension retains the previously tested 26-unit impulse rather than inferring an unsupported airtime measurement.

The pad’s radius increased from 48 to 78 units (96 to 156 in diameter). Native acceptance passed full-footprint clear-floor placement and rejection of an upper-rim position whose center was supported but whose larger edge overhung Temple’s opening. Contact follows the visible octagon, including rejection outside its corners. The real launch occurred 77.30 units from the center, beyond the old 56-unit trigger extent. Removing the old placement arming delay makes a newly placed deck responsive at first contact.

The enlarged pad passed isolated and live-bot equipment/RT placement, native upper-floor landing (peak 748.04, landing 468.79), reuse, airborne/missing-floor rejection, cancellation/reselect, ammunition preservation, real-ceiling stopping and owner reset. The live suite includes three missing-render-cache actor fixtures and an independent movement-geometry check. The full 64-route Temple deferred audit passed 10,240 recorded movement steps, eight pad launch directions, zero escaped-floor failures and zero detected solid-geometry crossings.

Carry/stow and short deck-compression feedback use simulation-published values. The gun drawing waits until the carried device lowers. The world material has perspective-correct UV interpolation, and the generated texture upload preserves pixel-unpack state. Original low-poly geometry, small generated rubber/paint texels, muted metal colors and original 5x7 lettering replace the modern case/panel treatment. The wheel icon matches the octagonal deck.

The native health suite passed again with the restyled green/amber/red health cells, blue armor and pixel labels: actual gunfire, hit refresh, 3.25-second hold, 0.20-second fade, misses, armor, invulnerability/foreign damage, multiple targets and lifecycle/sight checks. Nine wheel previews and six health-state previews passed OpenGL checks; full/damaged/amber/critical/armor/fade states and native carry/preview/placed/hit/fade readbacks were visually reviewed. Strict -Wall -Wextra -Werror compilation passed for the DLL and preview executable.

All reference videos, renderer readbacks, traces and generated diagnostic sheets stay local and excluded. This does not add shared cross-PC pad objects, a new networking protocol, or exhaustive all-map collision coverage.

Final control regressions passed 20 alternating equips, 100 rapid wheel reopen/cancel cycles, neutral-center and ownership checks, inventory pickup/removal and OpenGL rendering. Facility passed eight native jumps/ceiling contacts/landings, low-headroom rejection and 558 intro requests suppressed without physics changes or a queued jump at handoff. Standalone A/sprint/slap/RT edge tests also passed.

## Fixed gadget slots and native grenades

Strict DLL and preview builds pass -Wall -Wextra -Werror. Jump Pad uses synthetic ID 128 so native hand-grenade ID 26 is free. Production input and drawing share the same angular bounds: grenades right, pad bottom, guns upper/left. A hidden-window test verifies cardinal selection and icon/sector agreement with zero through 21 guns. Nine equipment previews and six health previews pass OpenGL checks; the four-slot equipment layout was visually reviewed.

The isolated native Temple grenade acceptance passed: exactly three on the first playable life, wheel-to-pad-to-grenade switching, native equip, three actual trigger-driven throws consuming one each down to zero, no refill after waiting or switching, empty-slot rejection, unchanged gun ammunition, and re-enabling the right slot after a native inventory/ammo pickup fixture. Empty grenades remain visible with a zero counter.

Wheel regression passed 20 alternating equips, 100 rapid open/cancel cycles, neutral-center and same-frame recenter cancellation, foreign-player snapshot isolation, dynamic pickup/removal and rendering. Jump-pad regression passed placement, launch to the upper Temple floor, reuse, invalid placement, B cancel/reselect, gun/ammo restoration, ceiling stopping and owner reset with the new item ID and slot layout. No collision/health timing changes were made.

These checks use a separate offline installation. Real cross-PC grenade replication, host/client authority and every multiplayer loadout remain unverified; no network protocol changes are included. ROMs, binaries, previews and logs remain ignored.

## Selected-item label placement

The single selected name is now anchored outward from its icon rather than at a fixed top heading. Left/right labels align toward their icon, top/bottom labels center on it, and viewport margins bound the text. Neutral center shows no name. Strict DLL/preview compilation passed; nine wheel states and six health previews passed OpenGL checks. Left, top, bottom and right selection previews were visually reviewed on dark/light backgrounds. Input, angular slots, ammo, physics and health behavior are unchanged.

## Medium-style movement tuning and sprint-crouch slide

Native displacement telemetry in an isolated Temple fixture measured forward run 4.733 m/s, forward sprint 6.368 m/s and diagonal run 4.518 m/s, using an explicit approximate 100-units-per-meter scale. These are extension measurements, not a claim of exact movement parity or an officially published Medium preset. Run scale is 1.052; the nonlinear native gait required a 1.46 sprint scalar ratio. Normalized diagonal input removes the classic extra-speed diagonal, while shot-push vectors bypass normalization.

Eleven native movement cases passed: run/sprint/diagonal displacement bounds; sprint-crouch entry; neutral-stick coasting and timeout; A into native jump; wheel cancellation; reverse braking; walking crouch without a slide; B cancellation; simulated death-state cleanup; and airborne crouch rejection without a queued landing slide. Entry fixtures apply crouch before the extension, matching the custom input adapter's order. A moving-grounded requirement rejects stationary entry. Native input-edge checks passed for B press/hold/release and blocked intro/menu handoff, alongside existing A/sprint/slap/pad-trigger checks.

The 76-route expanded Temple deferred-geometry audit passed during tuning with zero escaped-floor or solid-geometry failures. The final binary then passed all twelve slide approaches across the four opening edges/corners and eight pad directions: 20 completed routes, 2,196 traversal samples, zero escapes and zero physical failures. All twelve slide entries were observed. Native room/camera transition samples are recorded separately and are not treated as physical collision failures. A native slide framebuffer readback was visually reviewed; stance/camera lowering comes from GoldenEye's crouch handling, without a new camera offset.

Strict DLL and preview builds passed -Wall -Wextra -Werror. Nine wheel states and six health-state previews passed OpenGL checks. The supplied footage does not measure a complete slide curve, so the 0.85-second deceleration is a tuning choice. Physics remains native movement scalars, collision, floor support and gravity, with no position teleportation. Cross-PC authority/replication, all-map/slope behavior and physical-controller feel still need real multiplayer/playtesting.

The broader forced-dual quick-slap fixture was also checked in Temple practice. It crashes during case-1 dual-pistol preparation inside native texLoad (supported-release RVA 0x7a827). The unchanged installed main build reproduces the same crash, separating it from the movement changes. This artificial forced-dual practice scenario is recorded in docs/ROADMAP.md; normal wheel equipping clears the left hand and does not perform that setup. Facility is unsuitable for the fixture’s initial sprint assertion because its start requires crouching. No native texture-loading patch is included.

The final DLL passed all six native quick-slap cases in Bunker1 (-level_09), including dual-pistol magazine/reserve preservation, sniper palm override, rifle return, wheel deferral and active-request rejection. Sprint cancellation also passed there. This validates the existing melee path in a mission environment without claiming the forced-dual Temple practice fixture is fixed.

## Full-resolution zoomed controller aim

Source inspection found the existing centred controller path truncates to an integer N64-scale axis, truncates sensitivity again, rounds its EMA and then applies the turn curve. The new ADS adapter consumes the original 16-bit samples, retains fractional response/remainders and integrates a 30 ms exponential velocity response by simulation time. Gain is 0.80 relative to the existing sensitivity/FOV curve. Radial deadzone and exact integral are continuous; idle stops and opposite-direction input cannot retain the previous velocity tail. Hip look uses the existing native path.

Strict DLL and preview builds passed. Math checks passed for all successive raw values 4001..4699, equivalent one-second response at 30/60/144 Hz, FOV and pitch convention, jitter attenuation, center stop and reversal. Twelve native cases passed over 1,440 ticks: fine input ramps, jitter, release, diagonal, foreign player and life/context, wheel and stale input, native ADS masking/EMA clearing, raw hip-fire passthrough, N64-mode passthrough and wheel masking. The input callback publishes values only; camera writes occur in the local simulation thread. Tests include synthetic input fixtures rather than claiming a physical-controller subjective feel check.

Production-renderer checks passed all nine wheel and six health states. This change does not alter gun damage, movement physics, game assets or network protocol. Cross-PC behavior and subjective controller feel remain unverified.

Final compatibility checks also passed 20 alternating equipment switches, 100 rapid reopen/cancel cycles, neutral-center and foreign-player isolation, pickup/removal, and all eleven native run/sprint/slide/jump cancellation cases.

## Raised wall-opening traversal

The reported Temple position was reproduced in an isolated copy. A baseline retained the old restriction and stopped at x=593.64 despite clear geometry above the sill. Native background probes identified the wall face at x=623.64 and the physical sill near y=1092.42. The previous helper admitted only higher destination floors; the thick opening also lacks an ordinary floor-plan tile.

The pad-flight helper now permits same/lower destination floors after the existing full-body 3D sweep. Missing navigation inside a clear, physically supported opening can use a verified existing floor tile within 640 units as a temporary anchor. A signature-checked correction suppresses only the native post-move 2D clamp at an exact accepted point. The local bridge-position bounds check avoids native out-of-bounds recovery while traversing the aperture. Physical background support, not the distant navigation anchor, supplies a floor under the body. Native velocity, gravity and per-frame offsets remain in use.

Final static checks passed for a full-body aperture, blocked lower wall, blocked upper frame, adjacent solid wall, straight jamb grazing and real sill support. Eight native routes passed: forward and reverse crossings; stopping, resuming and retreating on the sill; a solid-wall approach; jamb grazing; a low approach; and the reported near-jamb line. All recorded body paths were independently checked against background triangles. Crossings landed in room 19 at y=0 from room 11's y=468.79, and returned to room 11 at y=468.79, with no fall damage/death. The jamb-grazing route may slide into a physically clear part of the opening; it is validated by the geometry audit rather than an arbitrary must-stay-blocked assertion. A native renderer readback after entry into the destination room was visually reviewed.

The broader 76-route Temple deferred audit passed during development: 7,848 traversal samples, zero escaped-floor failures and zero detected solid-geometry crossings. This includes original central-gap walking/crouch/jumps, twelve slide approaches and eight pad directions. Final opening tests also passed after replacing anchor-derived support with actual background support only. Cross-PC behavior, every map/opening and arbitrary moving-world geometry are not exhaustively verified. ROM data, captures and probe tables remain local and ignored.

The final DLL also passed existing pad equipment/RT placement, ammunition preservation, native upper-floor landing, reuse, invalid placement, cancellation/reselect, real-ceiling stopping and owner-reset regression. Strict -Wall -Wextra -Werror compilation passed.

## Grounded escape after a blocked pad flight

The user's previous session log lacked the portal hook marker, confirming an older extension was still loaded. Separately, an actual post-landing deadlock was reproduced on the current opening build: after a blocked jump at x=592.71, traversal remained active and forty frames of backward input produced no escape. The earlier window suite stopped at landing and did not cover this action.

The cause was the 31-unit conservative traversal footprint at a native 30-unit wall contact. Grounded cleanup now verifies a real containing floor tile and classifies every unsupported toe sample: static wall obstruction may release traversal, whereas a real open ledge must retain it. The check uses background geometry rather than actor/temporary-prop bounds. Native collision then handles the next ground step.

Four native regression cases passed: solid-wall and reported jamb-side blocked jumps, each with the pad removed or retained at contact. Each backed away approximately 262–264 units over forty input frames, including the retained-pad cases that immediately rebounced. No health override or damage was added. The log now names the revision to make loaded-build verification explicit.

The ledge/slide cases 0–67 completed with the new grounded guard. The long session then became gameplay-unavailable during the first pad route; the recorder had continued appending repeated samples while its input-side tick was paused. Its after-step guard now matches the input guard and reports unavailable gameplay explicitly. The final eight pad routes passed in a fresh session: eight upper-floor landings, zero escapes and zero physical failures. The 76 distinct routes are therefore covered across the two sessions rather than claimed as a single uninterrupted pass.

The final DLL also passed all eight high-opening routes and static clearance/sill checks after the grounded cleanup change. Strict -Wall -Wextra -Werror compilation passed.

## Full release map sweep

The installed release's actual multiplayer table was inspected rather than assuming the upstream source's original eleven-map list. Twenty multiplayer setups and twenty mission levels completed the mechanics sweep. The matrix combines core smoke tests, extended health/reload and radial tests, and targeted reruns after the fixes; it is not one uninterrupted forty-setup session. Each setup has eight completed radial routes, for 320 designated routes across both modes.

| Setup | Multiplayer entry: PASS | Mission stage: PASS |
|---|---:|---:|
| Dam | 17 | 33 |
| Facility | 7 | 34 |
| Runway | 14 | 35 |
| Surface I / Surface MP | 16 | 36 |
| Bunker I | — | 09 |
| Silo | — | 20 |
| Frigate | 20 | 26 |
| Surface II | — | 43 |
| Bunker II / Bunker MP | 8 | 27 |
| Statue | 12 | 22 |
| Archives | 9 | 24 |
| Streets | 15 | 29 |
| Depot | 18 | 30 |
| Train | — | 25 |
| Jungle | 19 | 37 |
| Control | — | 23 |
| Caverns | 10 | 39 |
| Cradle | 13 | 41 |
| Aztec | — | 28 |
| Egyptian | 11 | 32 |
| Temple | 1 | — |
| Complex | 2 | — |
| Caves | 3 | — |
| Library | 4 | — |
| Basement | 5 | — |
| Stack | 6 | — |

Core checks exercise real native movement while aiming/turning, jumping and landing, sprint-to-slide, quick slap with gun/ammo return, actual wheel selection, pad deployment and launch/landing, one native grenade throw, confirmed target damage, timed hold/fade/expiration and native magazine reload without ammo creation. The original eleven multiplayer maps received a core pass followed by an extended pass; Silo and Cradle coverage is similarly composed from complementary successful phases. Script-invulnerable targets must not invent damage confirmation; Cradle then uses an actually damageable target for the timed bar check.

Two gameplay defects were reproduced and fixed:

- Caves remained in extended traversal after a jump onto a slope: feet at 1260.92 while the actual containing floor was 1248.24, a roughly 12.68-unit hover. The settled-floor test also failed on a Silo route. Highest-toe support was never uniform on these surfaces. Connected native tile walks now permit grounded handoff; the exact failing Caves and Silo fixtures pass all eight radial routes after the change. Disconnected ledge layers and airborne bodies retain their traversal checks.
- A native-tick context fixture changed stage while retaining the player allocation; the old combat code kept melee phase 1 and cooldown 11. Stage change, clock rewind and allocation change now clear melee/sprint state and queued requests. All three lifecycle cases pass after the fix.

Fixture corrections are separated from gameplay changes. Facility's ordinary grounded scenery contact reproduced with added ledge admission disabled; strict background body auditing therefore applies to extended traversal, while native grounded movement retains its own collision semantics. Incoming guard damage and shot push are isolated to keep route inputs deterministic. Control retries blocked sprint headings and logs a contact relocation if native actors/doors prevent walking directly to a placed pad. Frigate's boat start and Cradle's distant narrow walkways require a broader search over real map floors. Archives and Jungle use recorded open-floor fixtures. Test-only relocations are excluded from body-path auditing.

These checks sample routes in every setup; they do not constitute complete mission playthroughs, every possible room/door/gadget combination, or cross-PC replication tests. Incoming damage isolation means the broad sweep is not a fall-damage validation. The separate normal-health Temple pad regression covers launch/landing without that override. ROMs, extracted assets, screenshots, binaries and raw logs remain ignored.

Final focused regressions also passed wheel neutrality/B cancellation, 20 alternating switches, 100 rapid reopen/cancels and foreign-player isolation; six native melee cases including dual magazines and sniper palm return; original pad equipment, upper-floor landing, reuse, invalid placement/cancel, real-ceiling stopping and owner reset at normal health; and all eight high-opening routes. Temple's long 76-route regression was partitioned after the external runner watchdog stopped it: routes 0–58 completed their deferred audits before route 59 began, then routes 59–75 passed in a separate run with zero escapes and physical failures. The interruption was a test-runner time limit, not a recorded gameplay failure.

The final exact binary also passed all four post-landing wall-escape cases, all three combat lifecycle cases, and eight radial routes at each of the original Caves and Silo failing anchors. The coverage audit found no missing core phase or radial route in any of the forty setup records.

## Character selection portrait clipping

The native portrait plane is tilted, but its orthographic near/far range was only 1..10. This clipped the upper panel completely and left chest-only strips in lower panels. The extension widens depth to -64..64 at the signature-checked portrait projection call only; layout, UVs, scrolling, scissoring and gameplay projections remain native.

Thirty isolated native menu runs passed: 2, 3 and 4 local players at 640x480, 960x540, 1920x1080, 2560x1440 and 3440x1440, each with safe-area cropping off/on. Every run scrolled all 64 fixture-unlocked roster entries and checked full portrait border heights, first/last boundaries, analog/D-pad navigation, A enlargement, B before/after selection, independent player selection/cancellation, duplicate rejection and persisted choices after all-ready. Production roster unlocks are unchanged. An additional four-player run passed with physical input isolated and visual captures of the return screens.

The negative control GE32_PORTRAIT_BASELINE=1 reproduces the old rendering and fails the head/border visibility check. Framebuffer captures were visually reviewed. These are local menu fixtures, not cross-PC lobby tests.

GE32_CHARACTER_TEST=1 enables the native fixture in a separate prepared runtime; GE32_CHARACTER_PLAYERS=2..4 selects its layout and GE32_CHARACTER_VISUAL=1 saves ignored framebuffer captures. It replaces native input getters and masks external input only in the opted-in process, then exits on success/failure. Keep test flags unset in normal play. Startup revision: character-portraits-v5.
