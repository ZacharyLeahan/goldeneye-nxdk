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
| A while crouched | Stand up; consume that press until release |
| A while standing | Interact |
| X | Reload |
| Y | Next weapon |

The wheel does not pause gameplay. Fire and look input are suppressed while selecting. Engine inventory/equipment functions execute only on the local player's gameplay thread; drawing uses a lock-protected snapshot.

## Files

- weapon-wheel.c: game-thread hook, inventory snapshot/queue, controller handling, regression tests.
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
