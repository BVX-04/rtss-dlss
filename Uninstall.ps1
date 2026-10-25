param([string]$RtssDirectory='C:\Program Files (x86)\RivaTuner Statistics Server',[switch]$RestartRtss)
$ErrorActionPreference='Stop'
$rtssExe=Join-Path $RtssDirectory 'RTSS.exe'
if(!(Test-Path -LiteralPath $rtssExe)){throw 'RTSS installation not found.'}
$active=@(Get-Process -Name RTSS -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $rtssExe })
if($active.Count -and !$RestartRtss){throw 'Close RTSS first, or allow -RestartRtss.'}
Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
public static class DlssRemoveIni {
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode)]
 public static extern bool WritePrivateProfileString(string section,string key,string value,string path);
}
'@
$hookLoaders=@(Get-Process -Name RTSSHooksLoader,RTSSHooksLoader64 -ErrorAction SilentlyContinue | Where-Object { $_.Path -and (Split-Path $_.Path -Parent) -eq $RtssDirectory })
foreach($process in @($active)+@($hookLoaders)){Stop-Process -Id $process.Id; $process.WaitForExit(5000) | Out-Null}
try {
    if(![DlssRemoveIni]::WritePrivateProfileString('Plugins','RTSSDLSS.dll',$null,(Join-Path $RtssDirectory 'Profiles\Config'))){throw 'Could not remove the plugin setting.'}
    $dll=Join-Path $RtssDirectory 'Plugins\Client\RTSSDLSS.dll'
    if(Test-Path -LiteralPath $dll){Remove-Item -LiteralPath $dll}
    # Preserve user overlays and backups. Select the original layout in OverlayEditor.
    Write-Host 'RTSS DLSS removed. Your overlays and configuration backups were preserved.'
} finally {if($active.Count){Start-Process -FilePath $rtssExe -WorkingDirectory $RtssDirectory -WindowStyle Hidden}}
