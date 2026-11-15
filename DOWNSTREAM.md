# GoldenEye PC Port — The Finals-inspired controls

This downstream fork keeps the upstream PC-port source and adds the local GE32 extension in [mods/the-finals](mods/the-finals/README.md).

The playable multiplayer build used here is GE32 2026.11.07 e8859fe9 for Windows, a separate unofficial fan fork of the base PC port. Its multiplayer source is not included in this checkout. The additions are a native Windows extension and reversible executable patches for that exact GE32 release; they are not integrated into the upstream port build.

## Current additions

- Hold LB for an owned-weapon radial wheel; select with the right stick and release to equip. B cancels.
- Weapon silhouettes, translucent panels, readable labels, an equipped marker, and a selected-slot outline.
- Thread-safe local-player inventory snapshots and queued game-thread equipment changes.
- A jumps using native gravity and floor landing, with grounded-only takeoff and ceiling checks. B toggles crouch; A also clears crouch before jumping. Y interacts.
- Slower, smoothed controller aim: horizontal 65%, vertical 60%, smoothing 3/10.
- B backs out of the standard character-selection screen. Online character selection is unchanged.

- Left-stick-click sprint with a smooth 35% speed boost and contextual cancellation.
- A larger deployable jump pad with a native-era military-equipment look, matched placement/contact footprint, and immediate launch on contact.
- GoldenEye-inspired pixel lettering, green health cells and blue armor indicators, with no copied game assets.
- Confirmed-hit health bars above targets, with independent timers and a 3.25-second hold plus 0.20-second fade measured from supplied gameplay footage.
- Right-stick-click quick slap using the native palm animation/strike dispatch, followed by ammunition-safe gun restoration.

Sliding is not implemented. Movement and combat additions have local offline coverage; cross-PC synchronization is not yet verified.

See the [extension setup and validation](mods/the-finals/README.md). The original [upstream README](README.md) remains the base-port documentation.

No ROMs, extracted game assets, save files, GE32 binaries, patched executables, DLLs, or compiler distributions are included. Bring your own supported US ROM and obtain GE32 separately from its author. This fork is unofficial and not affiliated with Embark Studios or THE FINALS.
