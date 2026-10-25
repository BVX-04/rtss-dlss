# Installation

1. Download **RTSSDLSS.dll** from the repository's Releases page.
2. Close RTSS.
3. Copy the DLL into RTSS's `Plugins\Client` folder. The default location is:

   `C:\Program Files (x86)\RivaTuner Statistics Server\Plugins\Client`

4. Open RTSS, go to **Setup > Plugins**, and enable **RTSSDLSS.dll**.

There is no installer to run. No CMD or PowerShell commands are needed.
Windows may ask for administrator permission when copying into Program Files.

The release ZIP includes `DLSS variables.ovl` for OverlayEditor data sources.
The maintainer will provide screenshots for the remaining layout setup.

To disable the provider, uncheck it in RTSS's Plugins tab. To remove it, close
RTSS and delete `RTSSDLSS.dll` from the plugin folder.
