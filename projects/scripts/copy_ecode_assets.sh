#!/bin/bash

# Usage: ./copy_ecode_assets.sh <source_dir> <dest_dir>
# Example: ./copy_ecode_assets.sh ../../../bin ecode.app

set -euo pipefail

if [ $# -ne 2 ]; then
    echo "Error: Exactly two arguments required: <source_dir> <dest_dir>"
    echo "Usage: $0 <source_dir> <dest_dir>"
    exit 1
fi

SOURCE_DIR="$1"
DEST_DIR="$2"

# Resolve SCRIPT_DIR to the directory where this script lives
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Running copy_ecode_assets.sh from: $SCRIPT_DIR"

# Resolve source_dir relative to the script location if it's relative
if [[ "$SOURCE_DIR" != /* ]]; then
    SOURCE_DIR="$SCRIPT_DIR/$SOURCE_DIR"
fi

# Convert SOURCE_DIR to absolute path and check it exists
SOURCE_DIR=$(realpath "$SOURCE_DIR" 2>/dev/null || {
    echo "Error: Source directory does not exist or cannot be resolved: $1"
    exit 1
})

if [ ! -d "$SOURCE_DIR/assets" ]; then
    echo "Error: Source assets directory not found: $SOURCE_DIR/assets"
    exit 1
fi

echo "Copying assets from $SOURCE_DIR to $DEST_DIR"

# Remove existing destination (if any)
rm -rf "$DEST_DIR"

# Create required directories (with error check)
mkdir -p "$DEST_DIR/assets"/{colorschemes,fonts,i18n,icon,ui,plugins} || {
    echo "Error: Failed to create destination directories in $DEST_DIR"
    exit 1
}

# Helper function to copy and report errors
copy_file() {
    local src="$1"
    local dest="$2"
    if [ -f "$src" ] || [ -d "$src" ]; then
        cp -r "$src" "$dest" || {
            echo "Error: Failed to copy $src -> $dest"
            exit 1
        }
    else
        echo "Error: Source file/directory not found: $src"
        exit 1
    fi
}

# Copy whole directories
# We copy into 'assets/' because mkdir already created the subfolders.
# cp will merge the source folder into the existing destination folder.
copy_file "$SOURCE_DIR/assets/i18n"          "$DEST_DIR/assets/"
copy_file "$SOURCE_DIR/assets/colorschemes"  "$DEST_DIR/assets/"
copy_file "$SOURCE_DIR/assets/plugins"       "$DEST_DIR/assets/"

# Copy individual fonts using a loop
fonts=(
    DejaVuSansMono.ttf
    DejaVuSansMono-Bold.ttf
    DejaVuSansMono-Oblique.ttf
    DejaVuSansMono-BoldOblique.ttf
    DejaVuSansMonoNerdFontComplete.ttf
    nonicons.ttf
    codicon.ttf
    NotoSans-Regular.ttf
    NotoSans-Bold.ttf
    NotoSans-Italic.ttf
    NotoSans-BoldItalic.ttf
    remixicon.ttf
    NotoEmoji-Regular.ttf
    NotoColorEmoji.ttf
    DroidSansFallbackFull.ttf
)

for font in "${fonts[@]}"; do
    copy_file "$SOURCE_DIR/assets/fonts/$font" "$DEST_DIR/assets/fonts/"
done

# Copy other individual files
copy_file "$SOURCE_DIR/assets/icon/ecode.png"        "$DEST_DIR/assets/icon/ecode.png"
copy_file "$SOURCE_DIR/assets/ui/breeze.css"         "$DEST_DIR/assets/ui/breeze.css"
copy_file "$SOURCE_DIR/assets/ca-bundle.pem"         "$DEST_DIR/assets/ca-bundle.pem"

echo "Assets successfully copied to $DEST_DIR"
