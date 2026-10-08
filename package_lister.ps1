$ErrorActionPreference='Stop'
$root=$PSScriptRoot
$wlx=Join-Path $root 'build32\Release\XRayTHMLister.wlx'
$wlx64=Join-Path $root 'build64\Release\XRayTHMLister.wlx64'
$editor=Join-Path $root 'build64\Release\XRayTHMEditor.exe'
$editor32=Join-Path $root 'build32\Release\XRayTHMEditor.exe'
$cli=Join-Path $root 'build64\Release\thm_info.exe'
foreach ($file in @($wlx,$wlx64,$cli,$editor,$editor32)) {if (-not (Test-Path -LiteralPath $file)) {throw "Missing $file. Run build_lister.bat first."}}
$dist=Join-Path $root 'dist'
$stage=Join-Path $dist 'XRayTHMTools-v0.4.0-beta'
if (Test-Path -LiteralPath $stage) {Remove-Item -LiteralPath $stage -Recurse -Force}
New-Item -ItemType Directory -Force -Path $stage | Out-Null
foreach ($file in @($wlx,$wlx64,$cli,$editor)) {Copy-Item -LiteralPath $file -Destination $stage}
Copy-Item -LiteralPath $editor32 -Destination (Join-Path $stage 'XRayTHMEditor32.exe')
Copy-Item -LiteralPath (Join-Path $root 'lister\pluginst.inf') -Destination $stage
Copy-Item -LiteralPath (Join-Path $root 'LISTER_README_RU.md') -Destination (Join-Path $stage 'LISTER_README_RU.md')
foreach ($name in @('LICENSE','FORMAT_NOTES.md','EDITOR_README_RU.md','ACKNOWLEDGMENTS.md','CHANGELOG.md','VALIDATION.md','README_RU.md','README.md','RELEASE_NOTES_RU.md')) {Copy-Item -LiteralPath (Join-Path $root $name) -Destination $stage}
Get-ChildItem $stage -File | Sort-Object Name | ForEach-Object {"$((Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower())  $($_.Name)"} | Set-Content (Join-Path $stage 'SHA256SUMS.txt') -Encoding ASCII
$zip=Join-Path $dist 'XRayTHMTools-v0.4.0-beta.zip'
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
"$((Get-FileHash $zip -Algorithm SHA256).Hash.ToLower())  $([IO.Path]::GetFileName($zip))" | Set-Content "$zip.sha256" -Encoding ASCII
Write-Host $zip
