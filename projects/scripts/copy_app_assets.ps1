# Copy the same manifests used by the Unix application packagers.
param(
	[Parameter(Mandatory = $true)][string]$App,
	[Parameter(Mandatory = $true)][string]$SourceDir,
	[Parameter(Mandatory = $true)][string]$DestDir,
	[string]$Platform = "windows"
)
$ErrorActionPreference = "Stop"
if ($App -cnotmatch '^[a-z][a-z0-9_-]*$') { throw "Invalid application: $App" }
$SourceDir = (Resolve-Path $SourceDir).Path
$AppDir = Join-Path "$PSScriptRoot/../../src/tools" $App

function Copy-Manifest([string]$Manifest) {
	foreach ($line in Get-Content -LiteralPath $Manifest) {
		if ($line -eq '' -or $line.StartsWith('#')) { continue }
		if ($line.StartsWith('@include ')) {
			Copy-Manifest (Join-Path (Split-Path $Manifest) $line.Substring(9))
			continue
		}
		$paths = $line.Split('|')
		$source = $paths[0]
		$target = $source
		if ($paths.Length -eq 2) { $target = $paths[1] }
		foreach ($path in @($source, $target)) {
			if ([IO.Path]::IsPathRooted($path) -or $path.Split('/') -contains '..') {
				throw "Invalid asset path: $line"
			}
		}
		$destination = Join-Path $DestDir $target
		New-Item -ItemType Directory -Force -Path (Split-Path $destination) | Out-Null
		$origin = Get-Item -LiteralPath (Join-Path $SourceDir $source)
		if ($origin.PSIsContainer) {
			New-Item -ItemType Directory -Force -Path $destination | Out-Null
			Get-ChildItem -LiteralPath $origin.FullName -Force | Copy-Item -Destination $destination -Recurse -Force
		} else {
			Copy-Item -LiteralPath $origin.FullName -Destination $destination -Force
		}
	}
}

Copy-Manifest (Join-Path $AppDir 'assets.list')
$Overlay = Join-Path $AppDir "assets.$Platform.list"
if (Test-Path -LiteralPath $Overlay) { Copy-Manifest $Overlay }
