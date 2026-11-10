# CrowdTune 1.0 — a behaved, seated regular-season crowd for NBA 2K14 (PC)

Stock 2K14 fans behave like a soccer end: half the arena stands, claps or waves props during normal possessions. CrowdTune keeps them seated and quiet during normal play, gives routine baskets mild seated applause, and saves the big reactions for the moments that earn them. It follows the score, quarter, game clock and shot clock. Works with the stock `crowd.iff` (a texture-only crowd mod is fine too). It is a [SLAM](https://github.com/Dokitooo/SLAM) mod.

**Download:** [CrowdTune_v1.0.zip](https://github.com/Dokitooo/CrowdTune/releases/download/v1.0/CrowdTune_v1.0.zip) (latest release) ·
[all releases](https://github.com/Dokitooo/CrowdTune/releases)

## Requirements

- NBA 2K14 PC
- [SLAM](https://github.com/Dokitooo/SLAM/releases/download/v1.0.0/SLAM_v1.0.0.zip) 1.0.0 or newer

## Install

1. Install SLAM first (see its README).
2. Extract the zip into the game folder (the one with `nba2k14.exe`). It contains only:
   ```
   SLAM\CrowdTune\CrowdTune.asi
   SLAM\CrowdTune\CrowdTune.ini
   ```
3. Start the game. The mod list at the top left shows `CrowdTune 1.0`.

Everything is set in `CrowdTune.ini`; each setting is explained there, and [`package/README.txt`](package/README.txt) describes every reaction. Edit, then restart the game. Updating later: keep a copy of your `CrowdTune.ini` first.

The log is `SLAM\CrowdTune\CrowdTune.log`. To remove the mod, delete `SLAM\CrowdTune`.

## Settings (`CrowdTune.ini`)

| Setting | What it does |
| --- | --- |
| `Attendance / PlayoffAttendance` | Share of the game's own crowd that shows up: 0.85 in the regular season, 1.0 in the playoffs. |
| `StartSeated` | Everybody is seated when the arena loads. |
| `CalmHoldSeconds` | A seated fan stays seated for at least this long (12). |
| `ScoreboardReactions` | Reactions from the scoreboard: home dunks, buzzer beaters, close finishes, final horn. |
| `CloseGameMargin / CloseGameSeconds / CloseGameStand` | Close finish: within 5 points in the last 2:00 of the 4th / OT, up to 80% stand. |
| `PropsBig` | Towels, thunder sticks, foam fingers, signs and chants: playoff games only. |
| `LeaveWhenTrailingBy` | Fans leave only while the opponent leads by 15+. |

## Repository layout

| Path | Contents |
| --- | --- |
| `src/` | The mod (`crowdtune.c`, generated `graph_gate.inc`) |
| `slam/` | SLAM SDK header and glue |
| `build/` | Build scripts (MSVC, clang-cl on Linux) |
| `test/` | `policysim.c` and `stubtest.c` simulations |
| `crowd_iff/` | Python scripts that read the stock `crowd.iff` clip graph and write `src/graph_gate.inc` (need an `iff14.py` IFF reader, not included) |
| `package/` | The README and default `.ini` |

## Build

From an *x86 Native Tools Command Prompt for VS*: `build\build_msvc.bat`, or `build/build_linux_clang.sh`. Output: `bin\CrowdTune.asi`.
On Linux with clang-cl + lld-link (LLVM 18). `slam/slam_glue.c` needs Windows headers: set `MGW` to the
MinGW-w64 `mingw-w64-headers` folder and `GEN` to a folder with a generated `_mingw.h` (see SLAM's `source/build`).

## Verify your download

Every release lists the SHA-256 of the zip in `SHA256SUMS.txt` (also attached to the release).
Compare it with the hash of the file you downloaded:

```powershell
Get-FileHash .\CrowdTune_v1.0.zip -Algorithm SHA256
```

```bash
sha256sum -c SHA256SUMS.txt
```

## About antivirus warnings

The ASI file in this mod hooks into `nba2k14.exe` at runtime, which some antivirus heuristics
flag on unknown files. To make the release easy to check:

- The full source is in this repository; the release binary is built from it.
- The binary is not packed, compressed or obfuscated, and the zip has no password.
- SHA-256 checksums are published with each release.

If your antivirus still flags a file, you can submit it as a false positive to the vendor, or
build it yourself from the source.
