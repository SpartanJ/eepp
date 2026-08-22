# Packages the eepp SDK for Windows (MSVC and MinGW).
#
# Usage: package_sdk.ps1 [-Platform <msvc|mingw>] [-Version <version>]
#
# -Version accepts "nightly" (default) or a stable release tag of the form
# eepp-%d.%d.%d (the tag prefix is stripped from the package names).
#
# Expects the release build to be present in bin\ and libs\windows\x86_64
# (see the CI workflows). Produces projects\windows\sdk\eepp-sdk-windows-x86_64-<platform>-<version>.zip.
#
# Package layout mirrors the repository root:
#   bin\assets + binaries, bin\ runtime DLLs, libs\windows\x86_64\,
#   include\ and docs\articles\.

param(
	[string]$Platform = "msvc",
	[string]$Version = "nightly"
)

$ErrorActionPreference = "Stop"

$RepoRoot = (Resolve-Path "$PSScriptRoot\..\..").Path
Push-Location $RepoRoot

try {
	$Arch = "x86_64"
	$SdkDirName = "eepp-sdk"

	# Stable release tags use the eepp-%d.%d.%d format; strip the tag prefix
	# for the package names. Anything else must be "nightly".
	if ($Version -eq "nightly") {
		$ArchiveSuffix = "nightly"
	} elseif ($Version -match "^eepp-\d+\.\d+\.\d+$") {
		$ArchiveSuffix = $Version.Substring("eepp-".Length)
	} else {
		throw "Invalid version '$Version' (expected nightly or eepp-%d.%d.%d)"
	}
	$Staging = "projects\windows\sdk\$SdkDirName"

	if (Test-Path "projects\windows\sdk") {
		Remove-Item -Recurse -Force "projects\windows\sdk"
	}
	New-Item -ItemType Directory -Force -Path "$Staging\bin", "$Staging\libs\windows\$Arch", "$Staging\docs" | Out-Null

	# -----------------------------------------------------------------------
	# Assets: everything except the unit-test / benchmark-only directories.
	# -----------------------------------------------------------------------

	$TestOnlyAssetDirs = @("html", "logo", "textfiles", "textformat")
	$TestOnlyAssetFiles = @("test.zip")

	New-Item -ItemType Directory -Force -Path "$Staging\bin\assets" | Out-Null

	Get-ChildItem "bin\assets" | ForEach-Object {
		if ($TestOnlyAssetDirs -notcontains $_.Name -and $TestOnlyAssetFiles -notcontains $_.Name) {
			Copy-Item -Recurse -Path $_.FullName -Destination "$Staging\bin\assets\$($_.Name)"
		}
	}

	# -----------------------------------------------------------------------
	# Binaries: every release executable in bin\ is packaged (any new
	# "eepp-*" binary is picked up automatically) plus the fixed tool set:
	# ecode, eterm and eeiv. Unit tests live in bin\unit_tests and benchmarks
	# in bin\benchmarks; they are never matched by this filter.
	# -----------------------------------------------------------------------

	function Add-Binary {
		param([string]$Name)

		$Source = "bin\$Name"
		if (-not (Test-Path $Source)) {
			Write-Warning "Binary not found, skipping: $Source"
			return
		}

		Copy-Item -Path $Source -Destination "$Staging\bin\$Name"
	}

	$Binaries = @(Get-ChildItem "bin" -File | Where-Object { $_.Name -like "eepp-*" } | Select-Object -ExpandProperty Name)
	foreach ($Name in $Binaries) {
		Add-Binary -Name $Name
	}
	Add-Binary -Name "ecode.exe"
	Add-Binary -Name "eterm.exe"
	Add-Binary -Name "eeiv.exe"

	# -----------------------------------------------------------------------
	# Libraries: shared libraries required at runtime plus the import/static
	# libraries useful for linking. Release configurations only. MSVC and
	# MinGW produce different import library flavors; missing files (for
	# example a MinGW import library on an MSVC build) are skipped with a
	# warning.
	# -----------------------------------------------------------------------

	$SharedLibs = @("eepp.dll", "eepp-maps.dll", "eepp-physics.dll")

	if ($Platform -eq "mingw") {
		# MinGW import libraries (libeepp*.a, generated along with the DLLs)
		# plus the statically linked modules and tools.
		$LinkLibs = @(
			"libeepp.a",
			"libeepp-maps.a",
			"libeepp-physics.a",
			"libeepp-maps-static.a",
			"libeepp-physics-static.a",
			"libeterm.a",
			"liblanguages-syntax-highlighting.a"
		)
	} else {
		$LinkLibs = @(
			"eepp.lib",
			"eepp-maps.lib",
			"eepp-physics.lib",
			"eepp-maps-static.lib",
			"eepp-physics-static.lib",
			"eterm.lib",
			"languages-syntax-highlighting.lib"
		)
	}

	foreach ($Lib in $SharedLibs) {
		$Source = "libs\windows\$Arch\$Lib"
		if (-not (Test-Path $Source)) {
			Write-Warning "Library not found, skipping: $Source"
			continue
		}
		Copy-Item -Path $Source -Destination "$Staging\bin\$Lib"
		Copy-Item -Path $Source -Destination "$Staging\libs\windows\$Arch\$Lib"
	}

	foreach ($Lib in $LinkLibs) {
		$Source = "libs\windows\$Arch\$Lib"
		if (-not (Test-Path $Source)) {
			Write-Warning "Library not found, skipping: $Source"
			continue
		}
		Copy-Item -Path $Source -Destination "$Staging\libs\windows\$Arch\$Lib"
	}

	# -----------------------------------------------------------------------
	# Headers, documentation and package README.
	# -----------------------------------------------------------------------

	Copy-Item -Recurse -Path "include" -Destination "$Staging\include"
	Copy-Item -Recurse -Path "docs\articles" -Destination "$Staging\docs\articles"
	(Get-Content "projects\scripts\eepp-sdk-README.md" -Raw) -replace '@SDK_VERSION@', $Version | Set-Content "$Staging\README.md"

	# SDL2 runtime DLL next to the binaries.
	$SdlDll = "bin\SDL2.dll"
	if (Test-Path $SdlDll) {
		Copy-Item -Path $SdlDll -Destination "$Staging\bin\SDL2.dll"
	}

	# -----------------------------------------------------------------------
	# Zip.
	# -----------------------------------------------------------------------

	$ZipName = "eepp-sdk-windows-$Arch-$Platform-$ArchiveSuffix.zip"
	Compress-Archive -LiteralPath $Staging -DestinationPath "projects\windows\sdk\$ZipName" -Force
	Write-Output "Generated projects\windows\sdk\$ZipName"
} finally {
	Pop-Location
}
