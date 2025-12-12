<#
.SYNOPSIS
    Copy ecode assets from a source directory to a destination directory.

.USAGE
    .\copy_ecode_assets.ps1 <source_dir> <dest_dir>
.EXAMPLE
    .\copy_ecode_assets.ps1 ..\..\..\bin ecode.app
#>

param (
    [Parameter(Mandatory = $true)]
    [string]$SourceDir,

    [Parameter(Mandatory = $true)]
    [string]$DestDir
)

# Ensure exactly two arguments
if ($args.Count -ne 2 -and (-not $PSBoundParameters.ContainsKey('SourceDir'))) {
    Write-Error "Error: Exactly two arguments required: <source_dir> <dest_dir>"
    Write-Host "Usage: .\copy_ecode_assets.ps1 <source_dir> <dest_dir>"
    exit 1
}

# Resolve SCRIPT_DIR to the directory where this script lives
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
Write-Host "Running copy_ecode_assets.ps1 from: $ScriptDir"

# Resolve source_dir relative to the script location if it's relative
if (-not [System.IO.Path]::IsPathRooted($SourceDir)) {
    $SourceDir = Join-Path $ScriptDir $SourceDir
}

# Convert SOURCE_DIR to absolute path and check it exists
try {
    $SourceDir = (Resolve-Path $SourceDir -ErrorAction Stop).Path
} catch {
    Write-Error "Error: Source directory does not exist or cannot be resolved: $SourceDir"
    exit 1
}

if (-not (Test-Path (Join-Path $SourceDir "assets"))) {
    Write-Error "Error: Source assets directory not found: $SourceDir\assets"
    exit 1
}

Write-Host "Copying assets from $SourceDir to $DestDir"

# Remove existing destination (if any)
if (Test-Path $DestDir) {
    Remove-Item $DestDir -Recurse -Force
}

# Create required directories
$subDirs = "colorschemes","fonts","i18n","icon","ui","plugins"
foreach ($d in $subDirs) {
    $target = Join-Path $DestDir "assets\$d"
    try {
        New-Item -ItemType Directory -Force -Path $target | Out-Null
    } catch {
        Write-Error "Error: Failed to create destination directories in $DestDir"
        exit 1
    }
}

function Copy-FileOrDir($src, $dest) {
    if (Test-Path $src) {
        try {
            Copy-Item $src -Destination $dest -Recurse -Force
        } catch {
            Write-Error "Error: Failed to copy $src -> $dest"
            exit 1
        }
    } else {
        Write-Error "Error: Source file/directory not found: $src"
        exit 1
    }
}

# Copy whole directories
Copy-FileOrDir (Join-Path $SourceDir "assets\i18n")         (Join-Path $DestDir "assets")
Copy-FileOrDir (Join-Path $SourceDir "assets\colorschemes") (Join-Path $DestDir "assets")
Copy-FileOrDir (Join-Path $SourceDir "assets\plugins")      (Join-Path $DestDir "assets")

# Copy individual fonts
$fonts = @(
    "DejaVuSansMono.ttf",
    "DejaVuSansMono-Bold.ttf",
    "DejaVuSansMono-Oblique.ttf",
    "DejaVuSansMono-BoldOblique.ttf",
    "DejaVuSansMonoNerdFontComplete.ttf",
    "nonicons.ttf",
    "codicon.ttf",
    "NotoSans-Regular.ttf",
    "NotoSans-Bold.ttf",
    "NotoSans-Italic.ttf",
    "NotoSans-BoldItalic.ttf",
    "remixicon.ttf",
    "NotoEmoji-Regular.ttf",
    "NotoColorEmoji.ttf",
    "DroidSansFallbackFull.ttf"
)

foreach ($font in $fonts) {
    Copy-FileOrDir (Join-Path $SourceDir "assets\fonts\$font") (Join-Path $DestDir "assets\fonts")
}

# Copy other individual files
Copy-FileOrDir (Join-Path $SourceDir "assets\icon\ecode.png")   (Join-Path $DestDir "assets\icon\ecode.png")
Copy-FileOrDir (Join-Path $SourceDir "assets\ui\breeze.css")    (Join-Path $DestDir "assets\ui\breeze.css")
Copy-FileOrDir (Join-Path $SourceDir "assets\ca-bundle.pem")    (Join-Path $DestDir "assets\ca-bundle.pem")

Write-Host "Assets successfully copied to $DestDir"
exit 0
