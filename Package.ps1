$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$version=(Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
if($version -notmatch '^\d+\.\d+\.\d+(?:-[A-Za-z0-9.]+)?$'){throw 'Invalid package version.'}
$dist=Join-Path $root 'dist'
$stage=Join-Path $root ('.build\package-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $dist,$stage -Force|Out-Null
$names=@('RTSSDLSS.dll','README.md','LICENSE','NOTICE','VERSION','Build.ps1','Test.ps1','Package.ps1','DLSS variables.ovl','docs','src')
foreach($name in $names){Copy-Item -LiteralPath (Join-Path $root $name) -Destination (Join-Path $stage $name) -Recurse -Force}
$hashes=Get-ChildItem -LiteralPath $stage -Recurse -File|Sort-Object FullName|ForEach-Object {
    (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()+'  '+$_.FullName.Substring($stage.Length+1).Replace('\','/')
}
[IO.File]::WriteAllLines((Join-Path $stage 'SHA256SUMS.txt'),$hashes)
$archive=Join-Path $dist ('RTSS-DLSS-'+$version+'.zip')
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $archive -Force
$hash=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText(($archive+'.sha256'),$hash+'  '+(Split-Path $archive -Leaf)+[Environment]::NewLine)
Copy-Item -LiteralPath (Join-Path $root 'RTSSDLSS.dll') -Destination (Join-Path $dist 'RTSSDLSS.dll') -Force
$dllHash=(Get-FileHash -LiteralPath (Join-Path $dist 'RTSSDLSS.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $dist 'RTSSDLSS.dll.sha256'),$dllHash+'  RTSSDLSS.dll'+[Environment]::NewLine)
Write-Host ('Packaged: '+$archive)
