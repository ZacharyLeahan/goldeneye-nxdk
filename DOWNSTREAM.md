# GoldenEye PC Port — The Finals-inspired controls

This downstream fork keeps the upstream PC-port source and adds the local GE32 extension in [mods/the-finals](mods/the-finals/README.md). The [original Xbox build](#original-xbox) uses the same source checkout.

The playable multiplayer build used here is GE32 2026.11.07 e8859fe9 for Windows, a separate unofficial fan fork of the base PC port. Its multiplayer source is not included in this checkout. The additions are a native Windows extension and reversible executable patches for that exact GE32 release; they are not integrated into the upstream port build.

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
shared software RSP and the NV2A renderer from Perfect Dark X. The build
converts the US cartridge's models, audio banks and Complex multiplayer
resources to the Xbox's little-endian, 32-bit layout.

### Build and boot

Use Python 3, CMake 3.20 or later, and a recursive nxdk checkout. Set
`NXDK_DIR` to its absolute path and activate nxdk. The SDK path needs to
be free of spaces for its compiler wrappers. Put matching LLVM `clang`,
`opt`, `llvm-link`, `llvm-ar` and `lld-link` on PATH. The tested nxdk
revision is `14d5ee97e73347c973f1f57b68b79ec08c9e77f2`.

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

On a physical Xbox, launching `default.xbe` from XBMC has been verified
through file select and into two-player split-screen Complex. The test used a
controller in port 1 and direct IP-KVM input in port 2 to turn and walk the
second player. The fork-controls build was also tested in a physical two-player
match using White, right-stick up, and A on port 2.

### Play Complex locally

1. Bind controllers to Xbox ports 1 and 2 before entering multiplayer setup.
2. At file select, point the left stick at a dossier and press A. The mode
   page lists **COOP CAMPAIGN**, **LAN MULTIPLAYER** and
   **LOCAL MULTIPLAYER** in that order. Select the third row with A.
3. Multiplayer Options opens with **Level Complex** selected. Check that it
   shows **Players 2**. The level picker displays Complex in the top row,
   third tile, and dims arenas without Xbox assets.
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

Complex supports two-player local play. Quick slap, hit bars and co-op still
need native Xbox support.

### Host Complex on a wired LAN

The NTSC-U Windows port in this checkout can host one original Xbox. Each
machine has its own full-screen camera and controls its own player. The
movement prototype exchanges position and facing over direct IPv4 UDP.
Firing, damage, pickups, respawns and match results still need replication.
The separate GE32 release uses a different multiplayer implementation.

1. Connect both machines to the same LAN. On Windows, build this revision
   with `./build-pc.sh ntsc-final` and prepare the US ROM and desktop asset
   sidecars as described in `docs/building.md`, or use a prepared Windows
   bundle. Run `ge007.x86_64.exe --lan-host` in the game directory. The
   window waits for a client on UDP 27007. Connect the Windows controller
   before launch.
2. Run `ipconfig` and read the IPv4 address of the Ethernet adapter. If
   Windows Firewall prompts, allow the game on the Private network profile.
3. On Xbox, pick a dossier, then **LAN MULTIPLAYER**. Use D-pad left or right
   to select an octet and up or down to change it. Hold LT to step by ten.
   Press A to join, or B to go back. The host and client enter a two-player,
   bot-free Complex match after the handshake. Use controller port 1.
4. At spawn, stand still on Xbox while the Windows player moves across its
   view and turns. Swap roles. Repeat from a fresh launch after unplugging
   only the router's WAN cable. Record each build and both screens. The
   third dossier row, **LOCAL MULTIPLAYER**, starts split-screen play.

For a Windows distribution bundle, place `port/lan/Host-Complex.cmd` beside
the executable. It prints the host's network addresses and starts the match.
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
