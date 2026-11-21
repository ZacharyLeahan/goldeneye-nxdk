# GoldenEye multiplayer fork

This downstream fork keeps the upstream PC-port source and adds the local GE32 extension in [mods/the-finals](mods/the-finals/README.md). The [original Xbox build](#original-xbox) uses the same source checkout.

The native Windows and original Xbox builds share the LAN protocol, multiplayer menu and movement controls from this checkout. The separate GE32 extension targets the 2026.11.07 e8859fe9 Windows release with reversible executable patches. The native builds run independently of GE32.

## Current additions

- Hold LB for an equipment wheel: Jump Pad fixed at the bottom, native grenades fixed on the right, and owned guns above/left. Each local life starts with three grenades; RT throws. Select with the right stick and release to equip; center keeps current equipment and B cancels.
- Weapon silhouettes, translucent panels, readable labels, an equipped marker, and a selected-slot outline.
- Thread-safe local-player inventory snapshots and queued game-thread equipment changes.
- A jumps using native gravity and floor landing, with grounded-only takeoff and ceiling checks. B toggles crouch; A also clears crouch before jumping. Y interacts.
- Slower controller look: horizontal 65%, vertical 60%, smoothing 3/10. Zoomed centred aim uses full-resolution stick input, time-based smoothing and a finer ADS gain.
- B backs out of the standard character-selection screen. Online character selection is unchanged.

- Calibrated Medium-style run/sprint pacing, normalized diagonal input, and left-stick-click sprint with contextual cancellation.
- Sprint then B for a short crouched momentum slide with deceleration, limited steering and A-to-jump cancellation.
- A larger deployable jump pad with a native-era military-equipment look, matched placement/contact footprint, and immediate launch on contact.
- Pad flight through physically clear raised room openings, with temporary navigation bridging and real window-sill support.
- GoldenEye-inspired pixel lettering, green health cells and blue armor indicators, with no copied game assets.
- Confirmed-hit health bars above targets, with independent timers and a 3.25-second hold plus 0.20-second fade measured from supplied gameplay footage.
- Right-stick-click quick slap using the native palm animation/strike dispatch, followed by ammunition-safe gun restoration.

Movement and combat additions have local offline coverage; cross-PC synchronization is not yet verified.

See the [extension setup and validation](mods/the-finals/README.md). The original [upstream README](README.md) remains the base-port documentation.

No ROMs, extracted game assets, save files, GE32 binaries, patched executables, DLLs, or compiler distributions are included. Bring your own supported US ROM and obtain GE32 separately from its author. This fork is unofficial and not affiliated with Embark Studios or THE FINALS.

## Original Xbox

The nxdk target in `port/xbox/` builds a single XBE from the game's menu,
level, multiplayer, scheduler and audio code in this checkout. It uses the
shared software RSP and the NV2A renderer from [Perfect Dark X](https://github.com/GTTeancum/Perfect-Dark-X). The build
converts the US cartridge's models, audio banks and Complex multiplayer
resources to the Xbox's little-endian, 32-bit layout.

### Build and boot

Use Python 3, CMake 3.20 or later, and a recursive nxdk checkout. Set
`NXDK_DIR` to its absolute path and activate nxdk. The SDK path needs to
be free of spaces for its compiler wrappers. Put matching LLVM `clang`,
`opt`, `llvm-link`, `llvm-ar` and `lld-link` on PATH. The tested nxdk
revision is `14d5ee97e73347c973f1f57b68b79ec08c9e77f2`.

On Windows, the Xbox build uses MSYS2's `/usr/bin` CMake, Python and
make to invoke nxdk's shell wrappers, with the MinGW64 LLVM tools on PATH.
In the activated SDK shell use `PATH="$NXDK_DIR/bin:/usr/bin:/mingw64/bin:$PATH"`
and `export CMAKE_GENERATOR="Unix Makefiles"` for the game build.

From an activated SDK shell, build the SDK libraries and tools once. Run
the packager from the GoldenEye checkout with your retail US .z64 or ZIP.

```sh
make -C "$NXDK_DIR" NXDK_ONLY=y NXDK_SDL=y NXDK_NET=y all cxbe extract-xiso
python3 port/xbox/build.py --rom "GoldenEye 007.zip"
```

The packager checks the dump against `ge007.u.sha1`. Open
`build-xbox/ge007.iso` as the DVD in a 64 MiB xemu profile and reset.
For an Xbox dashboard installation, copy `default.xbe`, `xbox.bin` and
`ge007.ntsc-final.z64` together from `build-xbox/disc/` into the game folder.
These private outputs contain cartridge data. Running the packager without
`--rom` builds the executable alone.

For the private four-player emulator check, add `--local-four-test`. The
resulting XISO includes `ge-local-four.on`, exposes four idle controller seats
and boots four-player Complex directly. The marker is never packaged without
that explicit flag. An empty marker selects Complex. A decimal number from
1 through 11 selects the corresponding arena in multiplayer menu order.
For repeatable hardware combat in a private QA directory, add `ge-close.on`
to start the four players together. Respawns use the normal spawn selection.
With `ge-profile.on` enabled, `ge-scores.csv` records the stage ID followed
by 16 kill counts, ordered by attacker and then victim from P1 through P4.

On a physical Xbox, launching `default.xbe` from XBMC has been verified
through file select and into two-player split-screen Complex. The test used a
controller in port 1 and direct IP-KVM input in port 2 to turn and walk the
second player. The fork-controls build was also tested in a physical two-player
match using White, right-stick up, and A on port 2. In a subsequent Complex
match both players independently turned and threw grenades using the fork
wheel. The hardware recorded four grenade launches. The three seven-second
samples containing throws averaged 59.8Ã¢â‚¬â€œ60.0 FPS with 19Ã¢â‚¬â€œ20 ms p95 frame
intervals.

A private xemu image built with `--local-four-test` booted Complex with four
simultaneous viewports, four distinct spawn positions and four player markers.
On original hardware, all four controller ports responded independently on
Complex. Simultaneous grenade deaths reproduced a music sequence assertion.
The Xbox sequence-state shim now accounts for queued playback when another
death requests the same sequence player.

The original Xbox completed a four-controller combat round on each of the
eleven arenas using wired P1 and wireless receivers for P2 through P4. The
private test used a common initial spawn so a designated shooter could hit
the other three players. Each arena recorded all four physical ports and a
kill against each opponent. For Caverns, the earlier apparent freeze left
the renderer running while the game saw only P3 and P4. The Xbox host now
retries attached USB devices that failed to enumerate, and the input layer
opens controllers by their current physical port after a connection change.
A subsequent Archives launch in the private test folder stayed black before
gameplay. The earlier Archives round completed and recorded three opponent
kills. The cause of the later launch failure is unconfirmed.

#### Four-player frame times on original Xbox

The scored combat sweep measured 10,996 completed frames over 322.79 seconds
with 19,371 vertical blanks. The game averaged **34.07 FPS**. All eleven
arenas missed the 60 FPS target. The profiler records intervals between
completed frames. Display refresh counts alone cannot establish how many
distinct frames reached the screen.

| Arena | Average FPS | FPS in windows with grenade throws | Worst sampled p95 (ms) | Longest frame (ms) |
|---|---:|---:|---:|---:|
| Caverns | 24.5 | 17.0 | 86 | 286 |
| Facility | 36.0 | 22.9 | 69 | 333 |
| Stack | 36.8 | 22.1 | 72 | 897 |
| Basement | 37.0 | 23.5 | 70 | 435 |
| Library | 35.6 | 22.4 | 82 | 971 |
| Caves | 36.1 | 21.4 | 75 | 591 |
| Complex | 35.4 | 23.3 | 69 | 713 |
| Temple | 38.9 | 22.9 | 75 | 351 |
| Bunker | 35.4 | 20.4 | 75 | 368 |
| Archives | 35.7 | 21.7 | 70 | 574 |
| Egypt | 28.2 | 21.9 | 73 | 355 |

Each row combines seven-second windows from one match with a shared initial
spawn. The first second and writes between windows are excluded. FPS uses
total frames divided by total sampled time. The p95 column reports the
highest individual window's 95th percentile. Windows with grenade throws
include the surrounding gameplay. These short runs do not measure long-match
stability or typical play from random spawn positions.

Across the matched renderer samples, the interval from back-buffer acquisition
to the start of the final GPU drain averaged 20.82 ms. This includes 4.03 ms
in draw submission. The subsequent drain averaged 1.95 ms and presentation
averaged 0.11 ms. The pre-drain interval alone exceeded the 16.67 ms total
frame budget.

A separate four-controller Caverns firefight measured **17.01 distinct
displayed frames per second** across 710 sampled refreshes at 60 Hz, with
no gaps in refresh sampling. The median display interval was 50.0 ms and
p95 was 83.33 ms. One image stayed on screen for 65 refresh periods.
Of the repeated refreshes classified by the trace, 341 occurred during
display-list work outside the backend draw callback, 97 inside that callback,
59 while constructing the next game frame and four during the final GPU drain.
On the longest game frame, three seats spent a combined 983 ms after
background rendering and before finishing their view. These are wall times.

A replay of the same four-player grenade exchange traced the long game-thread
pause to three calls that restarted the death music. Each call waited for the
previous sequence to stop. The Xbox port now yields the CPU to the audio
worker while waiting. With busy polling, the three waits totaled 570.57 ms
and the longest game frame took 579.32 ms before submission. With the yield,
the waits totaled 58.64 ms and the longest game frame took 67.39 ms. Both
physical runs recorded kills against all three opponents. The longest
observed interval between new images fell from 38 refresh periods to six.
The p95 display interval was still 83.33 ms in both runs.

A stationary four-player Caverns comparison alternated the existing
material-reuse optimization on and off across eight three-second blocks.
It raised displayed FPS from 17.44 to 17.54. Both conditions had a 66.67 ms
p95 interval. The release configuration retains its existing material behavior.
A separate trial reused the packed vertices of consecutive triangles. It
measured 17.23 displayed FPS against 17.35 FPS with the regular writer.
The trial was discarded.

To capture local display timing, place `ge-split-trace.on` and
`ge-profile.on` beside a private profiling build, enter local multiplayer,
and retrieve `ge-frame.bin` from the game folder. The trace starts after a
two-second warmup and runs for twelve seconds. Analyze it with
`python port/lan/tests/frame_trace.py ge-frame.bin`. It joins game frames,
render tasks and scanned surfaces and reports timing for each active seat.
In the private local test, adding `ge-close.on` and `ge-local-volley.on`
replays a grenade exchange from the same spawn and ties its inputs to the
trace start. Omit the volley marker for physical-controller testing.

#### Local multiplayer release measurements

The original Xbox ran a repeatable close-range grenade exchange in each
arena. Inputs came from the test script. Each result requires the selected
arena and seat count in the console log, a kill against every opponent, and
a trace that accounts for every interior rendered frame on the display.
Each cell gives distinct displayed FPS over a twelve-second capture and
the 95th-percentile interval between displayed frames in milliseconds.
At a steady 60 FPS that interval would be 16.7 ms. These short combat
samples use the sequence-stop fix and normal rendering with the overlay
disabled. The earlier four-controller sweep used physical input and
measured render completions over longer rounds.

| Arena | 2 players FPS / p95 ms | 3 players FPS / p95 ms | 4 players FPS / p95 ms |
|---|---:|---:|---:|
| Temple | 50.5 / 33.3 | 45.1 / 50.0 | 23.7 / 83.3 |
| Complex | 50.7 / 33.3 | 37.8 / 50.0 | 22.4 / 66.7 |
| Caves | 49.7 / 33.3 | 41.9 / 50.0 | 21.9 / 83.3 |
| Library | 49.6 / 50.0 | 39.8 / 50.0 | 25.0 / 66.7 |
| Basement | 49.9 / 50.0 | 37.3 / 50.0 | 22.2 / 66.7 |
| Stack | 50.4 / 33.3 | 40.1 / 50.0 | 25.7 / 66.7 |
| Facility | 49.6 / 50.0 | 44.7 / 50.0 | 24.5 / 66.7 |
| Bunker | 48.9 / 50.0 | 36.3 / 50.0 | 20.1 / 83.3 |
| Archives | 50.5 / 33.3 | 34.5 / 50.0 | 21.2 / 83.3 |
| Caverns | 42.9 / 50.0 | 27.8 / 66.7 | 20.4 / 66.7 |
| Egypt | 44.1 / 50.0 | 36.5 / 50.0 | 18.7 / 83.3 |

In four-player Caverns the CPU spent 33.0 ms per rendered frame processing
the visible display list outside the backend draw callback. The callback
took another 8.8 ms and the final GPU drain averaged 1.4 ms. Display-list
processing alone exceeded the entire 16.7 ms frame budget.

Separate KVM runs exercised controller input for about 70 sampled seconds
per arena at two and three players. At each player count, eight of eleven
rounds registered kills against every opponent. A three-player Caves launch
stayed black before gameplay. The retry reached gameplay and scored. A
separate four-player Caverns overlay test recorded all three opponent kills
before the console stopped responding on return to XBMC. Both incidents
required a power cycle. Their causes remain open.

A longer Caverns match on the overlay-capable build used normal spawn
positions, the overlay hidden and the four physical controller routes.
Each port took two turns as the grenade thrower. The console recorded
12 throws and a self-kill for each seat, then returned to XBMC through
the controller reset chord. The opening trace measured 24.82 displayed
FPS with a 50.0 ms p95 interval. The separate completion logger averaged
55.01 FPS across 238.31 sampled seconds, including time on respawn screens.

#### Performance overlay

Create an empty file named `ge-perf-hud.on` beside `default.xbe` and launch
the game. The overlay appears once at the top of the screen in single-player
and split-screen. Put `test` at the start of the file to show the longest
display interval in the last five seconds. Remove the file and relaunch to
hide the overlay.

FPS counts distinct scanned framebuffers over the last second. The graph
shows five seconds of intervals between new images. Horizontal guides mark
16.7 and 33.3 ms. Above 33.3 ms the vertical scale compresses to keep long
hitches visible. When several frames fall in one column its peak survives.
The overlay refreshes its text and graph about four times per second.

**Prep** measures elapsed time from acquiring the back buffer through
command preparation and overlay drawing. **GPU wait** measures the final
wait for submitted work to drain. Both are averages of the frames since
the last overlay update. **Free** is available physical memory in MiB.
The overlay uses the completed display intervals directly and works
independently of CSV logging and frame tracing.

In a stationary four-player Caverns hardware test, eight alternating
three-second blocks measured 15.70 displayed FPS with the overlay hidden
and 15.64 with it visible. Both conditions had an 83.33 ms p95, p99 and
longest observed display interval. The capture missed one refresh sample
in the visible condition. This comparison used the testing view.
Separate four-player grenade replays scored against all three opponents
with the overlay hidden and visible. They measured 17.42 and 17.71 displayed
FPS respectively. The alternating blocks above provide the closer comparison
of overlay overhead.

### Play Complex locally

1. Bind two to four controllers to contiguous Xbox ports before entering multiplayer setup.
2. At file select, point the left stick at a dossier and press A. The mode
   page lists **Solo Campaign**, **LAN Multiplayer**, then **Local Multiplayer**. Select **Local Multiplayer** with A.
3. Multiplayer Options opens with **Level Complex** selected. Check that it
   shows the connected count under **Players**. The level picker displays Complex in the top row,
   third tile. On Xbox, the picker offers all eleven arenas with **Players** set to four.
4. Press Start to begin. Each player uses the left stick to move and the
   right stick to look. Pushing the right stick up looks up. A jumps, B
   toggles crouch, Y interacts, X reloads, LT aims, and RT fires. Click the
   left stick while moving forward to sprint. Once up to speed, tap B to
   slide. Release the stick to coast, or press A to jump out of the slide.
   To mantle, face a ledge, move toward it and press A. You can press A again
   in the air as you reach the lip.
5. Hold the **White button** (LB on the Windows profile) to open the equipment
   wheel in your half of the screen. Flick the right stick toward an item and
   release White. Right selects grenades and down selects the jump pad.
   Owned guns use the upper and left sectors. The Xbox remembers the last
   direction for 100 ms as the stick springs back. Holding the stick at
   center clears the selection. B cancels immediately.
6. With the jump pad in hand, look toward the floor. Green marks a valid
   placement. RT deploys the pad and B stows it. On an invalid surface,
   RT keeps the pad in hand. Either player can walk onto the deployed pad.

Windows and Xbox compile the same movement, aim, wheel and equipment sources from `port/controls/`. Each local player has independent input and equipment state. The optional Windows GE32 extension keeps its executable-specific integration in `mods/the-finals/`.

In native Xbox and Windows gameplay, click the right stick to slap with Bond’s palm. After the strike, Bond returns to the equipped weapon. Xbox multiplayer shows an eight-cell health meter above an opponent you damage. A new hit refreshes the 3.25-second hold, followed by a 0.20-second fade.

### Host Complex on a wired LAN

The NTSC-U Windows port and original Xbox port can host each other over direct
IPv4 UDP. Windows can also host another Windows player. Each player has a
full-screen view. Position, facing and gait drive the other player's
multiplayer character animation. Each player starts with a PP7. The host
resolves gunfire and sends both players' health, armour, deaths, kills and
respawns to the client. The host also awards stage pickups and replicates
their visibility, weapon ownership, reserve rounds and magazines.
The separate GE32 release uses a different multiplayer implementation.

The Windows host uses LB for the fork equipment wheel. Hold LB, move the
right stick to an item, then release LB to equip it. B cancels. The Xbox
uses White for the same action. On a focused Windows game window, WASD
moves. Up and Down also walk forward and backward, and Left and Right turn.
Hold Tab and press an arrow to choose a wheel sector. Each player controls
their own equipment.

1. Connect both machines to the same LAN. On Windows, build this revision
   with `./build-pc.sh ntsc-final` and prepare the US ROM and desktop asset
   sidecars as described in `docs/building.md`, or use a prepared Windows
   bundle. Run `ge007.x86_64.exe --lan-host` in the game directory, or select **LAN Multiplayer**, then **Host Match** in the Windows dossier. The
   host waits for a client on UDP 27007. Connect the Windows controller
   before launch. Build and install both executables from the same revision.
   If the host logs `rejected peer build`, compare the two builds.
2. If Windows Firewall prompts, allow the game on the Private network profile.
   The host remains joinable while its waiting screen is visible.
3. On Xbox, pick a dossier, then **LAN MULTIPLAYER Ã¢â€ â€™ JOIN MATCH**. Select the
   discovered host and press A. The list shows its match name, arena, occupancy
   and endpoint. **Refresh** clears stale entries and searches again. If the
   host is not listed, choose **Enter IP manually**; use D-pad left or right to
   select an octet, up or down to change it, and hold LT to step by ten. The
   host and client enter a two-player, bot-free Complex match after the
   handshake. Use controller port 1.
4. At spawn, stand still on Xbox while the Windows player moves across its
   view and turns. Swap roles. Repeat from a fresh launch after unplugging
   only the router's WAN cable. Record each build and both screens. The
   third dossier option, **Local Multiplayer**, starts split-screen play.

To reverse the roles, choose **LAN MULTIPLAYER Ã¢â€ â€™ HOST MATCH** on Xbox. Its
wired IPv4 address appears while it waits on UDP 27007. On Windows, choose
**LAN Multiplayer**, then **Join Match** and enter that address, or run
`ge007.x86_64.exe --lan-join XBOX_IPV4`. The Xbox is then authoritative for
hits, kills, pickups, scores and rematches. This remains a two-player,
one-versus-one match with no bots.

On physical hardware, an original Xbox displayed its wired address, accepted
the Windows client, entered Complex and retained player-one movement, camera
and PP7 fire while hosting. Both machines rendered their own full-screen view.
The paired desktop regression then covered kills, deaths, respawns, reconnect,
pickup contention, both rematch winners and death drops in both directions.

A second Windows player can use **LAN Multiplayer**, then **Join Match** or join with `ge007.x86_64.exe --lan-join HOST_IPV4`.
The host defaults to Complex. Use `--lan-map Temple` to select another arena.
The host also sets the winning score. The default is five points. Use
`--lan-score-limit 1` for a one-point match, or zero for unlimited play.
After a win, both players press Start on their results screens for a rematch.
The physical Xbox joined a Windows host and fired on all eleven retail arenas.
Temple reached its pistol HUD after the portal-table fix. On the other ten
arenas the Xbox rejoined after each host restart and Windows recorded gunfire
from both seats. In Complex the Xbox displayed a 1Ã¢â‚¬â€œ0 result, accepted Start
for a rematch and fired in the new round. Install the
paired `default.xbe` and `xbox.bin` from the same Xbox build.
To repeat the Complex firefight regression on one Windows PC, run
`python port/lan/tests/firefight.py build-pc/ge007.x86_64.exe data` from
the checkout. It starts two game processes on separate test ports, drops
and duplicates UDP packets, and checks a kill and respawn from each seat.
It also checks reconnection and contests over a weapon and an ammo crate.
Each item must disappear, benefit one player and regenerate on both machines.
For death drops the host runs the native throw and pickup. The guest sees the
weapon at its host position. The regression has each player kill the other
and collect the dropped PP7, then checks both ammo totals and removal.
The suite also checks the winning score and rematch for each seat. Run
`python port/lan/tests/restart.py build-pc/ge007.x86_64.exe data` to
restart the guest during a firefight, then replace the host with a Temple
host. The guest retains its position, inventory and score when returning
to the same host. A fresh guest also sees the weapons still on the floor.
A new host starts a fresh match.
Add `--cold-pairs 12` to the restart test to exercise 24 fresh game
processes. Each must deliver the four controller-init timer messages and
enter Complex before the timeout.

On Windows the fork controls also work in local multiplayer. Each controller
operates its own equipment wheel with LB and the right stick. Player one
can also use Tab and the arrow keys. To repeat the split-screen check, run
`python port/lan/tests/splitscreen.py build-pc/ge007.x86_64.exe data`.
It tests two local players selecting and using grenades on every retail
arena, then tests four local players in Complex. Add `--arena Complex`
to run just the two- and four-player Complex cases.

To return to the Xbox dashboard from a menu or match, hold **LT + RT +
Back + Black** together on one controller for 1.5 seconds.

For a Windows distribution bundle, place `port/lan/Host-Complex.cmd` beside
the executable. It starts the host with the LAN address visible in the game window.
With a fresh asset-free bundle, place the retail US ROM at
`data/ge007.ntsc-final.z64`. The launcher uses the bundled Python scripts
and a Windows Python 3 installation for first-run conversion.

After configuring the Xbox build, the wheel regression test can also run
on a host with a C++20 compiler and SDL2. It compares the fork's original
selection function with the Xbox input path and exercises the Xbox release
window, cancel, empty grenades and independent controllers.

```sh
c++ -std=c++20 $(sdl2-config --cflags) \
    port/xbox/tests/wheel.cpp $(sdl2-config --libs) -o build-xbox/wheel-test
build-xbox/wheel-test
```

### Xbox frame pacing

For a hardware measurement, place `ge-profile.on` beside `default.xbe`
in a writable game directory. The Xbox appends seven-second samples to
`ge-profile.csv` after a one-second warm-up. Columns are stage ID, LAN or
split-screen, completed frames, elapsed microseconds, 95th-percentile
and maximum frame interval in microseconds, minimum free KiB, and native
shots across all seats. The four additional columns give the seat count,
successfully launched player grenades, 99th-percentile frame interval in
microseconds, and VBlanks elapsed during the sample, in that order. Earlier
rows end after shots. The disk write occurs between samples. A stage load,
including a rematch on the same arena, starts a fresh timing window.
The same marker enables `ge-render.csv`. Each row covers the same window.
The columns are stage, mode, frames, elapsed microseconds, shots, six mean
durations in microseconds per frame (renderer including submission, NV2A draw
submission, back-buffer acquisition, final GPU drain, present call, texture
upload), then draw calls, input triangles, uploaded RGBA bytes, elapsed VBlanks
and the sample number. Draw, triangle and byte counts are window totals. The
draw and upload durations overlap the renderer duration. Final GPU drain
measures the CPU wait after submission. Remove the marker to disable profiling.

From the repository root, run the stationary Windows regression with
`python port/lan/tests/egypt_firefight.py build-pc/ge007.x86_64.exe data build-pc/egypt-regression`.
It starts two seats on a private loopback port and saves both traces and logs.
Five guest framebuffers check the green ammo crate and the yellow-fill regression.
Set `GE_WAKE_AB=1` before launching to compare the scheduler modes.

For a frame-by-frame Egypt LAN trace, set `GE_FRAME_TRACE=1` on Windows or
place `ge-trace.on` beside the Xbox executable. Keep `ge-profile.on` for the
Xbox geometry and shot counters. A Windows host can set `GE_TRACE_EGYPT_PAD=0`
to select the guest's native start facing the tan wall, with an ammo box on
the left and columns on the right. The other five starts can run at very
different rates. After two seconds of warm-up the game captures 24 seconds
in memory and writes `ge-frame.bin` to the working directory or Xbox game
folder. Return to XBMC to retrieve it. Remove `ge-trace.on` to disable the
capture. Run `python port/lan/tests/frame_trace.py ge-frame.bin`. Each
Xbox refresh observation reads the display controller's selected surface
and joins its generation to the game frame and render task. Seat spans are
wall time and include gameplay work.

Normal LAN play skips draw-only calls for the hidden seat. For an in-match
comparison, enable the frame trace plus `GE_LAN_DRAW_AB=1` on Windows or
`ge-draw-ab.on` on Xbox. The 24-second window compares eight three-second
blocks in the order ABBA BAAB. A draws and discards the hidden view, and B
skips its draw-only calls. The analyzer excludes the first two frames per
block and prints seat times, discarded bytes, display timing and geometry.
Remove the comparison and trace markers after the measurement.

Egypt LAN can also wake the scheduler when a graphics task is submitted.
A real retrace still polls input, ticks music and notifies clients. The wake
allows an eligible task to enter the normal SP/DP schedule sooner. To compare
it with retrace-only scheduling, use `GE_WAKE_AB=1` or `ge-wake-ab.on` with
the same trace setup. Use one comparison at a time. The analyzer reports the
CPU wall time inside the backend draw callback and the remainder of visible
display-list traversal. At each Xbox refresh the interrupt also samples the
active render frame and which of those phases it was executing.

To test material reuse across vertex loads, select `GE_MATERIAL_AB=1` on
Windows or `ge-material-ab.on` beside the XBE with the frame trace enabled.
The eight blocks keep early scheduling active and alternate the vertex-load
optimization. The scheduler and hidden-view comparisons take precedence
when selected. Ordinary Egypt LAN play keeps material state across vertex
loads. Other arenas and local split-screen use their existing setup.

To profile TRI4 at this camera, use `GE_TRI4_AB=1` with the Windows
firefight regression or `ge-tri4-ab.on` alongside `ge-trace.on` on Xbox.
Use only one A/B marker. This experiment compares the original backface
test with exact projected coordinates cached for TRI4 vertex slots.
`GE_VERIFY_TRI4_CULL=1` checks culling decisions on Windows; leave it unset
when timing. The analyzer reports TRI4 time excluding nested backend draws,
commands per frame, interrupt gaps and the time available before the target
refresh. Remove the trace and comparison markers for regular play.

To trace the scheduler handoff in Egypt, use `GE_PAIR_AB=1` on Windows or
`ge-pair-ab.on` beside the Xbox executable together with the frame trace.
Ordinary Egypt LAN play accepts an audio/graphics pair on an early wake.
Both A/B conditions send the existing early graphics wake. The candidate also
accepts a queued pair consisting of one audio task and one graphics task,
appends them in FIFO order and lets the native scheduler select the task.
The analyzer reports submission, command enqueue, servicing interrupt receive,
list append and graphics handoff. A handler already running when the game
enqueues the task contributes zero queue-to-receive time. Wake-post latency
is measured separately, including wakes that find an empty command queue.
Interrupt 666 is retrace, 670 is the control wake, and 671 is the candidate wake.

For a stationary two-seat firing test, put the Windows host's IPv4 address
and UDP port on one line of `ge-test-peer.txt` beside the XBE. With
`ge-trace.on` present, the Xbox joins Egypt, pulses its normal trigger every
650 ms, and requests a dashboard return 75 seconds after joining. The test
uses native guest start 0 and ignores live gameplay input while recording.
The physical reset chord remains available. Set `GE_TRACE_EGYPT_PAD=0` on
the Windows host and give it the same scripted trigger cadence. Check for a
level view of the wall, green ammo box and columns before comparing hardware
intervals. For xemu user networking, put
`10.0.2.2 27008` in the fixture and use `--lan-port 27008` on the host.
With the trace marker removed, a successful join consumes the peer file on
a writable game drive and leaves the controls with the player.

For a trace revision, check a Windows boot with `GE_FRAME_TRACE_BOOT=1` and
`python port/lan/tests/frame_trace.py --boot ge-frame.bin`. Then include
`ge-boot-trace.on` in a private test DVD alongside the normal three game
files. Boot that disc in a 64 MiB [xemu](https://github.com/xemu-project/xemu) profile using the 1 MiB [OXOS v0.4.14 BIOS](https://github.com/kooscode/oxos/releases/tag/v0.4.14),
a writable Xbox disk and a loopback QMP monitor
(`-qmp tcp:127.0.0.1:47771,server=on,wait=off`). After the title has
rendered for six emulated seconds, run the following from the checkout.

```sh
python port/lan/tests/xemu_boot_trace.py --map build-xbox/world.map --port 47771 --output build-xbox/xemu-boot-trace.bin
```

Use the map from the same executable. The check reads the completed buffer
from emulated memory and verifies each interior game, task, render and
displayed surface ID. To exercise the LAN comparison in xemu, start a
separate Windows Egypt host on UDP 27008. With xemu user networking enabled,
put `ge-emu-join.on`, `ge-trace.on`, `ge-profile.on` and `ge-draw-ab.on` on
the private test disc, omitting `ge-boot-trace.on`. The emulator guest joins
the host at `10.0.2.2`. Read the buffer after the match trace completes.
Use the physical Xbox to measure FPS and display timing.

During a physical Xbox LAN sweep, each of the eleven arenas produced at least
one seven-second sample with gunfire. The slowest such sample in Egypt averaged
38.7 FPS with a 36 ms 95th-percentile frame interval. Stack reached 51.1 FPS
and Caverns 52.5 FPS in their slowest firing samples. Temple averaged 59.9 FPS
or better. These measurements cover the tested views and inputs.

In a follow-up on the physical Xbox, a fixed Egypt camera alternated seven-second
windows between the original and cached Fast3D material setup. Across four firing
windows per path, throughput rose from 34.02 to 38.75 FPS, with CPU renderer work
falling from 13.96 to 12.91 ms per frame. Both paths measured a 36 ms p95 frame
interval in every firing window. A lighter Egypt view held 59.9 FPS on both.

During a Complex LAN ammo test, the Xbox HUD showed 7 rounds loaded and
92 in reserve after the pickup. Eighteen shots later it showed 3 and 78.
The seven-second sample containing 13 shots averaged 60.0 FPS, with a
19 ms 95th-percentile frame interval and 19,504 KiB of free physical memory.

The Xbox renderer waits for a free back buffer, and the vertical-blank interrupt
selects completed frames for display. The build applies Perfect Dark X's pbkit
queue-retirement fix to a build-directory copy of the configured nxdk source.
Each split-screen player has a cached equipment-wheel texture. A change in
selection, ammo, owned items or viewport size refreshes the artwork. The NV2A
blends it over the current view each frame.

A physical Xbox comparison used NTSC 640Ãƒâ€”480, two players in Complex, a fixed
spawn seed and the same scripted inputs on each build. Each measurement covers
seven seconds after the one-second transition. Frame times include the game
loop and renderer, sampled at consecutive frame completions.

| Two-player scene | Previous FPS | Updated FPS | Previous 95th-percentile frame time | Updated 95th-percentile frame time |
| --- | ---: | ---: | ---: | ---: |
| Both standing still | 58.7 | 60.0 | 18.1 ms | 18.7 ms |
| Both turning | 50.6 | 57.7 | 33.9 ms | 20.0 ms |
| Both equipment wheels open | 7.5 | 60.0 | 135.9 ms | 18.3 ms |

### Bring Windows upstream changes into the fork

Keep `origin` pointed at this fork. In a fresh clone, add the Windows project
once with `git remote add upstream https://github.com/jkdansereau/goldeneye-pc-port.git`.
With your downstream work committed on `main`, bring in new commits with the following commands.

```sh
git fetch upstream main
git log --oneline HEAD..upstream/main
git merge upstream/main
```

Run the existing Windows build in its MSYS2 environment and the Xbox build
in its nxdk environment after merging. The desktop and cartridge builds have
their own existing entry points. Keep console adaptations in `port/xbox/` and
compile shared files in place. When an upstream interface changes, build the
Xbox target against its new header and adapt the console caller there.
