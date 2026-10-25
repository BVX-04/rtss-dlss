param([ValidateSet('Install','Uninstall')][string]$Operation='Install',[string]$OverlayPath='')
$ErrorActionPreference='Stop'
$script=Join-Path $PSScriptRoot ($Operation+'.ps1')
$identity=[Security.Principal.WindowsIdentity]::GetCurrent()
$principal=New-Object Security.Principal.WindowsPrincipal($identity)
if($principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    if($Operation -eq 'Install' -and $OverlayPath){& $script -OverlayPath $OverlayPath -RestartRtss}
    else {& $script -RestartRtss}
} else {
    $arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$script+'"'),'-RestartRtss')
    if($Operation -eq 'Install' -and $OverlayPath){$arguments+=@('-OverlayPath',('"'+$OverlayPath+'"'))}
    $installer=Start-Process -FilePath "$env:SystemRoot\System32\WindowsPowerShell\v1.0\powershell.exe" -Verb RunAs -ArgumentList $arguments -Wait -PassThru
    if($installer.ExitCode){throw "Installer exited with code $($installer.ExitCode)."}
}
