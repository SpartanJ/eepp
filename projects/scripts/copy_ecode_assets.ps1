# Compatibility entry point for ecode builds.
param(
	[Parameter(Mandatory = $true)][string]$SourceDir,
	[Parameter(Mandatory = $true)][string]$DestDir
)
$ErrorActionPreference = "Stop"
Push-Location "$PSScriptRoot/../.."
try {
	& "$PSScriptRoot/copy_app_assets.ps1" -App ecode -SourceDir $SourceDir -DestDir $DestDir
} finally {
	Pop-Location
}
