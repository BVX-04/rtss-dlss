# Testing and current limitations

This is an experimental preview, not a claim that every DLSS game or GPU works.

## Local and user-reported checks

| Configuration | Result |
|---|---|
| RTSS 7.3.5, NVIDIA 610.88, Deadlock | Live SR override preset M; ON/OFF switching verified |
| Additional Plague Tale game, exact title pending | User reported preset K and ON/OFF switching working |
| DLL lifecycle host | Three Start/Stop cycles, duplicate calls, and shared-memory cleanup passed |
| Occupied sensor channel | Activation refused; existing publisher's sentinel data preserved |
| Feedback decoder | 13 status/preset checks passed, including enabled but unevaluated = OFF |
| Isolated installer/remove tests | Other plugin/config settings preserved |
| Installer fault injection | Prior layouts, configuration and DLL state restored after failed copies |

The full RTSS-hosted sensor feed and the expanded OSD text were checked
locally. The user also confirmed visible readings, preset colors, and the
RTSS tray icon after recovery. RR/FG variants, additional driver versions,
other GPUs and broader game compatibility still need independent testing.

## Known startup issue

Some scripted restarts of RTSS 7.3.5 crashed in RTSSHooks/ntdll before either
overlay plugin appeared in the crash dump's module list. A clean restart
that also stopped the remaining RTSS hook loaders subsequently passed
no-plugin, OverlayEditor-only, and both-plugin startup checks. The precise
cause is not established, so this remains a known issue rather than a claim
of a complete fix. Installer/remover restarts now also stop this installation's
hook loaders.

If the tray icon disappears, do not trust the old text left in a game as a
live reading. Disable the preview plugin if instability repeats and report it.

## Reporting a test

Include the game and exact version if known, GPU, NVIDIA driver, RTSS version,
DLSS mode, requested preset, expected result and observed result. Test
ON → OFF → ON and a preset change. If possible, compare with NVIDIA's own
indicator. Add a screenshot with personal details removed.

Do not post full crash dumps publicly without inspecting them: dumps may
contain private data. Start with the faulting application/module/error code.

## Automated tests

`Test.ps1` builds and runs the native decoder checks and disposable x86 DLL
host tests. The host tests can run without an NVIDIA GPU: they exercise
unavailable-driver behavior and lifecycle, not real DLSS operation.
Close RTSS or disable this plugin and any AIDA64 shared-memory publisher
before running lifecycle tests; they need to own the sensor channel.
