param([Parameter(Mandatory=$true)][string]$Zig)
$ErrorActionPreference='Stop'
$Zig=(Resolve-Path -LiteralPath $Zig).Path
$packageRoot=$PSScriptRoot
$buildRoot=Join-Path $packageRoot '.build'
New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $buildRoot 'global-cache'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $buildRoot 'local-cache'
$env:TEMP=Join-Path $buildRoot 'temp'
$env:TMP=$env:TEMP
New-Item -ItemType Directory -Path $env:TEMP -Force | Out-Null
& $Zig cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror -shared (Join-Path $packageRoot 'src\plugin.c') -o (Join-Path $packageRoot 'RTSSDLSS.dll') -luser32 -lkernel32
if($LASTEXITCODE){throw 'Native plugin compilation failed.'}
Write-Host 'Built RTSSDLSS.dll (32-bit RTSS client plugin).'
