param(
  [string]$arch = "x64",
  [string]$backend = "sdl2"
)
Set-Location (Resolve-Path "$PSScriptRoot\..\..\..")

$premakeInPath = Get-Command premake5.exe -ErrorAction SilentlyContinue

if ($premakeInPath) {
  $premakeCmd = "premake5.exe"
} else {
  if (-not (Test-Path ".\premake5.exe")) {
    Invoke-WebRequest -Uri "https://github.com/premake/premake-core/releases/download/v5.0.0-beta8/premake-5.0.0-beta8-windows.zip" -OutFile "premake-5.0.0-beta8-windows.zip"
    Expand-Archive -LiteralPath "premake-5.0.0-beta8-windows.zip" -DestinationPath .
  }
  $premakeCmd = ".\premake5.exe"
}

$isArm64 = $arch -eq "arm64"
$isSdl3 = $backend -eq "sdl3"
$archSuffix = if ($isArm64) { "arm64" } else { "x86_64" }
$premakeExtra = if ($isArm64) { "--arch=aarch64" } else { "" }
$msbuildPlat = if ($isArm64) { "ARM64" } else { "x64" }
$backendArg = if ($isSdl3) { "SDL3" } else { "SDL2" }
$sdlDll = if ($isSdl3) { "SDL3.dll" } else { "SDL2.dll" }

$vsAction = "vs2026"
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsVersion = & $vsWhere -latest -property catalog_productLineVersion
    if ($vsVersion -lt 18.0) {
        $vsAction = "vs2022"
    }
}
Write-Output "Using premake action: $vsAction"

& $premakeCmd --windows-vc-build --with-backend=$backendArg $(if ($premakeExtra) { $premakeExtra }) --disable-static-build $vsAction

$msbuildInPath = Get-Command msbuild -ErrorAction SilentlyContinue

if ($msbuildInPath) {
  $msbuildCmd = "msbuild"
} elseif ($env:MSBUILD_PATH) {
  $msbuildCmd = "$env:MSBUILD_PATH/MSBuild.exe"
  if (-not (Test-Path $msbuildCmd)) {
    Write-Error "MSBuild.exe not found at $msbuildCmd"
    exit 1
  }
} else {
  Write-Error "msbuild not found in PATH and MSBUILD_PATH is not defined"
  exit 1
}

$slnExt = if ($vsAction -eq "vs2026") { "slnx" } else { "sln" }
& $msbuildCmd .\make\windows\eepp.$slnExt -m /t:ecode /p:Platform=$msbuildPlat /p:Configuration=release
.\projects\scripts\copy_ecode_assets.ps1 .\bin .\projects\windows\ecode\ecode
Copy-Item -Path ".\bin\$sdlDll", ".\libs\windows\$archSuffix\eepp.dll", ".\bin\ecode.exe" -Destination ".\projects\windows\ecode\ecode"
Compress-Archive -LiteralPath ".\projects\windows\ecode\ecode" -DestinationPath .\projects\windows\ecode\ecode-windows-nightly-msvc-$archSuffix.zip -Force
Compress-Archive -LiteralPath ".\bin\ecode.pdb" -DestinationPath .\projects\windows\ecode\ecode-windows-nightly-msvc-$archSuffix-pdb.zip -Force
