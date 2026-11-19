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
that explicit flag.

On a physical Xbox, launching `default.xbe` from XBMC has been verified
through file select and into two-player split-screen Complex. The test used a
controller in port 1 and direct IP-KVM input in port 2 to turn and walk the
second player. The fork-controls build was also tested in a physical two-player
match using White, right-stick up, and A on port 2. In a subsequent Complex
match both players independently turned and threw grenades using the fork
wheel. The hardware recorded four grenade launches. The three seven-second
samples containing throws averaged 59.8–60.0 FPS with 19–20 ms p95 frame
intervals.

A private xemu image built with `--local-four-test` booted Complex with four
simultaneous viewports, four distinct spawn positions and four player markers.
That verifies the Xbox build's four-seat local path in emulation. Four physical
controllers on original hardware are still owed.

### Play Complex locally

1. Bind two to four controllers to contiguous Xbox ports before entering multiplayer setup.
2. At file select, point the left stick at a dossier and press A. The mode
   page lists **Local Multiplayer** and **LAN Multiplayer**. Select **Local Multiplayer** with A. On Windows, select **Multiplayer** in the dossier first.
3. Multiplayer Options opens with **Level Complex** selected. Check that it
   shows the connected count under **Players**. The level picker displays Complex in the top row,
   third tile. The picker enforces each arena's native player limit.
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

The Xbox target builds the mantle search and climb path, slide momentum,
look response, wheel selection, layout and artwork
from this fork's sources in `mods/the-finals/`. The nxdk adapter reads each
physical controller separately and applies equipment and movement to that
player in the native game. Changes to those shared sources trigger a rebuild.
Changes to Windows-only hooks need a corresponding native Xbox implementation.

Complex supports two-player local play. Quick slap and hit bars still need native Xbox support.

### Host Complex on a wired LAN

The NTSC-U Windows port can host an original Xbox or another Windows player
over direct IPv4 UDP. Each player has a full-screen view. Position, facing
and gait drive the other player's multiplayer character animation. Each
player starts with a PP7. The host resolves gunfire and sends both players'
health, armour, deaths, kills and respawns to the client. The host also
awards stage pickups and replicates their visibility, weapon ownership,
reserve rounds and magazines.
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
   bundle. Run `ge007.x86_64.exe --lan-host` in the game directory, or select **Multiplayer → LAN Multiplayer → Host Match** in the Windows dossier. The
   host waits for a client on UDP 27007. Connect the Windows controller
   before launch. Build and install both executables from the same revision.
   If the host logs `rejected peer build`, compare the two builds.
2. The Windows host shows the IPv4 address to enter on Xbox. If it shows
   both Ethernet and Wi-Fi, use the connection on the same LAN as the Xbox. If
   Windows Firewall prompts, allow the game on the Private network profile.
3. On Xbox, pick a dossier, then **LAN MULTIPLAYER**. Use D-pad left or right
   to select an octet and up or down to change it. Hold LT to step by ten.
   Press A to join, or B to go back. The host and client enter a two-player,
   bot-free Complex match after the handshake. Use controller port 1.
4. At spawn, stand still on Xbox while the Windows player moves across its
   view and turns. Swap roles. Repeat from a fresh launch after unplugging
   only the router's WAN cable. Record each build and both screens. The
   first multiplayer row, **Local Multiplayer**, starts split-screen play.

A second Windows player can use **Multiplayer → LAN Multiplayer → Join Match** or join with `ge007.x86_64.exe --lan-join HOST_IPV4`.
The host defaults to Complex. Use `--lan-map Temple` to select another arena.
The host also sets the winning score. The default is five points. Use
`--lan-score-limit 1` for a one-point match, or zero for unlimited play.
After a win, both players press Start on their results screens for a rematch.
The physical Xbox joined a Windows host and fired on all eleven retail arenas.
Temple reached its pistol HUD after the portal-table fix. On the other ten
arenas the Xbox rejoined after each host restart and Windows recorded gunfire
from both seats. In Complex the Xbox displayed a 1–0 result, accepted Start
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
c++ -std=c++20 -Ibuild-xbox/core/finals $(sdl2-config --cflags) \
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

A physical Xbox comparison used NTSC 640×480, two players in Complex, a fixed
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
