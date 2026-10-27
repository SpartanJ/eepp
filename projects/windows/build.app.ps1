param(
  [Parameter(Mandatory = $true, Position = 0)][string]$App,
  [ValidateSet("x64", "x86_64", "arm64")][string]$Arch = "x64",
  [ValidateSet("sdl2", "sdl3")][string]$Backend = "sdl2",
  [string]$Version,
  [switch]$SkipBuild,
  [string]$SourceDir,
  [string]$OutputDir
)
$ErrorActionPreference = "Stop"
$RepoRoot = (Resolve-Path "$PSScriptRoot/../..").Path
if ($App -cnotmatch '^[a-z][a-z0-9_-]*$') { throw "Invalid application: $App" }
$AppDir = Join-Path $RepoRoot "src/tools/$App"
if (-not (Test-Path "$AppDir/assets.list")) { throw "Missing application manifest: $App" }
$header = Get-Content "$AppDir/version.hpp" -Raw
$parts = foreach ($part in @("MAJOR_VERSION", "MINOR_VERSION", "PATCH_LEVEL")) {
  $pattern = '(?m)^#define\s+' + $App.ToUpper().Replace('-', '_') + '_' + $part + '\s+([0-9]+)\s*$'
  if ($header -notmatch $pattern) { throw "Missing integer version field: $part" }
  $Matches[1]
}
if (-not $Version) { $Version = $parts -join '.' }
if ($Version -cnotmatch '^[a-zA-Z0-9][a-zA-Z0-9._+-]*$') { throw "Invalid version label: $Version" }
if (-not $SourceDir) { $SourceDir = Join-Path $RepoRoot "bin" }
$SourceDir = (Resolve-Path $SourceDir).Path
if (-not $OutputDir) { $OutputDir = Join-Path $PSScriptRoot $App }
$OutputDir = [IO.Path]::GetFullPath($OutputDir)
$archSuffix = if ($Arch -eq "arm64") { "arm64" } else { "x86_64" }
$sdlDll = if ($Backend -eq "sdl3") { "SDL3.dll" } else { "SDL2.dll" }
Push-Location $RepoRoot
try {
  if (-not $SkipBuild) {
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
    $premakeExtra = if ($isArm64) { "--arch=aarch64" } else { "" }
    $msbuildPlat = if ($isArm64) { "ARM64" } else { "x64" }
    $backendArg = if ($isSdl3) { "SDL3" } else { "SDL2" }

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
    if ($LASTEXITCODE -ne 0) { throw "Premake failed: $LASTEXITCODE" }

    $msbuildInPath = Get-Command msbuild -ErrorAction SilentlyContinue

    if ($msbuildInPath) {
      $msbuildCmd = "msbuild"
    } elseif ($env:MSBUILD_PATH) {
      $msbuildCmd = "$env:MSBUILD_PATH/MSBuild.exe"
      if (-not (Test-Path $msbuildCmd)) {
        throw "MSBuild.exe not found at $msbuildCmd"
      }
    } else {
      throw "msbuild not found in PATH and MSBUILD_PATH is not defined"
    }

    $slnExt = if ($vsAction -eq "vs2026") { "slnx" } else { "sln" }
    & $msbuildCmd ".\make\windows\eepp.$slnExt" -m "/t:$App" /p:Platform=$msbuildPlat /p:Configuration=release
    if ($LASTEXITCODE -ne 0) { throw "MSBuild failed: $LASTEXITCODE" }
  }
  $Staging = Join-Path $OutputDir $App
  if (Test-Path $Staging) { Remove-Item -Recurse -Force $Staging }
  New-Item -ItemType Directory -Force -Path $Staging | Out-Null
  & "$PSScriptRoot/../scripts/copy_app_assets.ps1" -App $App -SourceDir $SourceDir -DestDir $Staging
  $eeppDll = Join-Path $SourceDir "eepp.dll"
  if (-not (Test-Path $eeppDll)) { $eeppDll = "libs/windows/$archSuffix/eepp.dll" }
  Copy-Item -LiteralPath "$SourceDir/$sdlDll", $eeppDll, "$SourceDir/$App.exe" -Destination $Staging
  $ArchiveBase = Join-Path $OutputDir "$App-windows-$Version-msvc-$archSuffix"
  Compress-Archive -LiteralPath $Staging -DestinationPath "$ArchiveBase.zip" -Force
  if (Test-Path "$SourceDir/$App.pdb") {
    Compress-Archive -LiteralPath "$SourceDir/$App.pdb" -DestinationPath "$ArchiveBase-pdb.zip" -Force
  }
} finally {
  Pop-Location
}
