# Installation

Download the **release ZIP** from the repository's Releases page. GitHub's
automatic "Source code" archives do not contain a prebuilt DLL.

## Installer

1. Install RTSS and confirm its normal overlay works.
2. Save any edits currently open in OverlayEditor.
3. Extract the preview ZIP to a normal folder, keeping its files together.
4. Run `Install.cmd`. Approve Windows' administrator prompt so it can copy
   the DLL into RTSS's Program Files directory. This prompt does not certify
   the software; inspect the source or use manual installation if preferred.
5. The script enables `RTSSDLSS.dll`, copies the reusable overlay template,
   and restarts an already running RTSS. Start RTSS yourself if it was closed.
6. Check RTSS → Setup → Plugins → `RTSSDLSS.dll`. Its Setup button shows
   the current target process, feedback and errors.
7. Open OverlayEditor and merge `DLSS variables.ovl` from the RTSS Overlays
   folder. You can delete or move the demo layer after importing the sources.
8. Add `%DLSS SR override%` and `%DLSS SR preset%` to a text layer and save.

Your current overlay is not replaced by the default installer. Timestamped
backups are saved under `Install-backups` beside the extracted installer.
No Windows startup entry is added: the plugin loads when RTSS loads it.

### Custom RTSS directory

Open an administrator PowerShell in the extracted folder and run:

```powershell
.\Install.ps1 -RtssDirectory "D:\Apps\RivaTuner Statistics Server" -RestartRtss
```

## Manual installation

1. Close RTSS and its hook-loader processes, after saving OverlayEditor edits.
2. Copy `RTSSDLSS.dll` into your RTSS installation's `Plugins\Client` folder.
   The default is `C:\Program Files (x86)\RivaTuner Statistics Server`.
3. Copy `DLSS variables.ovl` into `Plugins\Client\Overlays`.
4. Start RTSS, open Setup → Plugins, and enable `RTSSDLSS.dll`.
5. Merge the template into your layout using OverlayEditor as described above.

You do not need to execute our installer scripts for manual installation.
Windows may still request permission to copy files into Program Files.

## Expected behavior

- `ON` requires enabled **and evaluated** NVIDIA override feedback.
- `OFF` means the override is not active. This includes an enabled driver
  override while the game is not evaluating DLSS.
- Preset `--` means no evaluated preset override is available from this API.
- `UNAVAILABLE` means there is no current usable reading. Switch to the game
  once after restarting RTSS with a game already open.
- With several games open, the foreground game takes priority. Check
  `%DLSS target process%` if a reading appears to belong to another game.

## Troubleshooting

**AIDA channel occupied:** Stop the old PowerShell DLSS prototype, or turn
off AIDA64 shared-memory publishing. The DLL refuses to replace existing
data. Then disable and re-enable the plugin.

**RTSS tray icon disappears:** Check whether RTSS.exe is still running.
The game may retain stale overlay text after RTSS exits. Close the remaining
RTSS hook loaders and restart RTSS. If this repeats, disable `RTSSDLSS.dll`
and report it with the RTSS version and crash details. Startup behavior is
still under testing; do not treat frozen text as a live reading.

**No variables:** Merge the supplied template. The data sources are under
provider `AIDA`; installing AIDA64 is not necessary.

**Preset still appears while OFF:** Use the conditional example in the README.
Hiding the preset is a layout choice; the raw preset source returns `--`.

## Disable and remove

Uncheck `RTSSDLSS.dll` in RTSS's Plugins tab to stop its polling.

For removal, run `Uninstall.cmd` and approve the administrator prompt. Or
close RTSS and manually delete `Plugins\Client\RTSSDLSS.dll`, then restart.
The uninstaller preserves your overlays and backups. Remove the DLSS layer
or select your earlier overlay if you no longer want those variables shown.
