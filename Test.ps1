param([Parameter(Mandatory=$true)][string]$Zig)
$ErrorActionPreference='Stop'
$Zig=(Resolve-Path -LiteralPath $Zig).Path
$root=$PSScriptRoot
$build=Join-Path $root '.build'
New-Item -ItemType Directory -Path $build,(Join-Path $build 'test-run\work'),(Join-Path $build 'temp') -Force|Out-Null
$env:ZIG_GLOBAL_CACHE_DIR=Join-Path $build 'global-cache'
$env:ZIG_LOCAL_CACHE_DIR=Join-Path $build 'local-cache'
$env:TEMP=Join-Path $build 'temp';$env:TMP=$env:TEMP
& $Zig cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror (Join-Path $root 'src\test_status.c') -o (Join-Path $build 'test-status.exe')
if($LASTEXITCODE){throw 'Decoder test compilation failed.'}
& (Join-Path $build 'test-status.exe')
if($LASTEXITCODE){throw 'Decoder tests failed.'}
& $Zig cc -target x86-windows-gnu -O2 -Wall -Wextra -Werror (Join-Path $root 'src\test_host.c') -o (Join-Path $build 'test-host.exe') -luser32 -lkernel32
if($LASTEXITCODE){throw 'Lifecycle host compilation failed.'}
Push-Location (Join-Path $build 'test-run')
try {
    & (Join-Path $build 'test-host.exe') (Join-Path $root 'RTSSDLSS.dll') --collision
    if($LASTEXITCODE){throw 'Occupied-channel preservation test failed.'}
    & (Join-Path $build 'test-host.exe') (Join-Path $root 'RTSSDLSS.dll')
    if($LASTEXITCODE){throw 'Lifecycle tests failed.'}
} finally {Pop-Location}
