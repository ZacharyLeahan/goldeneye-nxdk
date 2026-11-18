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
make -C "$NXDK_DIR" NXDK_ONLY=y NXDK_SDL=y all cxbe extract-xiso
python3 port/xbox/build.py --rom "GoldenEye 007.zip"
```

The packager checks the dump against `ge007.u.sha1`. Open
`build-xbox/ge007.iso` as the DVD in a 64 MiB xemu profile and reset.
For an Xbox dashboard installation, copy `default.xbe`, `xbox.bin` and
`ge007.ntsc-final.z64` together from `build-xbox/disc/` into the game folder.
These private outputs contain cartridge data. Running the packager without
`--rom` builds the executable alone.

### Play Complex locally

1. Bind controllers to Xbox ports 1 and 2 before entering multiplayer setup.
2. At file select, point the left stick at a dossier and press A. The mode
   page lists **COOP CAMPAIGN**, **ONLINE MULTIPLAYER** and
   **LOCAL MULTIPLAYER** in that order. Select the third row with A.
3. In Multiplayer Options, choose **Level**, then **Complex** (top row,
   third tile). Check that the options show **Players 2** and **Level Complex**.
4. Press Start to begin. The default GoldenEye control style uses the left
   stick to move and turn, the right stick for the N64 C buttons, RT for Z
   (fire), LT for R (aim), A to select or cycle weapons and B to go back or
   interact.

Complex is the supported arena in this build. Selecting co-op campaign or
online multiplayer displays an unavailable message. The Windows GE32
extension has a separate binary interface.

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
