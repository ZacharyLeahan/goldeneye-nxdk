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
