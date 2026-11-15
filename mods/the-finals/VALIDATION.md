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

The extension is pinned to GE32 Windows 2026.11.07 e8859fe9 and is not a portable multiplayer source implementation. Automated gameplay tests used an offline session; cross-PC multiplayer regression testing remains outstanding. No jumping, sprinting or sliding is implemented.
