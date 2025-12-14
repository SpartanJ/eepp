param(
  [string]$arch = "x64"
)
Set-Location (Resolve-Path "$PSScriptRoot\..\..\..")

$premakeInPath = Get-Command premake5.exe -ErrorAction SilentlyContinue

if ($premakeInPath) {
  $premakeCmd = "premake5.exe"
} else {
  if (-not (Test-Path ".\premake5.exe")) {
    Invoke-WebRequest -Uri "https://github.com/premake/premake-core/releases/download/v5.0.0-beta7/premake-5.0.0-beta7-windows.zip" -OutFile "premake-5.0.0-beta7-windows.zip"
    Expand-Archive -LiteralPath "premake-5.0.0-beta7-windows.zip" -DestinationPath .
  }
  $premakeCmd = ".\premake5.exe"
}

$isArm64 = $arch -eq "arm64"
$archSuffix = if ($isArm64) { "arm64" } else { "x86_64" }
$premakeExtra = if ($isArm64) { "--arch=arm64" } else { "" }
$msbuildPlat = if ($isArm64) { "ARM64" } else { "x64" }

& $premakeCmd --windows-vc-build --with-text-shaper $(if ($premakeExtra) { $premakeExtra }) --disable-static-build vs2022

& "$env:MSBUILD_PATH/MSBuild.exe" .\make\windows\eepp.sln -m /t:ecode /p:Platform=$msbuildPlat /p:Configuration=release
.\projects\scripts\copy_ecode_assets.ps1 .\bin .\projects\windows\ecode\ecode
Copy-Item -Path ".\bin\SDL2.dll", ".\libs\windows\$archSuffix\eepp.dll", ".\bin\ecode.exe" -Destination ".\projects\windows\ecode\ecode"
Compress-Archive -LiteralPath ".\projects\windows\ecode\ecode" -DestinationPath .\projects\windows\ecode\ecode-windows-nightly-msvc-$archSuffix.zip -Force
Compress-Archive -LiteralPath ".\bin\ecode.pdb" -DestinationPath .\projects\windows\ecode\ecode-windows-nightly-msvc-$archSuffix-pdb.zip -Force