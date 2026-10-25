param(
    [string]$RtssDirectory='C:\Program Files (x86)\RivaTuner Statistics Server',
    [string]$OverlayPath='',
    [switch]$RestartRtss
)
$ErrorActionPreference='Stop'
$rtssExe=Join-Path $RtssDirectory 'RTSS.exe'
$clientDir=Join-Path $RtssDirectory 'Plugins\Client'
$config=Join-Path $RtssDirectory 'Profiles\Config'
$editorConfig=Join-Path $clientDir 'OverlayEditor.cfg'
$source=Join-Path $PSScriptRoot 'RTSSDLSS.dll'
if(!(Test-Path -LiteralPath $rtssExe) -or !(Test-Path -LiteralPath $clientDir)){throw 'Select an existing RTSS installation.'}
if(!(Test-Path -LiteralPath $source -PathType Leaf)){throw 'RTSSDLSS.dll is missing beside this installer.'}
if(!(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'DLSS variables.ovl') -PathType Leaf)){throw 'DLSS variables.ovl is missing beside this installer.'}
if($OverlayPath -and !(Test-Path -LiteralPath $OverlayPath -PathType Leaf)){throw 'The supplied overlay file does not exist.'}
# Check write access before stopping the running RTSS process.
$writeProbe=Join-Path $clientDir ('.RTSSDLSS-writecheck-'+[Guid]::NewGuid().ToString('N')+'.tmp')
try {
    [IO.File]::WriteAllBytes($writeProbe,[byte[]]@())
    if(Test-Path -LiteralPath $config) {
        $configProbe=[IO.File]::Open($config,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::ReadWrite)
        $configProbe.Dispose()
    }
} catch {throw 'Windows denied write access to RTSS. Run Install.cmd and approve its administrator prompt. RTSS has not been stopped.'}
finally {if(Test-Path -LiteralPath $writeProbe){Remove-Item -LiteralPath $writeProbe}}
$active=@(Get-Process -Name RTSS -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $rtssExe })
if($active.Count -and !$RestartRtss){throw 'Close RTSS first, or run with -RestartRtss to allow its restart. Your game can remain open.'}
Add-Type -TypeDefinition @'
using System.Runtime.InteropServices;
public static class DlssInstallerIni {
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]
 public static extern bool WritePrivateProfileString(string section,string key,string value,string path);
}
'@
$backup=Join-Path $PSScriptRoot ('Install-backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'-'+[Guid]::NewGuid().ToString('N').Substring(0,8))
New-Item -ItemType Directory -Path $backup -Force | Out-Null
if(Test-Path -LiteralPath $config){Copy-Item -LiteralPath $config -Destination (Join-Path $backup 'RTSS-Config.ini')}
if(Test-Path -LiteralPath $editorConfig){Copy-Item -LiteralPath $editorConfig -Destination (Join-Path $backup 'OverlayEditor.cfg')}
$destination=Join-Path $clientDir 'RTSSDLSS.dll'
if(Test-Path -LiteralPath $destination){Copy-Item -LiteralPath $destination -Destination (Join-Path $backup 'RTSSDLSS.dll')}
$overlayDir=Join-Path $clientDir 'Overlays'
$demoOverlay=Join-Path $overlayDir 'DLSS variables.ovl'
$installedOverlay=Join-Path $overlayDir 'Rivatuner overlay - DLSS Native.ovl'
$demoExisted=Test-Path -LiteralPath $demoOverlay
$customExisted=Test-Path -LiteralPath $installedOverlay
$demoWritten=$false; $customWritten=$false
if($demoExisted){Copy-Item -LiteralPath $demoOverlay -Destination (Join-Path $backup 'Previous-DLSS-variables.ovl')}
if($OverlayPath -and $customExisted){Copy-Item -LiteralPath $installedOverlay -Destination (Join-Path $backup 'Previous-DLSS-overlay.ovl')}
# Only restart the verified RTSS executable. This does not terminate the game.
$hookLoaders=@(Get-Process -Name RTSSHooksLoader,RTSSHooksLoader64 -ErrorAction SilentlyContinue | Where-Object { $_.Path -and (Split-Path $_.Path -Parent) -eq $RtssDirectory })
foreach($process in @($active)+@($hookLoaders)){Stop-Process -Id $process.Id; $process.WaitForExit(5000) | Out-Null}
try {
    Copy-Item -LiteralPath $source -Destination $destination -Force
    if(![DlssInstallerIni]::WritePrivateProfileString('Plugins','RTSSDLSS.dll','1',$config)){throw 'Could not enable the RTSS plugin.'}
    New-Item -ItemType Directory -Path $overlayDir -Force | Out-Null
    $demoWritten=$true
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'DLSS variables.ovl') -Destination $demoOverlay -Force
    if($OverlayPath) {
        $customWritten=$true
        Copy-Item -LiteralPath $OverlayPath -Destination $installedOverlay -Force
        if(![DlssInstallerIni]::WritePrivateProfileString('Settings','Layout',$installedOverlay,$editorConfig)){throw 'Could not select the DLSS overlay.'}
    }
    Write-Host ('Installed and enabled: '+$destination)
    Write-Host ('Configuration backup: '+$backup)
} catch {
    # Attempt every recovery step, even if a damaged/locked file cannot be restored.
    $installFailure=$_
    $rollbackFailures=New-Object 'System.Collections.Generic.List[string]'
    $tryRestore={param([scriptblock]$Action) try { & $Action } catch { $rollbackFailures.Add($_.Exception.Message) }}
    & $tryRestore {if(Test-Path -LiteralPath (Join-Path $backup 'RTSS-Config.ini')){Copy-Item -LiteralPath (Join-Path $backup 'RTSS-Config.ini') -Destination $config -Force}}
    & $tryRestore {if(Test-Path -LiteralPath (Join-Path $backup 'OverlayEditor.cfg')){Copy-Item -LiteralPath (Join-Path $backup 'OverlayEditor.cfg') -Destination $editorConfig -Force}}
    & $tryRestore {if($demoWritten) {
        if($demoExisted){Copy-Item -LiteralPath (Join-Path $backup 'Previous-DLSS-variables.ovl') -Destination $demoOverlay -Force}
        elseif(Test-Path -LiteralPath $demoOverlay){Remove-Item -LiteralPath $demoOverlay}
    }}
    & $tryRestore {if($customWritten) {
        if($customExisted){Copy-Item -LiteralPath (Join-Path $backup 'Previous-DLSS-overlay.ovl') -Destination $installedOverlay -Force}
        elseif(Test-Path -LiteralPath $installedOverlay){Remove-Item -LiteralPath $installedOverlay}
    }}
    & $tryRestore {if(Test-Path -LiteralPath (Join-Path $backup 'RTSSDLSS.dll')) {
        Copy-Item -LiteralPath (Join-Path $backup 'RTSSDLSS.dll') -Destination $destination -Force
    } elseif(Test-Path -LiteralPath $destination) {
        if((Get-FileHash -LiteralPath $destination).Hash -eq (Get-FileHash -LiteralPath $source).Hash){Remove-Item -LiteralPath $destination}
    }}
    if($rollbackFailures.Count){Write-Warning ('Some files could not be restored. Backups: '+$backup+'; '+($rollbackFailures -join '; '))}
    throw $installFailure
} finally {
    if($active.Count){Start-Process -FilePath $rtssExe -WorkingDirectory $RtssDirectory -WindowStyle Hidden}
}
