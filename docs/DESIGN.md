# How the provider works

`RTSSDLSS.dll` exports RTSS's native client-plugin `Start`, `Stop`, and `Setup`
functions. Start creates one polling thread inside RTSS. Stop signals the
thread, joins it, and releases its sensor channel. No separate runtime helper
process is launched.

The DLL loads the installed 32-bit `nvapi.dll` from the Windows system
directory and calls NVIDIA's public `NvAPI_NGX_GetNGXOverrideState` for
candidate process IDs. It first checks the foreground process, then the last
detected game and RTSS's tracked applications. It polls once per second.
It does not open game memory or install a game hook.

The feedback flags are interpreted in `src/status.h`. Enabled and evaluated
flags are required for ON. Error flags take precedence. A preset is exposed
only for an active preset override; the returned integer maps to a preset
letter where defined. Future unknown values remain numeric.

The provider publishes fourteen string/numeric sensors through the
`AIDA64_SensorValues` shared-memory convention supported by OverlayEditor.
The sources in `DLSS variables.ovl` refer to those sensor IDs. This is an
adapter to an existing RTSS provider, not a new provider type added to RTSS.

The plugin creates that mapping only if the name is free. An existing AIDA64
publisher or an older DLSS bridge causes activation to fail without modifying
the other publisher's data. This is a known interoperability limit.

Runtime code has no networking, update mechanism, analytics, game injection,
driver-setting writes, or DLSS-indicator registry tweaks. The installation
scripts copy local files, edit RTSS's plugin settings, preserve backups, and
restart installation-owned RTSS processes. Read them before running them.

References:

- [NVIDIA's public NVAPI header](https://github.com/NVIDIA/nvapi/blob/main/nvapi.h)
  — `NvAPI_NGX_GetNGXOverrideState` and `NV_NGX_DLSS_OVERRIDE_BITFIELD`.
- Integration was checked against the RTSS SDK supplied with the local
  installation. RTSS SDK sources and NVIDIA driver binaries are not bundled.
