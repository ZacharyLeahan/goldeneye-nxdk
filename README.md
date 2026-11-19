# GoldenEye 007 multiplayer for Windows and original Xbox

Play split-screen on one machine, or give each player a full screen on the same LAN. This fork brings the native GoldenEye PC port to the original Xbox and shares its multiplayer menus and modern movement controls with Windows. The goal is smooth couch multiplayer on original hardware and direct LAN play between Xbox and PC. [Upstream GoldenEye PC port](https://github.com/jkdansereau/goldeneye-pc-port)

| Mode | How to play |
| --- | --- |
| **Local Multiplayer** | Connect two to four controllers, select **Local Multiplayer**, then choose the arena and rules in Multiplayer Options. Two-player firefights have been tested on original Xbox hardware. Four-player split-screen has been verified in xemu on Complex; a four-pad hardware pass is still owed. |
| **LAN Multiplayer** | One Windows PC hosts a two-player match. An original Xbox or another Windows PC joins by entering the host’s LAN IPv4 address. Each player has a full-screen view. The host tracks hits, kills, pickups, scores and rematches. |

On Xbox, select a dossier to see both modes. On Windows, open **Multiplayer** in the dossier. For LAN, choose **Host Match** on Windows, then **Join Match** on the other PC or **LAN Multiplayer** on Xbox. Enter the address shown by the host. The default is Complex on UDP 27007. The host and guest should run builds from the same revision.

## Modern movement and equipment

| Action | Controller input |
| --- | --- |
| Jump and mantle | Press **A** to jump. Move toward a ledge and press **A** near the lip to climb it. |
| Crouch and slide | Tap **B** to crouch. Tap **B** while sprinting at speed to slide. Press **A** to jump out of the slide. |
| Sprint | Click the **left stick** while moving forward. |
| Aim and fire | Look with the **right stick**, hold **LT** for fine aim and pull **RT** to fire. **X** reloads and **Y** interacts. |
| Equipment wheel | Hold **LB** on Windows or **White** on Xbox. Point the right stick and release to equip. Right selects grenades, down selects the jump pad, and the other slots hold owned guns. Each player controls their own wheel. |
| Deploy a jump pad | Equip the pad, aim at a valid floor surface and pull **RT**. Either player can use the placed pad. |

**Melee.** The separate GE32 Windows extension adds a right-stick-click **quick slap** that strikes with Bond’s palm and then restores the equipped weapon. That shortcut is specific to the GE32 extension. [Extension controls](mods/the-finals/README.md) · [Native controls and equipment](DOWNSTREAM.md#play-complex-locally)

## Build and play

Supply your own retail US (NTSC-U) ROM. On Windows, prepare the model and level sidecars and run `./build-pc.sh ntsc-final` from MSYS2 MINGW64. Launch `./build-pc/ge007.x86_64.exe` from the repository root. To host Egypt directly, append `--lan-host --lan-map Egypt`. A Windows guest can also use `--lan-join HOST_IPV4`. For Xbox, the nxdk build produces the executable, converted resources and ROM that go together on the console. [Windows asset and build instructions](docs/building.md) · [Xbox build and installation](DOWNSTREAM.md#build-and-boot)

## Performance on Windows and original Xbox

| Platform and workload | Measured frame rate |
| --- | --- |
| Windows, fixed Egypt camera, two game processes on one PC | **60.00 FPS host / 59.99 FPS guest**. Each rate uses the time between 1,437 consecutive render completions over about 24 seconds. |
| Original Xbox, same Egypt LAN firefight, 480p | **50.00 → 59.13 displayed FPS** with the scheduler fix. Observed repeated refreshes fell from **113 to 8**. The 95th-percentile display interval fell from 33.33 to 16.67 ms. |
| Original Xbox, heavy two-player Egypt split-screen | **46.29 displayed FPS** across 925 comparable camera steps with the regular vertex writer. Both players turned, fired and threw grenades. |

Windows timing comes from the game’s render thread. Xbox timing comes from physical display refreshes. The LAN guest draws one full-screen view. Local split-screen draws both players’ views. [Windows replay and measurement instructions](DOWNSTREAM.md) · [Xbox LAN measurements](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/21#issuecomment-7015760434) · [Xbox split-screen measurements](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/21#issuecomment-7016289480)

The largest LAN gain came from starting graphics work sooner. When an audio task and a graphics task arrived together, the earlier scheduler left both queued until the next refresh. Accepting the pair immediately and preserving audio priority cut average submission-to-handoff time from **4.05 to 0.66 ms**. The renderer also skips drawing the hidden LAN seat while continuing its gameplay updates. Reusing material setup across vertex loads saved another **0.14–0.18 ms per frame** in two separate hardware trials. [Scheduler trace](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/21#issuecomment-7015760434) · [Material reuse trials](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/21#issuecomment-7014737918)

Caching the equipment-wheel artwork helped local multiplayer most when both players opened it. The fixed Complex scene went from **7.5 to 60.0 FPS**. A separate experimental LAN renderer reached **59.92 displayed FPS** in Egypt, with zero observed repeats and one gap in refresh sampling. The playable build uses the scheduler fix. [Equipment-wheel measurements](DOWNSTREAM.md#original-xbox) · [Experimental renderer A/B](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/21#issuecomment-7016408401)

A physical Xbox joined a Windows host and fired on all eleven retail multiplayer arenas. Current goals include frame pacing, local match setup and finding hosts on the LAN. Xbox hosting is deferred. [LAN testing](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/19) · [Local multiplayer goals](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/22) · [Host discovery](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/23) · [Future Xbox hosting](https://github.com/ZacharyLeahan/goldeneye-nxdk/issues/24)

## Upstream and credits

- The reconstructed game comes from the GoldenEye 007 decompilation by Larry Ficken (kholdfuzion) and contributors. The native PC port supplies the desktop runtime, controls, menus and split-screen support that this fork builds on. [GoldenEye decompilation](https://github.com/n64decomp/007) · [GoldenEye PC port](https://github.com/jkdansereau/goldeneye-pc-port)
- The XboxDev community’s nxdk supplies the original Xbox toolchain, runtime and pbkit graphics library. The Xbox NV2A renderer and presentation fixes were adapted from Perfect Dark X. The Perfect Dark PC port by Ryan Dwyer and contributors provided the software Fast3D renderer lineage and a reference for porting Rare’s engine. [nxdk](https://github.com/XboxDev/nxdk) · [Perfect Dark X](https://github.com/GTTeancum/Perfect-Dark-X) · [Perfect Dark PC port](https://github.com/perfect-dark-pc-port/perfect_dark) · [Third-party notices](NOTICE)
- The Neverball nxdk port provided a comparison for Xbox push-buffer sizing and VBlank presentation. [Neverball for Xbox](https://github.com/ZacharyLeahan/neverball-nxdk) · [nxdk-gles11](https://github.com/Ryzee119/nxdk-gles11)
- We tested the Xbox build in [xemu](https://github.com/xemu-project/xemu) with the 1 MiB [OXOS BIOS](https://github.com/kooscode/oxos), version [v0.4.14](https://github.com/kooscode/oxos/releases/tag/v0.4.14). This setup checks boot and frame identity. We measure display timing on the physical Xbox.
- Popnoodler’s [GE32 Windows release](https://popnoodler.itch.io/ge32) was the original target for this fork’s movement and equipment extension. [The Finals-inspired controls](mods/the-finals/README.md)

For the base port’s campaign, PC settings and release history, see the [upstream README](https://github.com/jkdansereau/goldeneye-pc-port#readme).

Shared multiplayer code lives in `port/lan/`, Xbox-specific integration in `port/xbox/`, and the movement source in `mods/the-finals/`. Both builds attach the multiplayer menu to a generated copy of the upstream front end. [Bringing upstream changes into the fork](DOWNSTREAM.md#bring-windows-upstream-changes-into-the-fork)

## Legal

This unofficial, noncommercial fan project requires a legally obtained retail US GoldenEye 007 ROM. Players generate the game assets locally. GoldenEye 007 and James Bond belong to their respective rights holders. Nintendo, Rare, Microsoft, Valve, MGM, Danjaq, EON Productions, Embark Studios and Nexon have no affiliation with this project. The MIT license covers original port work. Inherited game sources and third-party components have separate terms. [License](LICENSE) · [Third-party notices](NOTICE)
