# RTSS DLSS

Live NVIDIA DLSS **override status and presets** in your own RivaTuner
Statistics Server overlay. Keep your RTSS layout and place the DLSS variables
beside your FPS, temperatures, or other information.

**Experimental preview.** Tested on a limited number of games and one system.
The DLL is unsigned. See [testing and known issues](docs/TESTING.md) before
installing or recommending it to others.

[Download the preview](https://github.com/bvx-04/rtss-dlss/releases) ·
[Installation guide](docs/INSTALLATION.md) ·
[How it works](docs/DESIGN.md)

## What it does

- Runs as an RTSS client plugin; starts and stops with RTSS.
- Exposes separate Super Resolution, Ray Reconstruction, and Frame Generation
  override status and preset variables in OverlayEditor.
- Reads live NVIDIA driver feedback instead of inferring the result from
  saved driver settings.
- Requires no NVIDIA overlay, recurring CMD launch, or background PowerShell
  helper. It does not inject code into the game, change DLSS settings, or make
  network requests.

An override being **OFF** does not mean native DLSS is necessarily disabled.
It means the queried driver override is not currently active. A preset is
shown only when the driver confirms an evaluated preset override. This does
not report a game's native preset or its DLSS DLL version.

## Requirements

- Windows 10/11 x64 with an installed NVIDIA driver exposing the NGX override
  API (R570 or later; only the listed local configuration has been tested).
- RTSS with OverlayEditor. The included DLL targets RTSS's **32-bit client
  plugin host**, even on 64-bit Windows and with 64-bit games.
- A free AIDA-compatible shared-memory channel. You do **not** need AIDA64
  installed. AIDA64 shared-memory publishing conflicts with this preview;
  the plugin refuses to overwrite an existing publisher.

## Quick installation

1. Download the release ZIP and extract it to a normal folder.
2. Save any open OverlayEditor changes. Run `Install.cmd` once and approve
   Windows' administrator prompt. RTSS is restarted; your game is not.
3. In RTSS → Setup → Plugins, confirm `RTSSDLSS.dll` is enabled.
4. In OverlayEditor, merge `DLSS variables.ovl` from RTSS's Overlays folder.
   Place the imported variables in your layout and save it.

No compiler is needed. The installer preserves the selected overlay by
default and backs up files it replaces. [Manual installation and removal](docs/INSTALLATION.md)
are also documented for people who prefer not to run the scripts.

## Overlay variables

| Variable | Value |
|---|---|
| `%DLSS SR override%` | `ON`, `OFF`, `ERROR`, or `UNAVAILABLE` |
| `%DLSS SR preset%` | Overridden preset, such as `K` or `M`; otherwise `--` |
| `%DLSS RR override%` / `%DLSS RR preset%` | Ray Reconstruction override |
| `%DLSS FG override%` / `%DLSS FG preset%` | Frame Generation override |
| `%DLSS live status%` | Combined SR status and active preset |
| `%DLSS target process%` | Game/process supplying the reading |
| `%DLSS SR state code%` | `-1` unavailable, `0` off, `2` on, `3` error |
| `%DLSS RR state code%` / `%DLSS FG state code%` | Numeric states for rules |
| `%DLSS FG target count%` | Driver-reported FG override target; `0` inactive |
| `%DLSS target PID%` / `%DLSS API status%` | Diagnostic values |

Keep string source format `%s` and numeric source format `%d`.

This layer hides the separator and preset when SR is not ON:

```text
DLSS SR: %DLSS SR override%<SWITCH "DLSS SR state code"><CASE 2> | Preset %DLSS SR preset%<CASE><SWITCH>
```

The source provider appears as `AIDA` because OverlayEditor has a fixed set
of provider types. It is the plugin supplying the data, not an AIDA64
installation. [Architecture and limitations](docs/DESIGN.md).

## Build and verification

All plugin and installer source is included. The public build workflow
downloads official Zig **0.15.2**, verifies its SHA256, builds the x86 DLL,
runs decoder and lifecycle tests, and produces the download package.
The GitHub-hosted runner does not validate DLSS on a real NVIDIA gaming GPU.

```powershell
.\Build.ps1 -Zig C:\path\to\zig.exe
.\Test.ps1 -Zig C:\path\to\zig.exe
.\Package.ps1
```

Release ZIPs contain `SHA256SUMS.txt` for integrity checks. A matching hash
checks file integrity; it is not a guarantee that software is safe.

## Testing and screenshots

Deadlock has been tested with preset M, including ON/OFF switching. A second
Plague Tale game was reported working with preset K; its exact title is
awaiting confirmation. These are individual reports, not broad compatibility
claims. [Test details and reports](docs/TESTING.md).

Screenshots will be added after they are supplied. Please avoid including
personal information in screenshots submitted for the README.

## License

**MIT License.** You may use, modify, share, and commercially reuse this
project under [LICENSE](LICENSE), while preserving the copyright and license
notice. Commercial reuse and resale are allowed by this license.

See [NOTICE](NOTICE) for attribution and third-party naming information.
