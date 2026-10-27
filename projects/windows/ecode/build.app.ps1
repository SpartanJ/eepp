# Compatibility entry point for ecode.
param(
  [string]$arch = "x64",
  [string]$backend = "sdl2",
  [string]$version = "nightly"
)
& "$PSScriptRoot/../build.app.ps1" -App ecode -Arch $arch -Backend $backend -Version $version
